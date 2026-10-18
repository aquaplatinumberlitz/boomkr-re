/* krlaunch.exe — chạy CA.exe (KR) ở trạng thái suspended, inject krhook.dll, rồi resume.
 *
 *   krlaunch.exe <đường dẫn CA.exe> [đường dẫn krhook.dll]
 *
 * Không dùng AppInit_DLLs để tránh ảnh hưởng client VN trong cùng máy ảo.
 */
#include <windows.h>
#include <stdio.h>

int main(int argc, char **argv) {
    char dll[MAX_PATH], cmd[MAX_PATH + 8];
    if (argc < 2) { printf("usage: krlaunch.exe <exe> [dll]\n"); return 2; }
    if (argc >= 3) lstrcpynA(dll, argv[2], MAX_PATH);
    else {
        GetModuleFileNameA(NULL, dll, MAX_PATH);
        char *p = strrchr(dll, '\\'); if (p) *p = 0;
        lstrcatA(dll, "\\krhook.dll");
    }
    lstrcpynA(cmd, argv[1], MAX_PATH - 8);
    lstrcatA(cmd, " ");
    lstrcatA(cmd, "");

    /* CWD = thư mục chứa exe (client tìm DLL theo CWD) */
    char dir[MAX_PATH];
    lstrcpynA(dir, argv[1], MAX_PATH);
    char *slash = strrchr(dir, '\\');
    if (slash) *slash = 0;

    STARTUPINFOA si = {0}; si.cb = sizeof si;
    PROCESS_INFORMATION pi = {0};
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, dir, &si, &pi)) {
        printf("CreateProcess lỗi %lu\n", GetLastError());
        return 1;
    }
    SIZE_T n = lstrlenA(dll) + 1;
    LPVOID mem = VirtualAllocEx(pi.hProcess, NULL, n, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem || !WriteProcessMemory(pi.hProcess, mem, dll, n, NULL)) {
        printf("WriteProcessMemory lỗi %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    LPTHREAD_START_ROUTINE pLoad = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryA");
    HANDLE th = CreateRemoteThread(pi.hProcess, NULL, 0, pLoad, mem, 0, NULL);
    if (!th) {
        printf("CreateRemoteThread lỗi %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    WaitForSingleObject(th, 5000);
    DWORD loaded = 0;
    GetExitCodeThread(th, &loaded);
    printf("LoadLibraryA -> %s (0x%lx)\n", loaded ? "OK" : "THẤT BẠI", (unsigned long)loaded);
    CloseHandle(th);
    ResumeThread(pi.hThread);
    printf("đã resume pid=%lu\n", (unsigned long)pi.dwProcessId);
    WaitForSingleObject(pi.hProcess, 20000);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    printf("kết thúc, exitcode=0x%lx\n", (unsigned long)code);
    return 0;
}
