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
#include <stdlib.h>
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

/* for the other standalone support files (d3dx_compat.c) */
void __cdecl standalone_log(const char *format, ...)
{
    char text[1024];
    va_list ap;
    va_start(ap, format);
    _vsnprintf(text, sizeof text - 1, format, ap);
    va_end(ap);
    text[sizeof text - 1] = 0;
    log_line("%s", text);
}

/* every trap stub pushes its symbol name and calls this */
void __cdecl standalone_missing_function(const char *name)
{
    char text[512];
    void *frames[16];
    char trace[16 * 9 + 1];
    USHORT count = RtlCaptureStackBackTrace(1, 16, frames, NULL), i;
    int used = 0;

    trace[0] = 0;
    for (i = 0; i < count && used < (int)sizeof trace - 9; i++)
        used += _snprintf(trace + used, sizeof trace - used, " %08lx", (unsigned long)(ULONG_PTR)frames[i]);
    log_line("MISSING FUNCTION called: %s (no C rewrite linked; see build/standalone/traps.txt) | stack:%s", name, trace);
    _snprintf(text, sizeof text, "Called a function with no C rewrite yet:\n%s\n\nSee halo_standalone.log.", name);
    if (!GetEnvironmentVariableA("HALO_STANDALONE_NOBOX", NULL, 0))
        MessageBoxA(NULL, text, "Halo standalone", MB_OK | MB_ICONERROR);
    TerminateProcess(GetCurrentProcess(), 3);   /* no DLL detach: some third-party DLLs crash there */
}

/* Developer mode (a standalone extension; retail 01.00.10 has no -devmode switch): lifts the console's command
   availability filter so every script function and global (cheats included) can be run from the in-game console.
   On with "-devmode" on the command line or HALO_DEVMODE in the environment. Read by console_process_command. */
