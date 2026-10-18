/* krhook.dll — hook các API mà client KR gọi ngay trước khi thoát (WinTrust/WinINet),
 * và ghi lại return address của ExitProcess để tìm chỗ chặn.
 *
 * Inline hook (5 byte: mov eax,imm32; jmp eax) — an toàn với delay-load vì ta LoadLibrary
 * chính DLL đích trước rồi mới vá prologue.
 *
 * Log: C:\boomkr\krhook.log (hoặc cùng thư mục exe nếu không ghi được).
 */
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <wininet.h>

typedef LONG (WINAPI *PFN_WVT)(GUID *, LPVOID, LPVOID, LONG, LPVOID, LPVOID *);
typedef LONG (WINAPI *PFN_WVTEX)(GUID *, LPVOID, LPVOID, LONG, LPVOID, LPVOID *);

static void krlog(const char *fmt, ...) {
    char path[MAX_PATH], buf[1024];
    DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
    char *p = strrchr(path, '\\');
    if (p) *p = 0;
    lstrcatA(path, "\\krhook.log");
    FILE *f = fopen(path, "a");
    if (!f) return;
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    fputs(buf, f); fputc('\n', f); fclose(f);
}

static int hook_prologue(void *fn, void *mine, const char *name) {
    if (!fn) { krlog("hook %s: không có hàm", name); return 0; }
    DWORD old;
    unsigned char *p = (unsigned char *)fn;
    if (!VirtualProtect(p, 8, PAGE_EXECUTE_READWRITE, &old)) return 0;
    p[0] = 0xB8;                                  /* mov eax, imm32 */
    *(unsigned int *)(p + 1) = (unsigned int)(size_t)mine;
    p[5] = 0xFF; p[6] = 0xE0;                     /* jmp eax */
    VirtualProtect(p, 8, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 8);
    krlog("hook %s tại %p", name, fn);
    return 1;
}

static LONG WINAPI my_WinVerifyTrust(GUID *a, LPVOID b, LPVOID c, LONG d, LPVOID e, LPVOID *f) {
    krlog("WinVerifyTrust(...) -> giả 0");
    return 0;                                      /* ERROR_SUCCESS */
}

static LONG WINAPI my_WinVerifyTrustEx(GUID *a, LPVOID b, LPVOID c, LONG d, LPVOID e, LPVOID *f) {
    krlog("WinVerifyTrustEx(...) -> giả 0");
    return 0;
}

static HINTERNET WINAPI my_InternetOpenA(LPCSTR agent, DWORD type, LPCSTR proxy, LPCSTR bypass, DWORD flags) {
    krlog("InternetOpenA(%s)", agent ? agent : "(null)");
    return (HINTERNET)1;
}
static HINTERNET WINAPI my_InternetConnectA(HINTERNET h, LPCSTR host, INTERNET_PORT port,
                                            LPCSTR user, LPCSTR pass, DWORD svc, DWORD flags,
                                            DWORD_PTR ctx) {
    krlog("InternetConnectA(%s:%u)", host ? host : "(null)", (unsigned)port);
    return (HINTERNET)1;
}
static HINTERNET WINAPI my_HttpOpenRequestA(HINTERNET h, LPCSTR verb, LPCSTR obj, LPCSTR ver,
                                            LPCSTR ref, LPCSTR *accept, DWORD flags, DWORD_PTR ctx) {
    krlog("HttpOpenRequestA(%s %s)", verb ? verb : "?", obj ? obj : "?");
    return (HINTERNET)1;
}
static BOOL WINAPI my_HttpSendRequestA(HINTERNET h, LPCSTR hdrs, DWORD hl, LPVOID opt, DWORD ol) {
    krlog("HttpSendRequestA(...) -> giả TRUE");
    return TRUE;
}
static BOOL WINAPI my_InternetReadFile(HINTERNET h, LPVOID buf, DWORD want, LPDWORD got) {
    krlog("InternetReadFile(want=%lu) -> 0 byte", (unsigned long)want);
    if (got) *got = 0;
    return TRUE;
}

/* Chặn thoát: nếu code != 0 thì KHÔNG gọi ExitProcess thật, chỉ log rồi trả về.
 * Mục đích: xem client có đi tiếp được qua cửa ProcessPatch hay không. */
static int g_nosuppress_done;

static void WINAPI my_ExitProcess(UINT code) {
    void *ra0 = __builtin_return_address(0);
    krlog("ExitProcess(%u) gọi từ %p", code, ra0);
    if (code != 0) {
        /* chỉ chặn lần đầu để tránh vòng lặp vô hạn */
        if (!g_nosuppress_done) {
            g_nosuppress_done = 1;
            krlog("*** CHẶN ExitProcess(%u): trả về thay vì thoát ***", code);
            return;
        }
        krlog("*** bỏ qua chặn, thoát thật ***");
    }
    ExitProcess(code);
}

static BOOL WINAPI my_TerminateProcess(HANDLE h, UINT code) {
    krlog("TerminateProcess(pid=%lu, %u) gọi từ %p", (unsigned long)(size_t)h, code,
          __builtin_return_address(0));
    return TerminateProcess(h, code);
}

