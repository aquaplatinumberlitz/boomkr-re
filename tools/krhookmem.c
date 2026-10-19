/* krhookmem.exe — instrument process từ NGOÀI (không inject DLL).
 * Hook IAT (của exe và của các module CRT) để bắt call site + dữ liệu.
 * usage: krhookmem.exe <exe|pid> <outdir> [giay]
 */
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *log_;
static void L(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    if (log_) { vfprintf(log_, fmt, ap); fputc('\n', log_); fflush(log_); }
    vfprintf(stdout, fmt, ap); fputc('\n', stdout);
    va_end(ap);
}

static DWORD find_pid(const char *arg) {
    if (arg[0] >= '0' && arg[0] <= '9') return (DWORD)strtoul(arg, NULL, 10);
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe; pe.dwSize = sizeof pe; DWORD pid = 0;
    if (Process32First(s, &pe)) do {
        if (_stricmp(pe.szExeFile, arg) == 0) { pid = pe.th32ProcessID; break; }
    } while (Process32Next(s, &pe));
    CloseHandle(s);
    return pid;
}

static DWORD g_modsize;

/* module đầu tiên của process = ảnh chính (nơi chứa IAT) */
static DWORD main_mod(DWORD pid, DWORD *size) {
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32 me; me.dwSize = sizeof me; DWORD b = 0;
    if (Module32First(s, &me)) { b = (DWORD)(UINT_PTR)me.modBaseAddr; if (size) *size = me.modBaseSize; }
    CloseHandle(s);
    return b;
}

static DWORD mod_base(DWORD pid, const char *name) {
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32 me; me.dwSize = sizeof me; DWORD b = 0;
    if (Module32First(s, &me)) do {
        if (_stricmp(me.szModule, name) == 0) { b = (DWORD)(UINT_PTR)me.modBaseAddr; g_modsize = me.modBaseSize; break; }
    } while (Module32Next(s, &me));
    CloseHandle(s);
    return b;
}

typedef struct { DWORD pid, page, slot; HANDLE h; const char *name; int kind; } Hook;

/* tạo stub + patch một slot */
static int patch_slot(Hook *hk, HANDLE h, DWORD pid, DWORD fnVa, DWORD slot, const char *label, int kind) {
    void *page = VirtualAllocEx(h, NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!page) return 0;
    DWORD pv = (DWORD)(UINT_PTR)page;
    unsigned char cd[128]; int n = 0;
    cd[n++] = 0x60;
    cd[n++] = 0x8B; cd[n++] = 0x44; cd[n++] = 0x24; cd[n++] = 0x20;
    cd[n++] = 0xA3; *(DWORD *)(cd + n) = pv + 0; n += 4;
    cd[n++] = 0x8B; cd[n++] = 0x44; cd[n++] = 0x24; cd[n++] = 0x24;
    cd[n++] = 0xA3; *(DWORD *)(cd + n) = pv + 4; n += 4;
    cd[n++] = 0x8B; cd[n++] = 0x44; cd[n++] = 0x24; cd[n++] = 0x28;
    cd[n++] = 0xA3; *(DWORD *)(cd + n) = pv + 8; n += 4;
    cd[n++] = 0x8B; cd[n++] = 0x44; cd[n++] = 0x24; cd[n++] = 0x2C;
    cd[n++] = 0xA3; *(DWORD *)(cd + n) = pv + 12; n += 4;
    cd[n++] = 0x61;
    cd[n++] = 0xFF; cd[n++] = 0x25; *(DWORD *)(cd + n) = pv + 0x100; n += 4;

    unsigned char *bb = (unsigned char *)calloc(1, 0x400);
    *(DWORD *)(bb + 0x100) = fnVa;
    memcpy(bb + 0x104, cd, n);
    SIZE_T wr = 0;
    if (!WriteProcessMemory(h, page, bb, 0x104 + n, &wr)) { free(bb); return 0; }
    DWORD hv = pv + 0x104, old = 0;
    ReadProcessMemory(h, (void *)(UINT_PTR)slot, &old, 4, &wr);
    if (!WriteProcessMemory(h, (void *)(UINT_PTR)slot, &hv, 4, &wr)) { free(bb); return 0; }
    free(bb);
    hk->page = pv; hk->slot = slot; hk->name = label; hk->kind = kind; hk->pid = pid; hk->h = h;
    L("  hook %-13s pid=%-6lu slot=0x%-8lx -> 0x%lx", label, pid, slot, hv);
    return 1;
}

