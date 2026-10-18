/* krhookmem.exe — instrument process từ NGOÀI (không thêm DLL), hook IAT của API chọn trước.
 *
 * Với mỗi API: ghi [return address, arg0, arg1] vào trang sink rồi jmp tới hàm thật.
 * Ta poll sink từ ngoài ⇒ biết **chỗ nào trong code gọi API** mà không cần debugger.
 *
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
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe; pe.dwSize = sizeof pe; DWORD pid = 0;
    if (Process32First(snap, &pe)) do {
        if (_stricmp(pe.szExeFile, arg) == 0) { pid = pe.th32ProcessID; break; }
    } while (Process32Next(snap, &pe));
    CloseHandle(snap);
    return pid;
}

static DWORD mod_base(DWORD pid, const char *name) {
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32 me; me.dwSize = sizeof me; DWORD b = 0;
    if (Module32First(s, &me)) do {
        if (_stricmp(me.szModule, name) == 0) { b = (DWORD)(UINT_PTR)me.modBaseAddr; break; }
    } while (Module32Next(s, &me));
    CloseHandle(s);
    return b;
}

static DWORD find_slot(HANDLE h, DWORD val, int wide) {
    MEMORY_BASIC_INFORMATION mbi;
    unsigned char *a = (unsigned char *)(wide ? 0x10000 : 0x400000);
    unsigned char *end = (unsigned char *)(wide ? 0x7FFF0000 : 0x2250000);
    while ((UINT_PTR)a < (UINT_PTR)end) {
        if (!VirtualQueryEx(h, a, &mbi, sizeof mbi)) break;
        SIZE_T sz = mbi.RegionSize; if (!sz) break;
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            SIZE_T w = sz > 0x400000 ? 0x400000 : sz, got = 0;
            DWORD *b = (DWORD *)malloc(w);
            if (b && ReadProcessMemory(h, a, b, w, &got)) {
                unsigned char *cb = (unsigned char *)b;
                for (SIZE_T i = 0; i + 4 <= got; i += 4) {
                    DWORD v; memcpy(&v, cb + i, 4);
                    if (v == val) { DWORD s = (DWORD)(UINT_PTR)a + (DWORD)i; free(b); return s; }
                }
            }
            free(b);
        }
        a = (unsigned char *)mbi.BaseAddress + sz;
    }
    return 0;
}

typedef struct { DWORD pid, page, slot; HANDLE h; const char *name; } Hook;

static int install(Hook *hk, DWORD pid, const char *dll, const char *fn, const char *label) {
    HANDLE h = hk->h;
    HMODULE m = LoadLibraryA(dll);
    FARPROC p = m ? GetProcAddress(m, fn) : NULL;
    if (!p) { L("  %s!%s: khong co export", dll, fn); return 0; }
    DWORD rva = (DWORD)((UINT_PTR)p - (UINT_PTR)m);
    DWORD base = mod_base(pid, dll);
    if (!base) { L("  %s khong co trong target", dll); return 0; }
    DWORD fnVa = base + rva;
    DWORD slot = find_slot(h, fnVa, 0);
    if (!slot) { L("  %s: khong thay IAT slot", fn); return 0; }

    void *page = VirtualAllocEx(h, NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!page) { L("  %s: VirtualAllocEx loi %lu", fn, GetLastError()); return 0; }
    DWORD pv = (DWORD)(UINT_PTR)page;
    unsigned char cd[128]; int n = 0;
    cd[n++] = 0x60;
    cd[n++] = 0x8B; cd[n++] = 0x44; cd[n++] = 0x24; cd[n++] = 0x20;
    cd[n++] = 0xA3; *(DWORD *)(cd + n) = pv + 0; n += 4;
    cd[n++] = 0x8B; cd[n++] = 0x44; cd[n++] = 0x24; cd[n++] = 0x24;
    cd[n++] = 0xA3; *(DWORD *)(cd + n) = pv + 4; n += 4;
    cd[n++] = 0x8B; cd[n++] = 0x44; cd[n++] = 0x24; cd[n++] = 0x28;
    cd[n++] = 0xA3; *(DWORD *)(cd + n) = pv + 8; n += 4;
    cd[n++] = 0x61;
    cd[n++] = 0xFF; cd[n++] = 0x25; *(DWORD *)(cd + n) = pv + 0x100; n += 4;

    unsigned char *bb = (unsigned char *)calloc(1, 0x400);
    *(DWORD *)(bb + 0x100) = fnVa;
    memcpy(bb + 0x104, cd, n);
    SIZE_T wr = 0;
    if (!WriteProcessMemory(h, page, bb, 0x104 + n, &wr)) { L("  %s: ghi page loi", fn); free(bb); return 0; }
    DWORD hv = pv + 0x104, old = 0;
    ReadProcessMemory(h, (void *)(UINT_PTR)slot, &old, 4, &wr);
    if (!WriteProcessMemory(h, (void *)(UINT_PTR)slot, &hv, 4, &wr)) { L("  %s: patch slot loi", fn); free(bb); return 0; }
    free(bb);
    hk->page = pv; hk->slot = slot; hk->name = label;
    L("  hook %-14s pid=%-6lu slot=0x%-8lx (cu 0x%lx) -> 0x%lx", label, pid, slot, old, hv);
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 3) { printf("usage: krhookmem.exe <exe|pid> <outdir> [giay]\n"); return 2; }
    int seconds = argc > 3 ? atoi(argv[3]) : 30;
    CreateDirectoryA(argv[2], NULL);
    char path[MAX_PATH]; snprintf(path, sizeof path, "%s\\hookmem.log", argv[2]);
    log_ = fopen(path, "w");

    Hook hooks[32]; int nh = 0;
    DWORD hooked[16]; int nhooked = 0;

    for (int t = 0; t < seconds * 4; t++) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 pe; pe.dwSize = sizeof pe;
        if (Process32First(snap, &pe)) do {
            if (_stricmp(pe.szExeFile, "CA.exe") != 0) continue;
            int done = 0;
            for (int i = 0; i < nhooked; i++) if (hooked[i] == pe.th32ProcessID) done = 1;
            if (done || nhooked >= 16) continue;
            HANDLE h = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE |
                                   PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
            if (!h) continue;
            L("process pid=%lu", pe.th32ProcessID);
            Hook a = {0}, b = {0}, c = {0};
            a.h = b.h = c.h = h;
            a.pid = b.pid = c.pid = pe.th32ProcessID;
            if (install(&a, pe.th32ProcessID, "kernel32.dll", "ExitProcess", "ExitProcess")) hooks[nh++] = a;
            if (install(&c, pe.th32ProcessID, "user32.dll", "MessageBoxA", "MessageBoxA")) hooks[nh++] = c;
            hooked[nhooked++] = pe.th32ProcessID;
        } while (Process32Next(snap, &pe));
        CloseHandle(snap);

        for (int i = 0; i < nh; i++) {
            DWORD v[3] = {0, 0, 0}; SIZE_T got = 0;
            if (!ReadProcessMemory(hooks[i].h, (void *)(UINT_PTR)hooks[i].page, v, sizeof v, &got)) continue;
            if (!v[0]) continue;
            L("%s pid=%lu: caller=0x%lx arg0=0x%lx arg1=0x%lx",
              hooks[i].name, hooks[i].pid, v[0], v[1], v[2]);
            if (strcmp(hooks[i].name, "MessageBoxA") == 0 && v[1]) {
                char txt[400] = {0};
                ReadProcessMemory(hooks[i].h, (void *)(UINT_PTR)v[1], txt, sizeof txt - 1, &got);
                L("   text = %s", txt);
            }
            DWORD z[3] = {0, 0, 0};
            WriteProcessMemory(hooks[i].h, (void *)(UINT_PTR)hooks[i].page, z, sizeof z, &got);
        }
        Sleep(250);
    }
    L("ket thuc (giu hook)");
    if (log_) fclose(log_);
    return 0;
}
