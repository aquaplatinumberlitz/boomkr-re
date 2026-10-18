/* krhookpro.exe — prologue hook + trampoline (bắt MỌI caller, kể cả trong DLL).
 *
 * Tại entry hàm F: ghi đè 5 byte bằng `E9 rel32` -> stub.
 * Trampoline = 5 byte gốc + `E9 rel32` về F+5.
 * Chỉ vá khi 5 byte đầu là ranh giới instruction hợp lệ (kiểm bằng bảng mẫu prologue).
 *
 * usage: krhookpro.exe <exe|pid> <outdir> [giay] <dll> <func>
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

static DWORD find_pid(const char *a) {
    if (a[0] >= '0' && a[0] <= '9') return (DWORD)strtoul(a, NULL, 10);
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe; pe.dwSize = sizeof pe; DWORD pid = 0;
    if (Process32First(s, &pe)) do { if (_stricmp(pe.szExeFile, a) == 0) { pid = pe.th32ProcessID; break; } } while (Process32Next(s, &pe));
    CloseHandle(s); return pid;
}

static DWORD mod_base(DWORD pid, const char *n) {
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32 me; me.dwSize = sizeof me; DWORD b = 0;
    if (Module32First(s, &me)) do { if (_stricmp(me.szModule, n) == 0) { b = (DWORD)(UINT_PTR)me.modBaseAddr; break; } } while (Module32Next(s, &me));
    CloseHandle(s); return b;
}

int main(int argc, char **argv) {
    if (argc < 6) { printf("usage: krhookpro.exe <exe|pid> <outdir> <giay> <dll> <func> [N]\n"); return 2; }
    int seconds = atoi(argv[3]);
    int NBYTES = argc > 6 ? atoi(argv[6]) : 0;   /* 0 = chỉ xử lý forwarder */
    const char *dll = argv[4], *fn = argv[5];
    CreateDirectoryA(argv[2], NULL);
    char path[MAX_PATH]; snprintf(path, sizeof path, "%s\\hookpro.log", argv[2]);
    log_ = fopen(path, "w");

    /* vòng lặp: hook mọi process CA.exe mới xuất hiện, rồi poll sink */
    struct { DWORD pid, slot, page; HANDLE h; } T[16]; int nt = 0;
    DWORD deadline = GetTickCount() + seconds * 1000;
    while (GetTickCount() < deadline) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 pe; pe.dwSize = sizeof pe;
        if (Process32First(snap, &pe)) do {
            if (_stricmp(pe.szExeFile, "CA.exe") != 0) continue;
            int known = 0;
            for (int i = 0; i < nt; i++) if (T[i].pid == pe.th32ProcessID) known = 1;
            if (known || nt >= 16) continue;
            HANDLE h = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE |
                                   PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
            if (!h) continue;
            HMODULE m = LoadLibraryA(dll);
            FARPROC p = m ? GetProcAddress(m, fn) : NULL;
            DWORD base = mod_base(pe.th32ProcessID, dll);
            if (!p || !base) { CloseHandle(h); continue; }
            DWORD F = base + (DWORD)((UINT_PTR)p - (UINT_PTR)m);
            unsigned char orig[8] = {0}; SIZE_T g = 0;
            if (!ReadProcessMemory(h, (void *)(UINT_PTR)F, orig, 8, &g)) { CloseHandle(h); continue; }
            int is_fwd = (orig[0] == 0xFF && orig[1] == 0x25);
            if (!is_fwd && NBYTES < 5) { CloseHandle(h); continue; }
            DWORD slot = 0, real = 0;
            if (is_fwd) {
                slot = *(DWORD *)(orig + 2);
                if (!ReadProcessMemory(h, (void *)(UINT_PTR)slot, &real, 4, &g)) { CloseHandle(h); continue; }
            }
            void *pg = VirtualAllocEx(h, NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (!pg) { CloseHandle(h); continue; }
            DWORD pv = (DWORD)(UINT_PTR)pg;
            unsigned char blob[0x400]; memset(blob, 0, sizeof blob);
            int n = 0; unsigned char *s2 = blob + 0x200;
            DWORD stubAddr;
            if (is_fwd) {
                *(DWORD *)(blob + 0x100) = real;
            s2[n++] = 0x60;
            s2[n++] = 0x8B; s2[n++] = 0x44; s2[n++] = 0x24; s2[n++] = 0x20;
            s2[n++] = 0xA3; *(DWORD *)(s2 + n) = pv + 0; n += 4;
            s2[n++] = 0x8B; s2[n++] = 0x44; s2[n++] = 0x24; s2[n++] = 0x24;
            s2[n++] = 0xA3; *(DWORD *)(s2 + n) = pv + 4; n += 4;
            s2[n++] = 0x8B; s2[n++] = 0x44; s2[n++] = 0x24; s2[n++] = 0x28;
            s2[n++] = 0xA3; *(DWORD *)(s2 + n) = pv + 8; n += 4;
            s2[n++] = 0x8B; s2[n++] = 0x44; s2[n++] = 0x24; s2[n++] = 0x2C;
            s2[n++] = 0xA3; *(DWORD *)(s2 + n) = pv + 12; n += 4;
            s2[n++] = 0x61;
            s2[n++] = 0xFF; s2[n++] = 0x25; *(DWORD *)(s2 + n) = pv + 0x100; n += 4;
                stubAddr = pv + 0x200;
                SIZE_T wr0 = 0;
                if (!WriteProcessMemory(h, pg, blob, sizeof blob, &wr0)) { CloseHandle(h); continue; }
                DWORD oldp0 = 0;
                VirtualProtectEx(h, (void *)(UINT_PTR)slot, 4, PAGE_EXECUTE_READWRITE, &oldp0);
                if (!WriteProcessMemory(h, (void *)(UINT_PTR)slot, &stubAddr, 4, &wr0)) { CloseHandle(h); continue; }
                VirtualProtectEx(h, (void *)(UINT_PTR)slot, 4, oldp0, &oldp0);
                L("hook(fwd) pid=%lu slot=0x%lx real=0x%lx -> stub 0x%lx", pe.th32ProcessID, slot, real, stubAddr);
            } else {
                /* trampoline: copy N byte gốc + jmp F+N */
                DWORD tramp = pv + 0x100;
                memcpy(blob + 0x100, orig, NBYTES);
                blob[0x100 + NBYTES] = 0xE9;
                *(DWORD *)(blob + 0x100 + NBYTES + 1) = (F + NBYTES) - (tramp + NBYTES + 5);
                s2[n++] = 0x60;
                s2[n++] = 0x8B; s2[n++] = 0x44; s2[n++] = 0x24; s2[n++] = 0x20;
                s2[n++] = 0xA3; *(DWORD *)(s2 + n) = pv + 0; n += 4;
                s2[n++] = 0x8B; s2[n++] = 0x44; s2[n++] = 0x24; s2[n++] = 0x24;
                s2[n++] = 0xA3; *(DWORD *)(s2 + n) = pv + 4; n += 4;
                s2[n++] = 0x8B; s2[n++] = 0x44; s2[n++] = 0x24; s2[n++] = 0x28;
                s2[n++] = 0xA3; *(DWORD *)(s2 + n) = pv + 8; n += 4;
                s2[n++] = 0x8B; s2[n++] = 0x44; s2[n++] = 0x24; s2[n++] = 0x2C;
                s2[n++] = 0xA3; *(DWORD *)(s2 + n) = pv + 12; n += 4;
                s2[n++] = 0x61;
                s2[n++] = 0xE9; *(DWORD *)(s2 + n) = tramp - (pv + 0x200 + n + 4); n += 4;
                stubAddr = pv + 0x200;
                SIZE_T wr1 = 0;
                if (!WriteProcessMemory(h, pg, blob, sizeof blob, &wr1)) { CloseHandle(h); continue; }
                unsigned char patch[16]; memset(patch, 0x90, sizeof patch);
                patch[0] = 0xE9;
                *(DWORD *)(patch + 1) = stubAddr - (F + 5);
                DWORD oldp1 = 0;
                VirtualProtectEx(h, (void *)(UINT_PTR)F, NBYTES, PAGE_EXECUTE_READWRITE, &oldp1);
                if (!WriteProcessMemory(h, (void *)(UINT_PTR)F, patch, NBYTES, &wr1)) { CloseHandle(h); continue; }
                VirtualProtectEx(h, (void *)(UINT_PTR)F, NBYTES, oldp1, &oldp1);
                FlushInstructionCache(h, (void *)(UINT_PTR)F, NBYTES);
                L("hook(tramp) pid=%lu F=0x%lx N=%d -> stub 0x%lx", pe.th32ProcessID, F, NBYTES, stubAddr);
            }
            T[nt].pid = pe.th32ProcessID; T[nt].slot = F; T[nt].page = pv; T[nt].h = h; nt++;
        } while (Process32Next(snap, &pe));
        CloseHandle(snap);

        for (int i = 0; i < nt; i++) {
            DWORD v[4] = {0, 0, 0, 0}; SIZE_T g2 = 0;
            if (!ReadProcessMemory(T[i].h, (void *)(UINT_PTR)T[i].page, v, sizeof v, &g2)) continue;
            if (!v[0]) continue;
            char buf[260] = {0};
            if (v[2] && v[3] && v[3] < 256 && ReadProcessMemory(T[i].h, (void *)(UINT_PTR)v[2], buf, v[3], &g2))
                for (DWORD k = 0; k < v[3] && k < 255; k++) if (buf[k] == '\r' || buf[k] == '\n') buf[k] = ' ';
            L("%s pid=%lu caller=0x%lx hFile=0x%lx buf=0x%lx len=%lu | %s",
              fn, T[i].pid, v[0], v[1], v[2], v[3], buf);
            DWORD z[4] = {0, 0, 0, 0};
            WriteProcessMemory(T[i].h, (void *)(UINT_PTR)T[i].page, z, sizeof z, &g2);
        }
        Sleep(150);
    }
    L("ket thuc (giu hook)");
    if (log_) fclose(log_);
    return 0;
}