int standalone_devmode(void)
{
    static int cached = -1;

    if (cached < 0) {
        const char *line = GetCommandLineA();

        cached = (line && strstr(line, "-devmode")) || GetEnvironmentVariableA("HALO_DEVMODE", NULL, 0) ? 1 : 0;
    }
    return cached;
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

/* A jump into original .text (the image is mapped non-executable) at the start of a function we rewrote: continue in
   our C instead. This covers constant function addresses baked into code -- a stable rewrite that passes 0x511f20
   as a callback is right in the harness, where the original code runs, and lands here in the standalone. Arguments
   and return address are untouched, so a cdecl/stdcall rewrite picks up exactly what the original would have. */
static LONG CALLBACK redirect_original_entry(EXCEPTION_POINTERS *info)
{
    EXCEPTION_RECORD *r = info->ExceptionRecord;
    CONTEXT *c = info->ContextRecord;
    int lo = 0, hi = standalone_code_entry_count - 1;

    if (r->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || r->NumberParameters < 2 ||
        r->ExceptionInformation[0] != 8 || r->ExceptionInformation[1] != c->Eip)
        return EXCEPTION_CONTINUE_SEARCH;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        unsigned long a = standalone_code_entries[mid].address;
        if (a == c->Eip) {
            static int logged;
            if (!standalone_code_entries[mid].target) break;
            if (logged < 20) {
                logged++;
                log_line("redirected a jump into original code at %08lx to its C rewrite", c->Eip);
            }
            c->Eip = (DWORD)standalone_code_entries[mid].target;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        if (a < c->Eip) lo = mid + 1; else hi = mid - 1;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void install_diagnostics(void)
{
    unsigned char *base = (unsigned char *)GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    g_code_begin = (unsigned long)base + nt->OptionalHeader.BaseOfCode;
    g_code_end = g_code_begin + nt->OptionalHeader.SizeOfCode;
    AddVectoredExceptionHandler(1, log_exception);
    AddVectoredExceptionHandler(1, redirect_original_entry); /* first: before the diagnostic log */
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

/* Data overrides: a relative path opened through CreateFileA that has a copy under <exe folder>\override\ opens
   that copy instead. The standalone build uses it for shadersx.bin, converted by tools/convert_fx.py from the 2003
   compiled-effect format (which only the D3DX statically linked into halo.exe could read) to fx_2_0 for d3dx9_43;
   the Halo install itself is never modified. */
typedef HANDLE (WINAPI *create_file_a_fn)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static create_file_a_fn g_create_file_a;

static HANDLE WINAPI standalone_create_file_a(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security,
                                              DWORD disposition, DWORD flags, HANDLE template_file)
{
    char path[MAX_PATH];
    if (name && name[0] && name[0] != '\\' && name[1] != ':' &&
        _snprintf(path, sizeof path, "%s\\override\\%s", g_exe_dir, name) > 0 &&
        GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
        log_line("override: %s -> %s", name, path);
        name = path;
    }
    return g_create_file_a(name, access, share, security, disposition, flags, template_file);
}

/* The Halo folder holds the hook harness's dinput8.dll proxy (it loads halo_rewrite.dll). The folder has to be on
   the DLL path for binkw32/vorbis/Keystone, so load the system dinput8.dll by full path first: later
   LoadLibraryA("DINPUT8.dll") calls then return that module instead of the proxy. */
static void preload_system_dinput8(void)
{
    char path[MAX_PATH];
    UINT n = GetSystemDirectoryA(path, MAX_PATH - 16);
    HMODULE h;

    if (n == 0 || n >= MAX_PATH - 16) return;
    strcat(path, "\\dinput8.dll");
    h = LoadLibraryA(path);
    log_line("preloaded %s: %s", path, h ? "ok" : "FAILED");
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
        if (im->name && strcmp(im->name, "CreateFileA") == 0 && p != (FARPROC)unresolved_import_trap) {
            g_create_file_a = (create_file_a_fn)p;
            p = (FARPROC)standalone_create_file_a;
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

/* The original CRT startup (mainCRTStartup) set some of its own globals before WinMain; the rewritten C reads
   a few of them, so the loader does the same work:
     __setargv 0x631e08: GetModuleFileNameA(NULL, _pgmname 0x006a3738, 0x104), _pgmname[0x104] = 0,
                         _pgmptr 0x006a32e8 = _pgmname (shell_check_previous_run_crash reads it) */
static void emulate_crt_startup(void)
{
    char *pgmname = (char *)0x006a3738;
    GetModuleFileNameA(NULL, pgmname, 0x104);
    pgmname[0x104] = 0;
    *(char **)0x006a32e8 = pgmname;
}

/* Test input driver (opt-in, first-boot testing only). Keystrokes injected from another process do not reach the
   game in the environment the tools run in, but keystrokes injected from inside the process do, so with
   HALO_STANDALONE_KEYS="delay_ms:KEY,delay_ms:KEY*count,..." a thread in this process brings the game window to the
   front and presses the keys by scan code through the normal OS path (window messages and DirectInput both see
   them). KEY: ENTER ESC UP DOWN LEFT RIGHT SPACE TAB W A S D E F G Q R X, or "KEY~ms" to hold a key for ms
   milliseconds (e.g. 1000:W~2000 walks forward for 2 s). Mouse: MUP MDOWN MLEFT MRIGHT move the mouse 40 counts
   per repeat (MUP*10), FIRE clicks the left button (FIRE~ms holds it). Each press is logged. */
static char g_key_script[1024];

static BOOL CALLBACK find_game_window(HWND hwnd, LPARAM out)
{
    DWORD pid = 0;
    char cls[64];
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId() && IsWindowVisible(hwnd) && GetClassNameA(hwnd, cls, sizeof cls) &&
        strcmp(cls, "Halo") == 0) {
        *(HWND *)out = hwnd;
        return FALSE;
    }
    return TRUE;
}

static void bring_to_front(HWND hwnd)
{
    DWORD foreground = GetWindowThreadProcessId(GetForegroundWindow(), NULL), me = GetCurrentThreadId();
    AttachThreadInput(me, foreground, TRUE);
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    AttachThreadInput(me, foreground, FALSE);
}

static DWORD WINAPI key_driver_thread(void *unused)
{
    static const struct { const char *name; BYTE scan; BOOL extended; } keys[] = {
        {"ENTER", 0x1c, FALSE}, {"ESC", 0x01, FALSE}, {"UP", 0x48, TRUE}, {"DOWN", 0x50, TRUE},
        {"LEFT", 0x4b, TRUE}, {"RIGHT", 0x4d, TRUE}, {"SPACE", 0x39, FALSE}, {"TAB", 0x0f, FALSE},
        {"W", 0x11, FALSE}, {"A", 0x1e, FALSE}, {"S", 0x1f, FALSE}, {"D", 0x20, FALSE}, {"E", 0x12, FALSE},
        {"F", 0x21, FALSE}, {"G", 0x22, FALSE}, {"Q", 0x10, FALSE}, {"R", 0x13, FALSE}, {"X", 0x2d, FALSE}};
    static const struct { const char *name; int dx, dy; } moves[] = {
        {"MUP", 0, -40}, {"MDOWN", 0, 40}, {"MLEFT", -40, 0}, {"MRIGHT", 40, 0}};
    char *step = g_key_script, *next;
    for (; step && *step; step = next) {
        char name[32] = {0};
        int delay = 0, count = 1, hold = 100, i, k, n = 0;
        const char *tail;
        HWND hwnd = NULL;
        next = strchr(step, ',');
        if (next) *next++ = 0;
        if (sscanf(step, "%d:%31[A-Z]%n", &delay, name, &n) < 2) continue;
        tail = step + n;
        if (*tail == '*') count = atoi(tail + 1);
        else if (*tail == '~') hold = atoi(tail + 1);
        Sleep(delay);
        EnumWindows(find_game_window, (LPARAM)&hwnd);
        if (hwnd) bring_to_front(hwnd);
        Sleep(200);
        for (k = 0; k < (int)(sizeof moves / sizeof moves[0]); k++) {
            if (strcmp(moves[k].name, name) != 0) continue;
            for (i = 0; i < count; i++) {
                mouse_event(MOUSEEVENTF_MOVE, (DWORD)moves[k].dx, (DWORD)moves[k].dy, 0, 0);
                Sleep(30);
            }
            log_line("key driver: mouse %s x%d", name, count);
        }
        if (strcmp(name, "FIRE") == 0) {
            for (i = 0; i < count; i++) {
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                Sleep(hold);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                Sleep(150);
            }
            log_line("key driver: fire x%d (hold %d ms)", count, hold);
        }
        for (k = 0; k < (int)(sizeof keys / sizeof keys[0]); k++) {
            if (strcmp(keys[k].name, name) != 0) continue;
            for (i = 0; i < count; i++) {
                DWORD flags = KEYEVENTF_SCANCODE | (keys[k].extended ? KEYEVENTF_EXTENDEDKEY : 0);
                keybd_event(0, keys[k].scan, flags, 0);
                Sleep(hold);
                keybd_event(0, keys[k].scan, flags | KEYEVENTF_KEYUP, 0);
                Sleep(150);
            }
            log_line("key driver: pressed %s x%d hold %d ms (window %p, foreground %s)", name, count, hold,
                     (void *)hwnd, GetForegroundWindow() == hwnd ? "yes" : "NO");
        }
    }
    return 0;
}

static void start_key_driver(void)
{
    if (GetEnvironmentVariableA("HALO_STANDALONE_KEYS", g_key_script, sizeof g_key_script) &&
        g_key_script[0]) {
        CreateThread(NULL, 0, key_driver_thread, NULL, 0, NULL);
        log_line("key driver: script %s", g_key_script);
    }
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
    preload_system_dinput8();
    SetDllDirectoryA(standalone_halo_folder);
    fill_imports();
    fix_code_pointers();
    emulate_crt_startup();
    if (!SetCurrentDirectoryA(standalone_halo_folder)) {
        log_line("cannot change to the Halo folder %s", standalone_halo_folder);
    }
    start_key_driver();
    log_line("calling shell_winmain");
    return shell_winmain(instance, previous, command_line, show);
}
