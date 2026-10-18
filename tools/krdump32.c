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
    DWORD pid = (DWORD)strtoul(argv[1], NULL, 10);
    const char *dir = argv[2];
    CreateDirectoryA(dir, NULL);
    char path[MAX_PATH];
    snprintf(path, sizeof path, "%s\\dump.log", dir);
    logf_ = fopen(path, "w");

    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!h) { logf("OpenProcess lỗi %lu\n", GetLastError()); return 1; }

    /* module chính */
    HMODULE mods[1024]; DWORD need = 0, n = 0;
    if (EnumProcessModulesEx(h, mods, sizeof mods, &need, LIST_MODULES_32BIT)) n = need / sizeof(HMODULE);
    logf("%lu module\n", n);
    for (DWORD i = 0; i < n; i++) {
        char name[MAX_PATH] = {0};
        GetModuleBaseNameA(h, mods[i], name, MAX_PATH);
        MODULEINFO mi = {0};
        GetModuleInformation(h, mods[i], &mi, sizeof mi);
        char out[MAX_PATH];
        snprintf(out, sizeof out, "%s\\mod_%08x_%s.bin", dir, (unsigned)(UINT_PTR)mods[i], name);
        SIZE_T got = 0;
        void *buf = malloc(mi.SizeOfImage ? mi.SizeOfImage : 1);
        if (buf && ReadProcessMemory(h, mi.lpBaseOfDll, buf, mi.SizeOfImage, &got) && got == mi.SizeOfImage) {
            FILE *f = fopen(out, "wb"); if (f) { fwrite(buf, 1, mi.SizeOfImage, f); fclose(f); }
        }
        free(buf);
        logf("  module %-20s base=%p size=0x%x\n", name, mi.lpBaseOfDll, (unsigned)mi.SizeOfImage);
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