static void hexlog(const char *tag, const char *p) {
    char buf[1200]; int n = 0;
    n += snprintf(buf + n, sizeof buf - n, "%s bytes=", tag);
    for (int i = 0; p && p[i] && n < (int)sizeof buf - 8; i++)
        n += snprintf(buf + n, sizeof buf - n, "%02x", (unsigned char)p[i]);
    krlog("%s", buf);
}

static int WINAPI my_WideCharToMultiByte(UINT cp, DWORD flags, LPCWSTR in, int inlen,
                                        LPSTR out, int outlen, LPCSTR defc, LPBOOL used) {
    if (in && inlen > 1 && inlen < 400) {
        char u8[1600] = {0};
        int n = WideCharToMultiByte(CP_UTF8, 0, in, inlen, u8, sizeof u8 - 1, NULL, NULL);
        if (n > 0) krlog("WideCharToMultiByte(cp=%u, inlen=%d) <- \"%s\"", cp, inlen, u8);
    }
    return WideCharToMultiByte(cp, flags, in, inlen, out, outlen, defc, used);
}

static int WINAPI my_MessageBoxA(HWND h, LPCSTR text, LPCSTR cap, UINT type) {
    krlog("MessageBoxA gọi từ %p", __builtin_return_address(0));
    hexlog("MessageBoxA text", text);
    hexlog("MessageBoxA caption", cap);
    return MessageBoxA(h, text, cap, type);
}

static int WINAPI my_MessageBoxW(HWND h, LPCWSTR text, LPCWSTR cap, UINT type) {
    char u8[1024] = {0};
    if (text) WideCharToMultiByte(CP_UTF8, 0, text, -1, u8, sizeof u8, NULL, NULL);
    krlog("MessageBoxW text(utf8)=%s", u8);
    if (cap) { WideCharToMultiByte(CP_UTF8, 0, cap, -1, u8, sizeof u8, NULL, NULL); krlog("MessageBoxW caption=%s", u8); }
    return MessageBoxW(h, text, cap, type);
}

typedef void (WINAPI *PFN_RTL_EXIT)(UINT);
static PFN_RTL_EXIT real_RtlExitUserProcess;

static void WINAPI my_RtlExitUserProcess(UINT code) {
    krlog("RtlExitUserProcess(%u) gọi từ %p", code, __builtin_return_address(0));
    real_RtlExitUserProcess(code);
}

static DWORD WINAPI install(LPVOID unused) {
    Sleep(50);   /* để loader resolve xong import của exe */
    HMODULE wintrust = LoadLibraryA("wintrust.dll");
    HMODULE wininet  = LoadLibraryA("wininet.dll");
    if (wintrust) {
        hook_prologue((void *)GetProcAddress(wintrust, "WinVerifyTrust"), (void *)my_WinVerifyTrust, "WinVerifyTrust");
        hook_prologue((void *)GetProcAddress(wintrust, "WinVerifyTrustEx"), (void *)my_WinVerifyTrustEx, "WinVerifyTrustEx");
    } else krlog("không load được wintrust.dll");
    if (wininet) {
        hook_prologue((void *)GetProcAddress(wininet, "InternetOpenA"), (void *)my_InternetOpenA, "InternetOpenA");
        hook_prologue((void *)GetProcAddress(wininet, "InternetConnectA"), (void *)my_InternetConnectA, "InternetConnectA");
        hook_prologue((void *)GetProcAddress(wininet, "HttpOpenRequestA"), (void *)my_HttpOpenRequestA, "HttpOpenRequestA");
        hook_prologue((void *)GetProcAddress(wininet, "HttpSendRequestA"), (void *)my_HttpSendRequestA, "HttpSendRequestA");
        hook_prologue((void *)GetProcAddress(wininet, "InternetReadFile"), (void *)my_InternetReadFile, "InternetReadFile");
    } else krlog("không load được wininet.dll");
    hook_prologue((void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess"), (void *)my_ExitProcess, "ExitProcess");
    hook_prologue((void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "TerminateProcess"), (void *)my_TerminateProcess, "TerminateProcess");
    hook_prologue((void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "WideCharToMultiByte"), (void *)my_WideCharToMultiByte, "WideCharToMultiByte");
    hook_prologue((void *)GetProcAddress(GetModuleHandleA("user32.dll"), "MessageBoxA"), (void *)my_MessageBoxA, "MessageBoxA");
    hook_prologue((void *)GetProcAddress(GetModuleHandleA("user32.dll"), "MessageBoxW"), (void *)my_MessageBoxW, "MessageBoxW");
    real_RtlExitUserProcess = (PFN_RTL_EXIT)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlExitUserProcess");
    hook_prologue((void *)real_RtlExitUserProcess, (void *)my_RtlExitUserProcess, "RtlExitUserProcess");
    krlog("cài hook xong");
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        krlog("krhook.dll nạp vào pid=%lu", (unsigned long)GetCurrentProcessId());
        CreateThread(NULL, 0, install, NULL, 0, NULL);
    }
    return TRUE;
}
