/* dinput8.dll proxy: the one file the harness adds to the Halo folder (delete it to uninstall).
   halo.exe does LoadLibrary("dinput8.dll") + GetProcAddress("DirectInput8Create"); the game folder is searched first,
   so this DLL is loaded. On the first DirectInput8Create call it loads halo_rewrite.dll (path below, or
   HALO_REWRITE_DLL in the environment), then forwards the call to the real dinput8.dll in the system directory.
   Build (x86): cl /nologo /O2 /LD proxy_dinput8.c /link /DEF:proxy_dinput8.def /OUT:dinput8.dll */
#include <windows.h>

#define DEFAULT_REWRITE_DLL "C:\\Users\\Liam-\\halo-re\\harness\\build\\halo_rewrite.dll"

typedef HRESULT (WINAPI *dinput8_create_t)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
static dinput8_create_t real_create;
static HMODULE rewrite_module;

static void load_once(void)
{
    char path[MAX_PATH];
    if (!real_create) {
        GetSystemDirectoryA(path, MAX_PATH);            /* SysWOW64 for a 32-bit process */
        lstrcatA(path, "\\dinput8.dll");
        real_create = (dinput8_create_t)GetProcAddress(LoadLibraryA(path), "DirectInput8Create");
    }
    if (!rewrite_module) {
        if (!GetEnvironmentVariableA("HALO_REWRITE_DLL", path, MAX_PATH)) lstrcpyA(path, DEFAULT_REWRITE_DLL);
        rewrite_module = LoadLibraryA(path);
        OutputDebugStringA(rewrite_module ? "dinput8 proxy: halo_rewrite.dll loaded\n" : "dinput8 proxy: halo_rewrite.dll FAILED to load\n");
        if (!rewrite_module) {
            char msg[512];
            wsprintfA(msg, "halo_rewrite.dll failed to load (error %lu)\n%s\nThe game continues without it.", GetLastError(), path);
            MessageBoxA(NULL, msg, "Halo rewrite harness", MB_ICONWARNING);
            rewrite_module = (HMODULE)1;               /* do not retry */
        }
    }
}

HRESULT WINAPI proxy_DirectInput8Create(HINSTANCE inst, DWORD version, REFIID riid, LPVOID *out, LPUNKNOWN outer)
{
    load_once();
    return real_create ? real_create(inst, version, riid, out, outer) : E_FAIL;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)instance; (void)reason; (void)reserved;
    return TRUE;
}