/* tìm slot trong dải ảnh của exe */
static int install_api(Hook *hk, DWORD pid, HANDLE h, const char *dll, const char *fn, const char *label, int kind) {
    HMODULE m = LoadLibraryA(dll);
    FARPROC p = m ? GetProcAddress(m, fn) : NULL;
    if (!p) return 0;
    DWORD rva = (DWORD)((UINT_PTR)p - (UINT_PTR)m);
    DWORD base = mod_base(pid, dll);
    if (!base) return 0;
    DWORD fnVa = base + rva;
    DWORD msz2 = 0;
    DWORD mbase = main_mod(pid, &msz2);
    if (!mbase) return 0;
    DWORD scan_end = mbase + (msz2 ? msz2 : 0x2000000);
    MEMORY_BASIC_INFORMATION mbi;
    unsigned char *a = (unsigned char *)(mbase & ~0xFFF);
    while ((UINT_PTR)a < scan_end) {
        if (!VirtualQueryEx(h, a, &mbi, sizeof mbi)) break;
        SIZE_T sz = mbi.RegionSize; if (!sz) break;
        if (mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            SIZE_T w = sz > 0x400000 ? 0x400000 : sz, got = 0;
            unsigned char *b = (unsigned char *)malloc(w);
            if (b && ReadProcessMemory(h, a, b, w, &got)) {
                for (SIZE_T i = 0; i + 4 <= got; i += 4) {
                    DWORD v; memcpy(&v, b + i, 4);
                    if (v == fnVa) { DWORD slot = (DWORD)(UINT_PTR)a + (DWORD)i; free(b);
                                     return patch_slot(hk, h, pid, fnVa, slot, label, kind); }
                }
            }
            free(b);
        }
        a = (unsigned char *)mbi.BaseAddress + sz;
    }
    return 0;
}

/* patch moi slot trong TAT CA module (tìm IAT của user32 trong DLL khác) */
static int install_all(Hook *out, int max, DWORD pid, HANDLE h, const char *dll, const char *fn, const char *label, int kind) {
    HMODULE m = LoadLibraryA(dll);
    FARPROC p = m ? GetProcAddress(m, fn) : NULL;
    if (!p) return 0;
    DWORD rva = (DWORD)((UINT_PTR)p - (UINT_PTR)m);
    DWORD base = mod_base(pid, dll);
    if (!base) return 0;
    DWORD fnVa = base + rva;
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32 me; me.dwSize = sizeof me; int n = 0;
    if (Module32First(s, &me)) do {
        DWORD mb = (DWORD)(UINT_PTR)me.modBaseAddr, msz = me.modBaseSize;
        SIZE_T got = 0;
        unsigned char *b = (unsigned char *)malloc(msz);
        if (!b) continue;
        if (ReadProcessMemory(h, (void *)(UINT_PTR)mb, b, msz, &got)) {
            for (SIZE_T i = 0; i + 4 <= got && n < max; i += 4) {
                DWORD v; memcpy(&v, b + i, 4);
                if (v == fnVa) {
                    Hook hk = {0};
                    if (patch_slot(&hk, h, pid, fnVa, mb + (DWORD)i, label, kind)) out[n++] = hk;
                }
            }
        }
        free(b);
    } while (Module32Next(s, &me));
    CloseHandle(s);
    return n;
}

/* patch moi slot trong cac module CRT: msvcrt, ucrtbase, msvcrXX, api-ms-win-crt-* */
static int install_crt(Hook *out, int max, DWORD pid, HANDLE h, const char *fn, const char *label, int kind) {
    HMODULE km = LoadLibraryA("kernel32.dll");
    FARPROC p = GetProcAddress(km, fn);
    if (!p) return 0;
    DWORD rva = (DWORD)((UINT_PTR)p - (UINT_PTR)km);
    DWORD kbase = mod_base(pid, "kernel32.dll");
    if (!kbase) return 0;
    DWORD fnVa = kbase + rva;
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32 me; me.dwSize = sizeof me; int n = 0;
    if (Module32First(s, &me)) do {
        const char *nm = me.szModule;
        if (!(strstr(nm, "msvcrt") || strstr(nm, "ucrtbase") || strstr(nm, "msvcr") || strstr(nm, "api-ms-win-crt")))
            continue;
        DWORD base = (DWORD)(UINT_PTR)me.modBaseAddr, size = me.modBaseSize;
        SIZE_T got = 0;
        unsigned char *b = (unsigned char *)malloc(size);
        if (!b) continue;
        if (ReadProcessMemory(h, (void *)(UINT_PTR)base, b, size, &got)) {
            for (SIZE_T i = 0; i + 4 <= got && n < max; i += 4) {
                DWORD v; memcpy(&v, b + i, 4);
                if (v == fnVa) {
                    Hook hk = {0};
                    if (patch_slot(&hk, h, pid, fnVa, base + (DWORD)i, label, kind)) out[n++] = hk;
                }
            }
        }
        free(b);
    } while (Module32Next(s, &me));
    CloseHandle(s);
    return n;
}

