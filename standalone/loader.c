/* Standalone halo.exe loader (first-boot track).
   Our exe links far from 0x400000 and runs no original code. Halo's globals are referenced by absolute address
   (the EQU symbols tools/gen_standalone_link.py emits), so before any game code runs this loader puts halo.exe's
   data sections back where they were:
     1. The first instance starts a suspended copy of itself and reserves 0x400000..0x891000 in it before its
        loader initialises (the same trick harness/difftest_main.c uses), so no heap or DLL can land there.
     2. The child commits the range, copies halo_image.bin (the .rdata/.data/.tls/.rsrc raw bytes written by
        tools/gen_standalone.py) to the recorded addresses, fills both import tables (normal and delay-load) with
        GetProcAddress, and writes our C functions' addresses into every code pointer stored in that data.
     3. It changes to the Halo install folder (maps\, binkw32.dll, vorbis.dll live there) and calls the rewritten
        shell_winmain, as the original CRT entry did.
   A call to a function we have no C for lands in standalone_missing_function (the trap stubs in
   build/standalone/resolve.asm), which logs the name to halo_standalone.log and exits. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "standalone_tables.h"

#define RESERVE_BASE 0x400000
#define RESERVE_END  0x891000

extern int __stdcall shell_winmain(void *hInstance, void *hPrevInstance, char *lpCmdLine, int nCmdShow);

static FILE *g_log;
static char g_exe_dir[MAX_PATH];

static void log_line(const char *fmt, ...)
{
    va_list ap;
    if (!g_log) {
        char path[MAX_PATH];
        _snprintf(path, sizeof path, "%s\\halo_standalone.log", g_exe_dir);
        g_log = fopen(path, "w");
        if (!g_log) return;
    }
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fputc('\n', g_log);
    fflush(g_log);
}

/* every trap stub pushes its symbol name and calls this */
void __cdecl standalone_missing_function(const char *name)
{
    char text[512];
    log_line("MISSING FUNCTION called: %s (no C rewrite linked; see build/standalone/traps.txt)", name);
    _snprintf(text, sizeof text, "Called a function with no C rewrite yet:\n%s\n\nSee halo_standalone.log.", name);
    if (!GetEnvironmentVariableA("HALO_STANDALONE_NOBOX", NULL, 0))
        MessageBoxA(NULL, text, "Halo standalone", MB_OK | MB_ICONERROR);
    ExitProcess(3);
}

static void unresolved_import_trap(void)
{
    log_line("an import that could not be resolved was called");
    MessageBoxA(NULL, "An unresolved import was called; see halo_standalone.log.", "Halo standalone", MB_OK | MB_ICONERROR);
    ExitProcess(4);
}

/* First-boot diagnostics: every exception (first chance, so the game's own handlers still run) is logged with EIP,
   the fault address and the return addresses on the stack that fall inside this exe; build/standalone/halo_rebuilt.map
   turns them into function names (tools/standalone_symbolize.py). A jump into the original .text range lands on
   a non-executable page (map_image), so "0xc0000005 at eip 0x4xxxxx" names an original function nothing replaced. */
static unsigned long g_code_begin, g_code_end;
static int g_exceptions_logged;
static volatile LONG g_in_handler;

