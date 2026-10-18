/* krdump32.exe — dumper 32-bit cho process 32-bit (CA.exe KR).
 *
 * Vì sao cần: PowerShell 64-bit dùng VirtualQueryEx với MEMORY_BASIC_INFORMATION 64-bit lên target
 * 32-bit thì không đi được (trả 0 vùng). Bản 32-bit dùng đúng cấu trúc 32-bit.
 *
 *   krdump32.exe <pid> <outdir>        dump module + mọi vùng commit
 *   krdump32.exe --all <outdir>        (không dùng)
 */
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static FILE *logf_;
static void logf(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vfprintf(logf_ ? logf_ : stdout, fmt, ap);
    va_end(ap);
    if (logf_) fflush(logf_);
}

int main(int argc, char **argv) {
    if (argc < 3) { printf("usage: krdump32.exe <pid> <outdir>\n"); return 2; }
    DWORD pid = 0;
    if (argv[1][0] >= '0' && argv[1][0] <= '9') pid = (DWORD)strtoul(argv[1], NULL, 10);
    else {  /* tìm pid theo tên exe */
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 pe32; pe32.dwSize = sizeof pe32;
        if (Process32First(snap, &pe32)) do {
            if (_stricmp(pe32.szExeFile, argv[1]) == 0) { pid = pe32.th32ProcessID; break; }
        } while (Process32Next(snap, &pe32));
        CloseHandle(snap);
        if (!pid) { printf("khong tim thay process %s\n", argv[1]); return 1; }
    }
    const char *dir = argv[2];
    CreateDirectoryA(dir, NULL);
    char path[MAX_PATH];
    snprintf(path, sizeof path, "%s\\dump.log", dir);
    logf_ = fopen(path, "w");

    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!h) { logf("OpenProcess lỗi %lu\n", GetLastError()); return 1; }
    static unsigned char buf2[0x100000];

    /* module: đọc theo từng vùng con (module có trang không đọc được -> ReadProcessMemory 1 phát sẽ fail) */
    HMODULE mods[1024]; DWORD need = 0, n = 0;
    if (EnumProcessModulesEx(h, mods, sizeof mods, &need, LIST_MODULES_32BIT)) n = need / sizeof(HMODULE);
    logf("%lu module\n", n);
    for (DWORD i = 0; i < n; i++) {
        char name[MAX_PATH] = {0};
        GetModuleBaseNameA(h, mods[i], name, MAX_PATH);
        MODULEINFO mi = {0};
        if (!GetModuleInformation(h, mods[i], &mi, sizeof mi)) continue;
        char out[MAX_PATH];
        snprintf(out, sizeof out, "%s\\mod_%08x_%s.bin", dir, (unsigned)(UINT_PTR)mods[i], name);
        FILE *mf = fopen(out, "wb");
        if (!mf) { logf("  không ghi được %s\n", out); continue; }
        SIZE_T total = 0, done = 0;
        unsigned char *a = (unsigned char *)mi.lpBaseOfDll;
        unsigned char *endp = a + mi.SizeOfImage;
        while (a < endp) {
            MEMORY_BASIC_INFORMATION mbi;
            if (!VirtualQueryEx(h, a, &mbi, sizeof mbi)) break;
            SIZE_T sz = mbi.RegionSize;
            if (!sz) break;
            unsigned char *lim = (unsigned char *)mbi.BaseAddress + sz;
            if (lim > endp) lim = endp;
            SIZE_T want = (SIZE_T)(lim - a);
            SIZE_T off = 0;
            int readable = (mbi.State == MEM_COMMIT) && !(mbi.Protect & PAGE_NOACCESS) && !(mbi.Protect & PAGE_GUARD);
            while (off < want) {
                SIZE_T chunk = want - off > sizeof buf2 ? sizeof buf2 : want - off;
                SIZE_T got = 0;
                if (readable && ReadProcessMemory(h, a + off, buf2, chunk, &got) && got == chunk) {
                    fwrite(buf2, 1, chunk, mf); done += chunk;
                } else {
                    unsigned char z[4096] = {0};
                    SIZE_T k = 0;
                    while (k < chunk) { SIZE_T c2 = chunk - k > sizeof z ? sizeof z : chunk - k; fwrite(z, 1, c2, mf); k += c2; }
                }
                off += chunk;
            }
            total += want;
            a = lim;
        }
        fclose(mf);
        logf("  module %-20s base=%p size=0x%x doc=0x%x\n", name, mi.lpBaseOfDll, (unsigned)mi.SizeOfImage, (unsigned)done);
    }

    /* export RVA của vài API quan trọng (RVA giống nhau giữa các process) */
    {
        struct { const char *dll, *fn; } want[] = {
            {"user32.dll","MessageBoxA"}, {"user32.dll","MessageBoxW"},
            {"kernel32.dll","ExitProcess"}, {"kernel32.dll","TerminateProcess"},
            {"wintrust.dll","WinVerifyTrust"}, {"wininet.dll","InternetOpenA"},
        };
        for (int i = 0; i < (int)(sizeof want / sizeof want[0]); i++) {
            HMODULE m = LoadLibraryA(want[i].dll);
            if (!m) { logf("export %s!%s: không load được\n", want[i].dll, want[i].fn); continue; }
            FARPROC p2 = GetProcAddress(m, want[i].fn);
            if (!p2) { logf("export %s!%s: không có\n", want[i].dll, want[i].fn); continue; }
            logf("export %s!%s rva=0x%x\n", want[i].dll, want[i].fn,
                 (unsigned)((UINT_PTR)p2 - (UINT_PTR)m));
        }
    }

    /* mọi vùng commit */
    FILE *tsv = fopen((snprintf(path, sizeof path, "%s\\regions.tsv", dir), path), "w");
    int nreg = 0;
    unsigned char *buf = malloc(0x400000);
    for (unsigned char *a = (unsigned char *)0x10000; (UINT_PTR)a < 0x7FFF0000; ) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQueryEx(h, a, &mbi, sizeof mbi)) { logf("VirtualQueryEx dừng tại %p\n", a); break; }
        SIZE_T sz = mbi.RegionSize;
        if (!sz) break;
        if (mbi.State == MEM_COMMIT && sz <= 0x400000) {
            SIZE_T got = 0;
            if (ReadProcessMemory(h, mbi.BaseAddress, buf, sz, &got) && got == sz) {
                snprintf(path, sizeof path, "%s\\reg_%08x.bin", dir, (unsigned)(UINT_PTR)mbi.BaseAddress);
                FILE *f = fopen(path, "wb"); if (f) { fwrite(buf, 1, sz, f); fclose(f); }
                fprintf(tsv, "%p\t%x\t%x\n", mbi.BaseAddress, (unsigned)sz, mbi.Protect);
                nreg++;
            }
        }
        a = (unsigned char *)mbi.BaseAddress + sz;
    }
    logf("%d vùng commit đã dump\n", nreg);
    fclose(tsv); free(buf); CloseHandle(h);
    if (logf_) fclose(logf_);
    return 0;
}
