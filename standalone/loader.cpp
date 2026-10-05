/**
 * Standalone halo.exe loader.
 *
 * The game is C++ linked into this exe: its globals are extern "C" objects and the data tables are ordinary arrays
 * (standalone/data/tables.cpp) whose internal pointers are linker relocations. The loader installs the diagnostics,
 * preloads the system dinput8, redirects data files to override\ where present, changes to the Halo install folder
 * (maps\ lives there) and calls the rewritten shell_winmain, as the original CRT entry did.
 *
 * The names other files link against (standalone_log, standalone_devmode, standalone_halo_folder, WinMain) are
 * declared in the single extern "C" block below; everything else lives in halo::standalone.
 */
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "halo/shell/api.hpp"

#ifndef HALO_FOLDER
#define HALO_FOLDER "C:\\Program Files (x86)\\Microsoft Games\\Halo"
#endif

extern "C" {

extern char *shell_module_path;

extern const char standalone_halo_folder[];
void __cdecl standalone_log(const char *format, ...);
int standalone_devmode(void);
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show);
int standalone_data_layout_check(void);

}

namespace halo::standalone {

namespace {

FILE *g_log;
char g_exe_dir[MAX_PATH];

/**
 * Appends one formatted line to halo_standalone.log next to the exe. A relaunched child appends to what the first
 * instance logged.
 */
void log_line(const char *fmt, ...)
{
    va_list ap;
    if (!g_log) {
        char path[MAX_PATH];
        _snprintf(path, sizeof path, "%s\\halo_standalone.log", g_exe_dir);
        g_log = fopen(path, GetEnvironmentVariableA("HALO_STANDALONE_RELAUNCHED", nullptr, 0) ? "a" : "w");
        if (!g_log) return;
    }
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fputc('\n', g_log);
    fflush(g_log);
}

unsigned long g_code_begin, g_code_end;
int g_exceptions_logged;
volatile LONG g_in_handler;

/**
 * First-boot diagnostics: every exception (first chance, so the game's own handlers still run) is logged with EIP,
 * the fault address and the return addresses on the stack that fall inside this exe; build/standalone/halo_rebuilt.map
 * turns them into function names (tools/standalone_symbolize.py).
 */
LONG CALLBACK log_exception(EXCEPTION_POINTERS *info)
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
    top = (unsigned long *)((NT_TIB *)NtCurrentTeb())->StackBase;
    __try {
        for (i = 0; i < 2048 && n < 16 && sp + i < top; i++) {
            unsigned long v = sp[i];
            if (v >= g_code_begin && v < g_code_end) {
                used += _snprintf(frames + used, sizeof frames - used, " %08lx", v);
                n++;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    log_line("EXCEPTION 0x%08lx at eip %08lx (fault address %08lx) eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx "
             "edi=%08lx ebp=%08lx esp=%08lx | stack:%s",
             r->ExceptionCode, c->Eip, r->NumberParameters > 1 ? (unsigned long)r->ExceptionInformation[1] : 0,
             c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp, frames);
    g_in_handler = 0;
    return EXCEPTION_CONTINUE_SEARCH;
}

/** Records the exe's code range and installs the exception logger. */
void install_diagnostics()
{
    auto *base = reinterpret_cast<unsigned char *>(GetModuleHandleA(nullptr));
    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(base + reinterpret_cast<IMAGE_DOS_HEADER *>(base)->e_lfanew);
    g_code_begin = reinterpret_cast<unsigned long>(base) + nt->OptionalHeader.BaseOfCode;
    g_code_end = g_code_begin + nt->OptionalHeader.SizeOfCode;
    AddVectoredExceptionHandler(1, log_exception);
}

using create_file_a_fn = HANDLE(WINAPI *)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
create_file_a_fn g_create_file_a;

/**
 * Data overrides: a relative path opened through CreateFileA that has a copy under <exe folder>\override\ opens that
 * copy instead. The standalone build uses it for shadersx.bin, converted by tools/convert_fx.py from the 2003
 * compiled-effect format to fx_2_0 for d3dx9_43; the Halo install itself is never modified.
 */
HANDLE WINAPI standalone_create_file_a(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security,
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

/**
 * The Halo folder holds the hook harness's dinput8.dll proxy (it loads halo_rewrite.dll). The folder has to be on the
 * DLL path for Keystone, so the system dinput8.dll is loaded by full path first: later
 * LoadLibraryA("DINPUT8.dll") calls then return that module instead of the proxy.
 */
void preload_system_dinput8()
{
    char path[MAX_PATH];
    UINT n = GetSystemDirectoryA(path, MAX_PATH - 16);
    HMODULE h;

    if (n == 0 || n >= MAX_PATH - 16) return;
    strcat(path, "\\dinput8.dll");
    h = LoadLibraryA(path);
    log_line("preloaded %s: %s", path, h ? "ok" : "FAILED");
}

/**
 * The C++ calls Windows through this exe's own import table, so the CreateFileA data override sits in this module's
 * import address table. Only relative paths with a copy under override\ are redirected, so the loader's and the
 * CRT's own file opens are unaffected.
 */
void hook_own_create_file_a()
{
    auto *base = reinterpret_cast<unsigned char *>(GetModuleHandleA(nullptr));
    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(base + reinterpret_cast<IMAGE_DOS_HEADER *>(base)->e_lfanew);
    IMAGE_DATA_DIRECTORY *dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    FARPROC real = GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateFileA");
    int hooked = 0;

    if (!real || !dir->VirtualAddress) return;
    g_create_file_a = reinterpret_cast<create_file_a_fn>(real);
    for (auto *d = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(base + dir->VirtualAddress); d->Name; d++) {
        auto *thunk = reinterpret_cast<IMAGE_THUNK_DATA *>(base + d->FirstThunk);
        for (; thunk->u1.Function; thunk++) {
            if (reinterpret_cast<FARPROC>(thunk->u1.Function) == real) {
                DWORD old_protect;
                VirtualProtect(&thunk->u1.Function, sizeof thunk->u1.Function, PAGE_READWRITE, &old_protect);
                thunk->u1.Function = reinterpret_cast<ULONG_PTR>(&standalone_create_file_a);
                VirtualProtect(&thunk->u1.Function, sizeof thunk->u1.Function, old_protect, &old_protect);
                hooked++;
            }
        }
    }
    log_line("CreateFileA override: %d entries in this module's import table", hooked);
}

char g_pgmname[0x105];

/**
 * The original CRT startup (mainCRTStartup) set some of its own globals before WinMain; the rewritten code reads a
 * few of them, so the loader does the same work: __setargv at 0x631e08 stores the module path in _pgmname and
 * _pgmptr, which shell_check_previous_run_crash reads as shell_module_path.
 */
void emulate_crt_startup()
{
    GetModuleFileNameA(nullptr, g_pgmname, 0x104);
    g_pgmname[0x104] = 0;
    shell_module_path = g_pgmname;
}

char g_key_script[1024];

BOOL CALLBACK find_game_window(HWND hwnd, LPARAM out)
{
    DWORD pid = 0;
    char cls[64];
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId() && IsWindowVisible(hwnd) && GetClassNameA(hwnd, cls, sizeof cls) &&
        strcmp(cls, "Halo") == 0) {
        *reinterpret_cast<HWND *>(out) = hwnd;
        return FALSE;
    }
    return TRUE;
}

void bring_to_front(HWND hwnd)
{
    DWORD foreground = GetWindowThreadProcessId(GetForegroundWindow(), nullptr), me = GetCurrentThreadId();
    AttachThreadInput(me, foreground, TRUE);
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    AttachThreadInput(me, foreground, FALSE);
}

struct key_entry {
    const char *name;
    BYTE scan;
    BOOL extended;
};

struct move_entry {
    const char *name;
    int dx, dy;
};

/**
 * Test input driver (opt-in, first-boot testing only). Keystrokes injected from another process do not reach the
 * game in the environment the tools run in, but keystrokes injected from inside the process do, so with
 * HALO_STANDALONE_KEYS="delay_ms:KEY,delay_ms:KEY*count,..." a thread in this process brings the game window to the
 * front and presses the keys by scan code through the normal OS path.
 *
 * KEY: ENTER ESC UP DOWN LEFT RIGHT SPACE TAB W A S D E F G Q R X, or "KEY~ms" to hold a key for ms milliseconds
 * (e.g. 1000:W~2000 walks forward for 2 s). Mouse: MUP MDOWN MLEFT MRIGHT move the mouse 40 counts per repeat
 * (MUP*10), FIRE clicks the left button (FIRE~ms holds it). Each press is logged.
 */
DWORD WINAPI key_driver_thread(void *)
{
    static const key_entry keys[] = {
        {"ENTER", 0x1c, FALSE}, {"ESC", 0x01, FALSE}, {"UP", 0x48, TRUE}, {"DOWN", 0x50, TRUE},
        {"LEFT", 0x4b, TRUE}, {"RIGHT", 0x4d, TRUE}, {"SPACE", 0x39, FALSE}, {"TAB", 0x0f, FALSE},
        {"W", 0x11, FALSE}, {"A", 0x1e, FALSE}, {"S", 0x1f, FALSE}, {"D", 0x20, FALSE}, {"E", 0x12, FALSE},
        {"F", 0x21, FALSE}, {"G", 0x22, FALSE}, {"Q", 0x10, FALSE}, {"R", 0x13, FALSE}, {"X", 0x2d, FALSE}};
    static const move_entry moves[] = {{"MUP", 0, -40}, {"MDOWN", 0, 40}, {"MLEFT", -40, 0}, {"MRIGHT", 40, 0}};
    char *step = g_key_script, *next;
    for (; step && *step; step = next) {
        char name[32] = {0};
        int delay = 0, count = 1, hold = 100, n = 0;
        const char *tail;
        HWND hwnd = nullptr;
        next = strchr(step, ',');
        if (next) *next++ = 0;
        if (sscanf(step, "%d:%31[A-Z]%n", &delay, name, &n) < 2) continue;
        tail = step + n;
        if (*tail == '*') count = atoi(tail + 1);
        else if (*tail == '~') hold = atoi(tail + 1);
        Sleep(delay);
        EnumWindows(find_game_window, reinterpret_cast<LPARAM>(&hwnd));
        if (hwnd) bring_to_front(hwnd);
        Sleep(200);
        for (const move_entry &move : moves) {
            if (strcmp(move.name, name) != 0) continue;
            for (int i = 0; i < count; i++) {
                mouse_event(MOUSEEVENTF_MOVE, (DWORD)move.dx, (DWORD)move.dy, 0, 0);
                Sleep(30);
            }
            log_line("key driver: mouse %s x%d", name, count);
        }
        if (strcmp(name, "FIRE") == 0) {
            for (int i = 0; i < count; i++) {
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                Sleep(hold);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                Sleep(150);
            }
            log_line("key driver: fire x%d (hold %d ms)", count, hold);
        }
        for (const key_entry &key : keys) {
            if (strcmp(key.name, name) != 0) continue;
            for (int i = 0; i < count; i++) {
                DWORD flags = KEYEVENTF_SCANCODE | (key.extended ? KEYEVENTF_EXTENDEDKEY : 0);
                keybd_event(0, key.scan, flags, 0);
                Sleep(hold);
                keybd_event(0, key.scan, flags | KEYEVENTF_KEYUP, 0);
                Sleep(150);
            }
            log_line("key driver: pressed %s x%d hold %d ms (window %p, foreground %s)", name, count, hold,
                     static_cast<void *>(hwnd), GetForegroundWindow() == hwnd ? "yes" : "NO");
        }
    }
    return 0;
}

void start_key_driver()
{
    if (GetEnvironmentVariableA("HALO_STANDALONE_KEYS", g_key_script, sizeof g_key_script) && g_key_script[0]) {
        CreateThread(nullptr, 0, key_driver_thread, nullptr, 0, nullptr);
        log_line("key driver: script %s", g_key_script);
    }
}

}  // namespace

/** Writes one formatted line to the standalone log (the entry point behind standalone_log). */
void log_formatted(const char *format, va_list ap)
{
    char text[1024];
    _vsnprintf(text, sizeof text - 1, format, ap);
    text[sizeof text - 1] = 0;
    log_line("[%lu] %s", (unsigned long)GetTickCount(), text);
}

/**
 * Developer mode (a standalone extension; retail 01.00.10 has no -devmode switch): lifts the console's command
 * availability filter so every script function and global (cheats included) can be run from the in-game console.
 * On with "-devmode" on the command line or HALO_DEVMODE in the environment.
 */
bool devmode_enabled()
{
    static int cached = -1;

    if (cached < 0) {
        const char *line = GetCommandLineA();

        cached = (line && strstr(line, "-devmode")) || GetEnvironmentVariableA("HALO_DEVMODE", nullptr, 0) ? 1 : 0;
    }
    return cached != 0;
}

/** Runs the loader: log, diagnostics, DLL path, CreateFileA override, CRT globals, then shell_winmain. */
int run(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show)
{
    GetModuleFileNameA(nullptr, g_exe_dir, sizeof g_exe_dir);
    if (char *slash = strrchr(g_exe_dir, '\\')) *slash = 0;

    log_line("halo standalone: started");
    if (GetEnvironmentVariableA("HALO_STANDALONE_WAIT_DEBUGGER", nullptr, 0) && !IsDebuggerPresent()) {
        char text[256];

        log_line("waiting for a debugger to attach to process %lu", GetCurrentProcessId());
        fflush(g_log);
        _snprintf(text, sizeof text, "Attach the debugger to halo_rebuilt.exe, process ID %lu "
                  "(CLion: Run > Attach to Process), then press OK.", GetCurrentProcessId());
        MessageBoxA(nullptr, text, "halo_rebuilt: waiting for debugger", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
    }
    install_diagnostics();
    preload_system_dinput8();
    SetDllDirectoryA(standalone_halo_folder);
    hook_own_create_file_a();
    emulate_crt_startup();
    if (int misplaced = standalone_data_layout_check()) {
        log_line("data layout: %d globals are not where the original image has them", misplaced);
    }
    if (!SetCurrentDirectoryA(standalone_halo_folder)) {
        log_line("cannot change to the Halo folder %s", standalone_halo_folder);
    }
    start_key_driver();
    log_line("calling shell_winmain");
    return halo::shell::shell_winmain(instance, previous, command_line, show);
}

}  // namespace halo::standalone

/**
 * The Halo install folder: the maps, Keystone.dll and the game's working directory.
 * Override at build time with cmake -DHALO_FOLDER=D:/Games/Halo.
 */
const char standalone_halo_folder[] = HALO_FOLDER;

void __cdecl standalone_log(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    halo::standalone::log_formatted(format, ap);
    va_end(ap);
}

int standalone_devmode(void)
{
    return halo::standalone::devmode_enabled() ? 1 : 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show)
{
    return halo::standalone::run(instance, previous, command_line, show);
}