static LONG CALLBACK log_exception(EXCEPTION_POINTERS *info)
{
    EXCEPTION_RECORD *r = info->ExceptionRecord;
    CONTEXT *c = info->ContextRecord;
    unsigned long *sp, *top;
    int i, n = 0;
    char frames[512];
    int used = 0;

    if (c->Esp < (DWORD)((NT_TIB *)NtCurrentTeb())->StackLimit || c->Esp >= (DWORD)((NT_TIB *)NtCurrentTeb())->StackBase)
        return EXCEPTION_CONTINUE_SEARCH;
    if (r->ExceptionCode == 0x406D1388 || r->ExceptionCode == DBG_PRINTEXCEPTION_C) return EXCEPTION_CONTINUE_SEARCH;
    if (g_exceptions_logged >= 25 || InterlockedExchange(&g_in_handler, 1)) return EXCEPTION_CONTINUE_SEARCH;
    g_exceptions_logged++;
    frames[0] = 0;
    sp = (unsigned long *)c->Esp;
    top = (unsigned long *)((NT_TIB *)NtCurrentTeb())->StackBase;   /* no IsBadReadPtr: it raises its own AVs */
    __try {
        for (i = 0; i < 2048 && n < 16 && sp + i < top; i++) {
            unsigned long v = sp[i];
            if (v >= g_code_begin && v < g_code_end) {
                used += _snprintf(frames + used, sizeof frames - used, " %08lx", v);
                n++;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        /* a guard page left mid-stack by a large unprobed frame: keep what was read */
    }
    log_line("EXCEPTION 0x%08lx at eip %08lx (fault address %08lx) eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx "
             "edi=%08lx ebp=%08lx esp=%08lx | stack:%s",
             r->ExceptionCode, c->Eip, r->NumberParameters > 1 ? (unsigned long)r->ExceptionInformation[1] : 0,
             c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp, frames);
    g_in_handler = 0;
    return EXCEPTION_CONTINUE_SEARCH;
}

static void install_diagnostics(void)
{
    unsigned char *base = (unsigned char *)GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    g_code_begin = (unsigned long)base + nt->OptionalHeader.BaseOfCode;
    g_code_end = g_code_begin + nt->OptionalHeader.SizeOfCode;
    AddVectoredExceptionHandler(1, log_exception);
}

static int run_reserved_child(void)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD code = 1;
    HANDLE job;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION li;

    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    SetEnvironmentVariableA("HALO_STANDALONE_CHILD", "1");
    if (!CreateProcessA(NULL, GetCommandLineA(), NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        log_line("CreateProcess failed (%lu)", GetLastError());
        return 1;
    }
    job = CreateJobObjectA(NULL, NULL);
    ZeroMemory(&li, sizeof li);
    li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (job) {
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &li, sizeof li);
        AssignProcessToJobObject(job, pi.hProcess);
    }
    if (!VirtualAllocEx(pi.hProcess, (void *)RESERVE_BASE, RESERVE_END - RESERVE_BASE, MEM_RESERVE, PAGE_EXECUTE_READWRITE)) {
        log_line("could not reserve 0x%x..0x%x in the child (%lu)", RESERVE_BASE, RESERVE_END, GetLastError());
    }
    ResumeThread(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    return (int)code;
}

static int map_image(void)
{
    char path[MAX_PATH];
    FILE *f;
    long size;
    unsigned char *blob;
    DWORD old_protect;
    int i;

    if (!VirtualAlloc((void *)RESERVE_BASE, RESERVE_END - RESERVE_BASE, MEM_COMMIT, PAGE_EXECUTE_READWRITE)) {
        log_line("could not commit 0x%x..0x%x (%lu) -- was the range reserved?", RESERVE_BASE, RESERVE_END,
                 GetLastError());
        return 0;
    }
    _snprintf(path, sizeof path, "%s\\halo_image.bin", g_exe_dir);
    f = fopen(path, "rb");
    if (!f) {
        log_line("cannot open %s", path);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    blob = (unsigned char *)malloc(size);
    if (!blob || fread(blob, 1, size, f) != (size_t)size) {
        log_line("cannot read %s", path);
        fclose(f);
        return 0;
    }
    fclose(f);
    for (i = 0; i < standalone_piece_count; i++) {
        const standalone_piece *p = &standalone_pieces[i];
        if (p->blob_offset + p->raw_size > (unsigned long)size) {
            log_line("halo_image.bin is shorter than %s needs", p->name);
            return 0;
        }
        memcpy((void *)p->va, blob + p->blob_offset, p->raw_size);   /* the rest of the committed range stays zero */
    }
    free(blob);
    /* nothing in the range is code any more: without execute permission a jump into original code faults at once
       (DEP is on for this exe: /NXCOMPAT) and log_exception names the address */
    VirtualProtect((void *)RESERVE_BASE, RESERVE_END - RESERVE_BASE, PAGE_READWRITE, &old_protect);
    log_line("mapped %d image pieces (range now read/write, no execute)", standalone_piece_count);
    return 1;
}

static void fill_imports(void)
{
    int i, missing = 0;
    for (i = 0; i < standalone_import_count; i++) {
        const standalone_import *im = &standalone_imports[i];
        HMODULE h = LoadLibraryA(im->dll);
        FARPROC p = 0;
        if (h) p = GetProcAddress(h, im->name ? im->name : (const char *)(ULONG_PTR)im->ordinal);
        if (!p) {
            log_line("import not resolved: %s!%s", im->dll, im->name ? im->name : "(ordinal)");
            p = (FARPROC)unresolved_import_trap;
            missing++;
        }
        *(FARPROC *)im->slot = p;
        if (im->module_handle_slot && h) *(HMODULE *)im->module_handle_slot = h;
    }
    log_line("filled %d import slots (%d unresolved)", standalone_import_count, missing);
}

static void fix_code_pointers(void)
{
    int i;
    for (i = 0; i < standalone_code_pointer_count; i++) {
        *(void **)standalone_code_pointers[i].slot = standalone_code_pointers[i].target;
    }
    log_line("redirected %d code pointers to C", standalone_code_pointer_count);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show)
{
    char *slash;
    GetModuleFileNameA(NULL, g_exe_dir, sizeof g_exe_dir);
    slash = strrchr(g_exe_dir, '\\');
    if (slash) *slash = 0;

    if (!GetEnvironmentVariableA("HALO_STANDALONE_CHILD", NULL, 0)) {
        return run_reserved_child();
    }
    log_line("halo standalone: child started");
    install_diagnostics();
    if (!map_image()) return 2;
    SetDllDirectoryA(standalone_halo_folder);
    fill_imports();
    fix_code_pointers();
    if (!SetCurrentDirectoryA(standalone_halo_folder)) {
        log_line("cannot change to the Halo folder %s", standalone_halo_folder);
    }
    log_line("calling shell_winmain");
    return shell_winmain(instance, previous, command_line, show);
}