int main(int argc, char **argv) {
    if (argc < 3) { printf("usage: krhookmem.exe <exe|pid> <outdir> [giay]\n"); return 2; }
    int seconds = argc > 3 ? atoi(argv[3]) : 30;
    const char *proc_name = argc > 4 ? argv[4] : "CA.exe";
    CreateDirectoryA(argv[2], NULL);
    char path[MAX_PATH]; snprintf(path, sizeof path, "%s\\hookmem.log", argv[2]);
    log_ = fopen(path, "w");

    Hook hooks[64]; int nh = 0;
    struct { DWORD pid; HANDLE h; int attempts; int done[16]; } P[16]; int np = 0;

    for (int t = 0; t < seconds * 4; t++) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 pe; pe.dwSize = sizeof pe;
        if (Process32First(snap, &pe)) do {
            if (_stricmp(pe.szExeFile, proc_name) != 0) continue;
            int known = 0;
            for (int i = 0; i < np; i++) if (P[i].pid == pe.th32ProcessID) known = 1;
            if (known || np >= 16) continue;
            HANDLE h = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE |
                                   PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
            if (!h) continue;
            L("process pid=%lu", pe.th32ProcessID);
            P[np].pid = pe.th32ProcessID; P[np].h = h; P[np].attempts = 0;
            for (int k = 0; k < 16; k++) P[np].done[k] = 0;
            np++;
        } while (Process32Next(snap, &pe));
        CloseHandle(snap);

        for (int i = 0; i < np && P[i].attempts <= 60; i++) {
            P[i].attempts++;
            Hook hk;
            if (!P[i].done[0]) { hk = (Hook){0}; if (install_api(&hk, P[i].pid, P[i].h, "kernel32.dll", "ExitProcess", "ExitProcess", 0)) { hooks[nh++] = hk; P[i].done[0] = 1; } }
            if (!P[i].done[1]) { hk = (Hook){0}; if (install_api(&hk, P[i].pid, P[i].h, "user32.dll", "MessageBoxA", "MessageBoxA", 1)) { hooks[nh++] = hk; P[i].done[1] = 1; } }
            if (!P[i].done[2]) { int c = install_crt(&hooks[nh], 6, P[i].pid, P[i].h, "WriteFile", "WriteFile(CRT)", 4); if (c) { nh += c; P[i].done[2] = 1; } }
            if (!P[i].done[3]) { int c = install_crt(&hooks[nh], 4, P[i].pid, P[i].h, "CreateFileW", "CreateFileW(CRT)", 2); if (c) { nh += c; P[i].done[3] = 1; } }
            if (!P[i].done[4]) { hk = (Hook){0}; if (install_api(&hk, P[i].pid, P[i].h, "user32.dll", "MessageBoxW", "MessageBoxW", 2)) { hooks[nh++] = hk; P[i].done[4] = 1; } }
            if (!P[i].done[5]) { hk = (Hook){0}; if (install_api(&hk, P[i].pid, P[i].h, "user32.dll", "DialogBoxParamW", "DialogBoxParamW", 0)) { hooks[nh++] = hk; P[i].done[5] = 1; } }
            if (!P[i].done[6]) { hk = (Hook){0}; if (install_api(&hk, P[i].pid, P[i].h, "user32.dll", "CreateDialogParamW", "CreateDialogParamW", 0)) { hooks[nh++] = hk; P[i].done[6] = 1; } }
            if (!P[i].done[7]) { hk = (Hook){0}; if (install_api(&hk, P[i].pid, P[i].h, "user32.dll", "MessageBoxIndirectA", "MessageBoxIndirectA", 0)) { hooks[nh++] = hk; P[i].done[7] = 1; } }
            if (!P[i].done[8]) { hk = (Hook){0}; if (install_api(&hk, P[i].pid, P[i].h, "user32.dll", "DialogBoxParamA", "DialogBoxParamA", 0)) { hooks[nh++] = hk; P[i].done[8] = 1; } }
            if (!P[i].done[9]) { hk = (Hook){0}; if (install_api(&hk, P[i].pid, P[i].h, "user32.dll", "CreateDialogParamA", "CreateDialogParamA", 0)) { hooks[nh++] = hk; P[i].done[9] = 1; } }
            if (!P[i].done[10]) { int c = install_all(&hooks[nh], 8, P[i].pid, P[i].h, "user32.dll", "MessageBoxW", "MessageBoxW(any)", 2); if (c) { nh += c; P[i].done[10] = 1; } }
            if (!P[i].done[11]) { int c = install_all(&hooks[nh], 8, P[i].pid, P[i].h, "user32.dll", "MessageBoxA", "MessageBoxA(any)", 1); if (c) { nh += c; P[i].done[11] = 1; } }
            if (!P[i].done[12]) { int c = install_all(&hooks[nh], 6, P[i].pid, P[i].h, "user32.dll", "CreateWindowExW", "CreateWindowExW", 2); if (c) { nh += c; P[i].done[12] = 1; } }
            if (!P[i].done[13]) { int c = install_all(&hooks[nh], 6, P[i].pid, P[i].h, "user32.dll", "CreateWindowExA", "CreateWindowExA", 1); if (c) { nh += c; P[i].done[13] = 1; } }
            if (!P[i].done[14]) { int c = install_all(&hooks[nh], 4, P[i].pid, P[i].h, "user32.dll", "CreateDialogIndirectParamA", "CreateDlgIndA", 0); if (c) { nh += c; P[i].done[14] = 1; } }
            if (!P[i].done[15]) { int c = install_all(&hooks[nh], 6, P[i].pid, P[i].h, "gdi32.dll", "ExtTextOutW", "ExtTextOutW", 5); if (c) { nh += c; P[i].done[15] = 1; } }
        }

        for (int i = 0; i < nh; i++) {
            DWORD v[4] = {0, 0, 0, 0}; SIZE_T got = 0;
            if (!ReadProcessMemory(hooks[i].h, (void *)(UINT_PTR)hooks[i].page, v, sizeof v, &got)) continue;
            if (!v[0]) continue;
            if (hooks[i].kind == 4 && v[2]) {
                DWORD len = v[3] > 200 ? 200 : v[3];
                char buf[256] = {0};
                if (len && ReadProcessMemory(hooks[i].h, (void *)(UINT_PTR)v[1], buf, len, &got)) {
                    for (DWORD k = 0; k < len; k++) if (buf[k] == '\r' || buf[k] == '\n') buf[k] = ' ';
                    L("WriteFile pid=%lu caller=0x%lx len=%lu: %s", hooks[i].pid, v[0], len, buf);
                }
            } else if (hooks[i].kind == 5 && v[5] && v[6] && v[6] < 200) {
                wchar_t wt[260] = {0};
                if (ReadProcessMemory(hooks[i].h, (void *)(UINT_PTR)v[5], wt, v[6] * 2, &got) && wt[0]) {
                    char u8[600] = {0};
                    WideCharToMultiByte(CP_UTF8, 0, wt, v[6], u8, sizeof u8 - 1, NULL, NULL);
                    L("ExtTextOutW pid=%lu caller=0x%lx len=%lu: %s", hooks[i].pid, v[0], v[6], u8);
                }
            } else if (hooks[i].kind == 2 && v[1]) {
                wchar_t wt[260] = {0};
                if (ReadProcessMemory(hooks[i].h, (void *)(UINT_PTR)v[1], wt, sizeof wt - 2, &got) && wt[0]) {
                    char a2[600] = {0};
                    WideCharToMultiByte(CP_UTF8, 0, wt, -1, a2, sizeof a2 - 1, NULL, NULL);
                    L("%s pid=%lu caller=0x%lx path=%s", hooks[i].name, hooks[i].pid, v[0], a2);
                }
            } else if (hooks[i].kind == 1 && v[1]) {
                char txt[300] = {0};
                ReadProcessMemory(hooks[i].h, (void *)(UINT_PTR)v[1], txt, sizeof txt - 1, &got);
                L("%s pid=%lu caller=0x%lx text=%s", hooks[i].name, hooks[i].pid, v[0], txt);
            } else {
                L("%s pid=%lu caller=0x%lx arg0=0x%lx arg1=0x%lx", hooks[i].name, hooks[i].pid, v[0], v[1], v[2]);
            }
            DWORD z[4] = {0, 0, 0, 0};
            WriteProcessMemory(hooks[i].h, (void *)(UINT_PTR)hooks[i].page, z, sizeof z, &got);
        }
        Sleep(250);
    }
    L("ket thuc (giu hook)");
    if (log_) fclose(log_);
    return 0;
}
