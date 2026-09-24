/* halo_launch.exe: start the retail halo.exe with halo_rewrite.dll loaded, without modifying any game file.
   Creates the process suspended, loads the DLL into it with a remote LoadLibraryA, then resumes the main thread.
   Usage: halo_launch.exe [halo arguments, e.g. -window -novideo]
   Looks for halo.exe in the current directory, else in C:\Program Files (x86)\Microsoft Games\Halo, and for the DLL
   next to this launcher. Build (x86): cl /nologo /O2 launcher.c /Fe:halo_launch.exe */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static void fail(const char *what)
{
    char msg[512];
    sprintf(msg, "halo_launch: %s failed (error %lu)", what, GetLastError());
    fprintf(stderr, "%s\n", msg); fflush(stderr);
    MessageBoxA(NULL, msg, "halo_launch", MB_ICONERROR);
    ExitProcess(1);
}

int main(int argc, char **argv)
{
    char dll[MAX_PATH], exe[MAX_PATH], dir[MAX_PATH], cmd[4096];
    STARTUPINFOA si; PROCESS_INFORMATION pi;
    void *remote; HANDLE thread; DWORD code = 0; int i; char *slash;

    GetModuleFileNameA(NULL, dll, MAX_PATH);
    slash = strrchr(dll, '\\'); strcpy(slash ? slash + 1 : dll, "halo_rewrite.dll");
    if (GetFileAttributesA(dll) == INVALID_FILE_ATTRIBUTES) fail("finding halo_rewrite.dll next to the launcher");

    GetCurrentDirectoryA(MAX_PATH, dir);
    sprintf(exe, "%s\\halo.exe", dir);
    if (GetFileAttributesA(exe) == INVALID_FILE_ATTRIBUTES) {
        strcpy(dir, "C:\\Program Files (x86)\\Microsoft Games\\Halo");
        sprintf(exe, "%s\\halo.exe", dir);
    }
    sprintf(cmd, "\"%s\"", exe);
    for (i = 1; i < argc; i++) { strcat(cmd, " "); strcat(cmd, argv[i]); }

    ZeroMemory(&si, sizeof si); si.cb = sizeof si;
    printf("halo_launch: %s\n  dll %s\n", cmd, dll); fflush(stdout);
    if (!CreateProcessA(exe, cmd, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, dir, &si, &pi)) fail("starting halo.exe");

    remote = VirtualAllocEx(pi.hProcess, NULL, strlen(dll) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote || !WriteProcessMemory(pi.hProcess, remote, dll, strlen(dll) + 1, NULL)) fail("writing the DLL path");
    thread = CreateRemoteThread(pi.hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"), remote, 0, NULL);
    if (!thread) fail("CreateRemoteThread(LoadLibraryA)");
    WaitForSingleObject(thread, INFINITE);
    GetExitCodeThread(thread, &code);          /* the module handle, or 0 */
    if (!code) { TerminateProcess(pi.hProcess, 1); fail("LoadLibraryA of halo_rewrite.dll inside halo.exe"); }
    printf("halo_launch: halo_rewrite.dll loaded at 0x%08lx, resuming halo.exe\n", code);

    ResumeThread(pi.hThread);
    CloseHandle(thread); CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    return 0;
}
