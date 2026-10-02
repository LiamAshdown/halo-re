// shell_winmain  (Ghidra: shell_winmain, already named)
// address 0x5411e0, size 2077 bytes
// name confidence: 0.7   rewrite confidence: 0.7
// evidence: string literals match every command line flag shell.h documents (-window(ed),
// -shell_nosound, -network_disabled_flag, -novideo/-connect, -nojoystick, -width640, -screenshot(s), -checkfpu,
// -timedemo, -nowinkey/-nowindowskey, -safemode, -? / -help, -testcrash, -port, -cport, -ip), the
// registry path "Software\Microsoft\Microsoft Games\Halo" / "FIRSTRUN" matches the EULA first-run
// check, and 0x00721f08 / 0x00721f0c match shell.h's shell_stack_guard_page / _old_protect.
// register convention: __stdcall WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow), ret 0x10.
// Review pass: re-derived against objdump 0x5411e0..0x541a13. Corrections to the first rewrite:
//   - The body from the stack guard fill to the guard restore is one __try whose filter is
//     exception_filter_crash_reporter(GetExceptionInformation()) (scope table 0x006732c8, filter
//     0x5419c1, handler 0x5419cb). The handler resumes at the exit path (exit flag, mutex close).
//     The first rewrite dropped it.
//   - The byte at [ebp-0x19] is a local set to 1 inside the __try (0x541252) and never cleared;
//     MSVC keeps it in memory because of the __try. It gates the "Corrupted Halo.exe" / -testcrash
//     block (dead while it stays 1) and whether engine_initialize_subsystems is called. The
//     -testcrash branch writes a byte to address 0 (mov [0],bl at 0x541757): that fault is the
//     point of -testcrash (it exercises the crash reporter), so it is kept.
//   - The command line flags are 32 bit BOOLs (movzx then a dword store); sound_cache_size and
//     the 0x006869c8 value are 16 bit stores.
//   - memory_global_alloc 0x449370 takes the size in EAX (length + 1) and memory_global_free
//     0x449380 the block in EAX: the copied command line, then shell_argv.
//   - shell_parse_config_txt takes the adapter index in ECX (0) and the IDirect3D9 in EDX, and
//     returns an error string. shell_direct3d is stored only when Direct3DCreate9 succeeded.
//   - The EULA call passes eula_file_name (0x006f0870), not a resource id.
//   - The window class / title copies are 5 bytes ("Halo" and its terminator).
//   - 0x00670fe8 is "-?" and 0x00670f9c is "-ip" (read from .rdata). The -ip value goes through
//     inet_addr (delay-load import slot 0x0069fffc) and is byte swapped.
//   - network_session_host_start_info_set 0x576100 gets EAX = "halor", ESI = a stack string
//     "e4Rd9J" (the GameSpy game name and secret key), EDI = the -ip value and the port on the
//     stack. The first rewrite passed only the port.
//   - 0x4c7610 (Ghidra: game_state_save_core, a mechanical name) is the main loop: it plays the
//     intro movies, pumps messages and paces frames. Renamed main_loop.
//   - The -? / -help path shows the message box and returns without the exit path.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include <string.h>
#include "interface.h"

#if defined(_MSC_VER)
#include <excpt.h>   // GetExceptionInformation is the _exception_info intrinsic, not a function
#define WINMAIN_TRY __try
#define WINMAIN_EXCEPT __except (exception_filter_crash_reporter((win32_exception_pointers *)GetExceptionInformation()))
#else
#define WINMAIN_TRY
#define WINMAIN_EXCEPT if (0)
#endif

extern char *shell_command_line;             // 0x006e35c0
extern void *shell_window;                   // 0x007461c4
extern void *shell_instance;                 // 0x007461c0
extern int32_t shell_show_command;           // 0x007461cc
extern uint32_t shell_window_proc;           // 0x007461d0
extern uint8_t code_address_shell_window_procedure[]; // 0x00541b30: the original address in the hooked build,
                                             // shell_window_procedure itself in the standalone build
extern void *rasterizer_window_handle;             // 0x007461c8
extern uint8_t shell_window_maximized;       // 0x00746255
extern uint8_t shell_window_minimized;       // 0x00746254
extern char shell_window_class_name[k_shell_window_name_length]; // 0x007461d4 "Halo"
extern char shell_window_title[k_shell_window_name_length];      // 0x00746214 "Halo"
extern void *shell_arrow_cursor;             // 0x006e35bc
extern char eula_file_name[k_shell_eula_name_length];            // 0x006f0870 "eula.rtf"

extern char **shell_argv;                    // 0x00721e90
extern int32_t shell_argc;                   // 0x00721e94
extern void *shell_direct3d;                 // 0x00721e98 IDirect3D9 *
extern void *d3d9_module;                    // 0x00746264
extern void *direct3d_create9;               // 0x00746274 FARPROC
extern void *dsound_module;                  // 0x0074625c
extern void *direct_sound_create8;           // 0x00746270
extern void *dinput8_module;                 // 0x00746258
extern void *direct_input8_create;           // 0x00746268
extern void *shfolder_module;                // 0x00746260
extern void *sh_get_folder_path;             // 0x0074626c

extern int32_t screenshots;                  // 0x007196e0
extern int32_t shell_nosound;                      // 0x007196e4
extern int32_t novideo_or_connect;           // 0x007196e8
extern int32_t network_disabled_flag;                    // 0x007196ec
extern int32_t width640;                     // 0x007196f0
extern int32_t safe_mode;                    // 0x007196f4
extern int32_t nowindowskey;                 // 0x007196f8
extern int32_t nojoystick;                   // 0x00712c2c
extern int32_t checkfpu;                     // 0x0071d1a4
extern int32_t windowed;                     // 0x0071d1ac
extern int32_t game_time_force_single_tick;  // 0x007196d8 -timedemo

extern uint32_t physical_memory;             // 0x00722ba8
extern int16_t sound_cache_size_megabytes;   // 0x006869c4, 16 bit store
extern int16_t sound_cache_unknown_c8;       // 0x006869c8, 16 bit store of 0x30
extern int32_t required_memory;              // 0x006effcc
extern uint32_t cpu_speed;                   // 0x00722bac
extern int32_t required_cpu_speed;           // 0x006effe0
extern int32_t required_disk_space;          // 0x006effd4
extern char *shell_product_id;               // 0x007461a8
extern void *shell_instance_mutex;           // 0x00721f00
extern int32_t shell_instance_mode_value;    // 0x0069eab4 (the name shell_instance_mode is the enum typedef)
extern int32_t shell_instance_index;         // 0x00721f04
extern char strings_dll_invalid_text[k_shell_strings_dll_error_length]; // 0x006f0890, shown for -? / -help

extern void *shell_stack_guard_page;         // 0x00721f08
extern uint32_t shell_stack_guard_old_protect; // 0x00721f0c

extern uint32_t network_game_socket_port;                   // 0x00698208 -port
extern uint32_t game_cport;                  // 0x0069820c -cport
extern uint8_t port_overridden;              // 0x0071c2d0
extern uint32_t network_local_address;         // 0x006869b0 inet_addr result, then the swapped address
extern uint32_t connect_address;             // 0x006869a4 byte swapped copy


typedef int32_t (__cdecl *eula_show_fn)(const char *registry_path, const char *eula_file, int32_t unknown_2,
                                        int32_t unknown_3);                     // eula.dll EBUEula, add esp,0x10
typedef void *(__stdcall *direct3d_create9_fn)(uint32_t sdk_version);

extern void *memory_global_alloc(uint32_t size);                   // 0x449370, blam-cc: size in EAX; GlobalAlloc(0, size)
extern void memory_global_free(void *block);                       // 0x449380, blam-cc: block in EAX; GlobalFree
extern void cache_reserve_map_memory(void);                        // 0x4448d0
extern void main_loop(void);                                       // 0x4c7610 (Ghidra: game_state_save_core, see header)
extern uint8_t engine_initialize_subsystems(void);                 // 0x540ee0
extern void engine_shutdown_subsystems(void);                      // 0x541010
extern char **command_line_parse_to_argv(char *command_line, int32_t *out_count); // 0x5425f0, command_line in EDI
extern uint8_t command_line_check_flag(const char *flag_name, const char **out_value); // 0x542760, out_value in EDI; result in AL only
extern int32_t __stdcall exception_filter_crash_reporter(win32_exception_pointers *exception_pointers); // 0x542fa0
extern void keystone_library_load(void);                           // 0x542ad0
extern void keystone_library_unload(void);                         // 0x542cf0
extern void game_single_instance_check(int32_t mode);              // 0x542d70
extern void network_session_host_start_info_set(char *game_name, char *secret_key, char *ip_address,
                                                int32_t port);     // 0x576100, blam-cc: EAX, ESI, EDI, stack
extern char *shell_parse_config_txt(uint32_t adapter_index, d3d9_interface *d3d); // 0x57d410, ECX, EDX
extern void shell_detect_hardware_specs(void);                     // 0x57d880
extern int32_t shell_check_previous_run_crash(void);               // 0x57e850, full EAX result
extern void shell_registry_set_exit_flag_clean(void);              // 0x57ea10
extern int32_t shell_display_fatal_error_dialog(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal); // 0x57ea70
extern void shell_init_localization_strings(void);                 // 0x57efa0
extern char *shell_build_product_id_string(void);                  // 0x57f3f0

// The game's WinMain: loads the localized strings, takes the single instance mutex, arms a
// no-access guard byte inside a 0x2000 byte stack buffer, shows the EULA on first run, parses the
// command line flags, measures the hardware, loads d3d9 / dsound / dinput8 / shfolder, applies
// config.txt, checks the minimum requirements, and runs the main loop between
// engine_initialize_subsystems and engine_shutdown_subsystems. Any unhandled exception in there
// goes to the crash reporter.
int32_t __stdcall shell_winmain(void *hInstance, void *hPrevInstance, char *lpCmdLine, int32_t nCmdShow)
{
    uint32_t stack_guard_buffer[0x800];
    char temp_path[k_shell_path_length];
    volatile uint8_t integrity_ok;
    int32_t command_line_length;
    char *command_line_copy;
    void *hkey;
    uint32_t firstrun;
    uint32_t firstrun_size;
    void *eula_module;
    eula_show_fn eula_show;
    int32_t eula_accepted;
    d3d9_interface *direct3d9;
    char *config_error;
    void (*disable_d3dspy)(void);
    large_integer free_bytes_available;
    const char *port_value;
    const char *ip_value;
    char secret_key[9];
    int32_t i;

    (void)hPrevInstance;

    shell_init_localization_strings();
    game_single_instance_check(k_shell_instance_mode_single);

    WINMAIN_TRY {
        for (i = 0; i < 0x800; i++) {
            stack_guard_buffer[i] = 0xeeeeeeee;
        }
        shell_stack_guard_page = (uint8_t *)stack_guard_buffer + 0x1000;
        VirtualProtect(shell_stack_guard_page, 1, 1 /* PAGE_NOACCESS */, (PDWORD)&shell_stack_guard_old_protect);
        integrity_ok = 1;

        command_line_length = 0;
        while (lpCmdLine[command_line_length] != 0) {
            command_line_length++;
        }
        command_line_copy = (char *)memory_global_alloc((uint32_t)command_line_length + 1);
        if (command_line_length > 0) {
            char *source = lpCmdLine;
            char *dest = command_line_copy;
            do {
                *dest++ = *source;
            } while (*source++ != 0);
        }
        command_line_copy[command_line_length] = 0;

        SetLastError(0);
        shell_command_line = lpCmdLine;
        shell_window = 0;
        shell_instance = hInstance;
        shell_show_command = nCmdShow;
        shell_window_proc = (uint32_t)code_address_shell_window_procedure; // the window procedure 0x541b30
        rasterizer_window_handle = 0;
        shell_window_maximized = 0;
        shell_window_minimized = 0;
        memcpy(shell_window_class_name, "Halo", 5);
        memcpy(shell_window_title, "Halo", 5);
        shell_arrow_cursor = LoadCursorA(0, (const char *)0x7f00 /* IDC_ARROW */);

        // first run: the EULA
        firstrun = 0;
        firstrun_size = 4;
        RegOpenKeyExA((HKEY)0x80000001 /* HKEY_CURRENT_USER */, "Software\\Microsoft\\Microsoft Games\\Halo", 0,
                      0x20019 /* KEY_READ */, (PHKEY)&hkey);
        RegQueryValueExA((HKEY)hkey, "FIRSTRUN", 0, 0, (uint8_t *)&firstrun, (LPDWORD)&firstrun_size);
        RegCloseKey((HKEY)hkey);
        if (firstrun == 0) {
            eula_accepted = 0;
            eula_module = LoadLibraryA("eula.dll");
            if (eula_module != 0) {
                eula_show = (eula_show_fn)GetProcAddress((HMODULE)eula_module, "EBUEula");
                if (eula_show != 0) {
                    eula_accepted = eula_show("Software\\Microsoft\\Microsoft Games\\Halo", eula_file_name, 0, 1);
                }
                FreeLibrary((HMODULE)eula_module);
            }
            if (eula_accepted == 0) {
                ExitProcess(1);
            }
        }

        // command line
        shell_argv = command_line_parse_to_argv(command_line_copy, &shell_argc);
        windowed = command_line_check_flag("-window", 0) || command_line_check_flag("-windowed", 0);
        shell_nosound = command_line_check_flag("-nosound", 0);
        network_disabled_flag = command_line_check_flag("-nonetwork", 0);
        novideo_or_connect = command_line_check_flag("-novideo", 0) ||
                             command_line_check_flag("-connect", 0);
        nojoystick = command_line_check_flag("-nojoystick", 0);
        width640 = command_line_check_flag("-width640", 0);
        screenshots = command_line_check_flag("-screenshots", 0) ||
                      command_line_check_flag("-screenshot", 0);
        checkfpu = command_line_check_flag("-checkfpu", 0);
        game_time_force_single_tick = command_line_check_flag("-timedemo", 0) != 0;
        nowindowskey = command_line_check_flag("-nowinkey", 0) ||
                       command_line_check_flag("-nowindowskey", 0);
        if (safe_mode != 0 || (safe_mode = command_line_check_flag("-safemode", 0)) != 0) {
            width640 = 1;
            novideo_or_connect = 1;
            nojoystick = 1;
            windowed = 1;
            shell_nosound = 1;
        }

        shell_detect_hardware_specs();
        if (physical_memory <= 0x80) {
            sound_cache_size_megabytes = 12;
        } else if (physical_memory <= 0x100) {
            sound_cache_size_megabytes = 16;
        } else {
            sound_cache_size_megabytes = physical_memory < 0x200 ? 32 : 64;
        }
        sound_cache_unknown_c8 = 0x30;

        if (command_line_check_flag("-?", 0) || command_line_check_flag("-help", 0)) {
            MessageBoxA(0, strings_dll_invalid_text, "Halo", 0);
            return 0;   // leaves the __try without the exit path (state -1 at 0x5419b8)
        }

        // Direct3D and config.txt
        cache_reserve_map_memory();
        d3d9_module = LoadLibraryA("d3d9.dll");
        direct3d_create9 = GetProcAddress((HMODULE)d3d9_module, "Direct3DCreate9");
        if (d3d9_module == 0 || direct3d_create9 == 0) {
            shell_display_fatal_error_dialog(0x6b, (uint32_t)((const char *)0x7a), 1);
        }
        direct3d9 = (d3d9_interface *)((direct3d_create9_fn)direct3d_create9)(0x1f /* D3D_SDK_VERSION */);
        if (direct3d9 != 0) {
            config_error = shell_parse_config_txt(0, direct3d9);
            if (config_error != 0) {
                shell_display_fatal_error_dialog(0xffffffff, (uint32_t)(config_error), 0);
            }
            shell_direct3d = direct3d9;
        } else {
            shell_display_fatal_error_dialog(0x81, (uint32_t)((const char *)0x82), 1);
        }

        if (GetAsyncKeyState(0x11 /* VK_CONTROL */) < 0) {
            shell_display_fatal_error_dialog(0x87, (uint32_t)((const char *)0x7e), 0);
        }
        disable_d3dspy = (void (*)(void))GetProcAddress((HMODULE)d3d9_module, "DisableD3DSpy");
        if (disable_d3dspy != 0) {
            disable_d3dspy();
        }

        // the other DirectX / shell DLLs
        if (shell_nosound != 0) {
            dsound_module = 0;
            direct_sound_create8 = 0;
        } else {
            dsound_module = LoadLibraryA("dsound.dll");
            direct_sound_create8 = GetProcAddress((HMODULE)dsound_module, "DirectSoundCreate8");
            if (dsound_module == 0 || direct_sound_create8 == 0) {
                shell_display_fatal_error_dialog(0x7b, (uint32_t)((const char *)0x7a), 1);
            }
        }
        dinput8_module = LoadLibraryA("dinput8.dll");
        direct_input8_create = GetProcAddress((HMODULE)dinput8_module, "DirectInput8Create");
        if (dinput8_module == 0 || direct_input8_create == 0) {
            shell_display_fatal_error_dialog(0x7c, (uint32_t)((const char *)0x7a), 1);
        }
        shfolder_module = LoadLibraryA("shfolder.dll");
        sh_get_folder_path = GetProcAddress((HMODULE)shfolder_module, "SHGetFolderPathA");
        if (shfolder_module == 0 || sh_get_folder_path == 0) {
            shell_display_fatal_error_dialog(0x7d, (uint32_t)((const char *)0x7e), 1);
        }

        if (shell_check_previous_run_crash() != 0) {
            shell_display_fatal_error_dialog(0x6a, (uint32_t)((const char *)0x73), 0);
        }

        if (integrity_ok == 0) {
            if (command_line_check_flag("-testcrash", 0) == 0) {
                MessageBoxA(0, "Corrupted Halo.exe", "Halo", 0);
                ExitProcess(1);
            }
            *(volatile uint8_t *)0 = 0;     // -testcrash: fault on purpose
        }

        // minimum requirements
        if (physical_memory < (uint32_t)(required_memory - 0x10)) {
            shell_display_fatal_error_dialog(0x65, (uint32_t)((const char *)0x6e), 0);
        }
        if (cpu_speed < (uint32_t)required_cpu_speed) {
            shell_detect_hardware_specs();
            if (cpu_speed < (uint32_t)required_cpu_speed) {
                shell_detect_hardware_specs();
                if (cpu_speed < (uint32_t)required_cpu_speed) {
                    shell_display_fatal_error_dialog(0x66, (uint32_t)((const char *)0x6f), 0);
                }
            }
        }
        GetTempPathA(sizeof(temp_path), temp_path);
        GetDiskFreeSpaceExA(temp_path, (PULARGE_INTEGER)&free_bytes_available, 0, 0);
        if (free_bytes_available.parts.high_part <= 0 &&
            (free_bytes_available.parts.high_part < 0 ||
             free_bytes_available.parts.low_part < (uint32_t)(required_disk_space << 20))) {
            shell_display_fatal_error_dialog(0x6d, (uint32_t)((const char *)0x76), 0);
        }

        shell_product_id = shell_build_product_id_string();
        if (*shell_product_id == 0) {
            shell_display_fatal_error_dialog(0xa0, (uint32_t)((const char *)0x7e), 1);
        }

        keystone_library_load();

        // run
        if (integrity_ok == 0 || engine_initialize_subsystems() != 0) {
            port_value = 0;
            ip_value = 0;
            network_local_address = 0;
            if (command_line_check_flag("-port", &port_value) && port_value != 0) {
                network_game_socket_port = (uint32_t)atol(port_value);
                port_overridden = 1;
            }
            if (command_line_check_flag("-cport", &port_value) && port_value != 0) {
                game_cport = (uint32_t)atol(port_value);
                port_overridden = 1;
            }
            if (command_line_check_flag("-ip", &ip_value) && ip_value != 0) {
                network_local_address = inet_addr(ip_value);
                if (network_local_address != 0) {
                    connect_address = (network_local_address << 24) | ((network_local_address & 0xff00) << 8) |
                                      ((network_local_address >> 8) & 0xff00) | (network_local_address >> 24);
                    network_local_address = connect_address;
                }
            }
            memset(secret_key, 0, sizeof(secret_key));
            memcpy(secret_key, "e4Rd9J", 7);
            network_session_host_start_info_set((char *)"halor", secret_key, (char *)ip_value, (int32_t)network_game_socket_port);
            main_loop();
            engine_shutdown_subsystems();
        }

        memory_global_free(command_line_copy);
        memory_global_free(shell_argv);
        keystone_library_unload();
        if (shell_stack_guard_page != 0) {
            VirtualProtect(shell_stack_guard_page, 1, shell_stack_guard_old_protect, (PDWORD)&shell_stack_guard_old_protect);
            shell_stack_guard_page = 0;
        }
    } WINMAIN_EXCEPT {
    }

    shell_registry_set_exit_flag_clean();
    if (shell_instance_mutex != 0) {
        CloseHandle(shell_instance_mutex);
        shell_instance_mutex = 0;
        shell_instance_mode_value = k_shell_instance_mode_none;
        shell_instance_index = -1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x5411e0):


/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int shell_winmain(void *hInstance,void *hPrevInstance,char *lpCmdLine,int nCmdShow)

{
  char cVar1;
  SHORT SVar2;
  char *pcVar3;
  HMODULE hModule;
  uint uVar4;
  int iVar5;
  char *pcVar6;
  undefined4 *puVar7;
  undefined4 local_216c [1024];
  undefined1 local_116c [4096];
  CHAR local_16c [260];
  int local_68;
  FARPROC local_64;
  FARPROC local_60;
  int local_5c;
  HMODULE local_58;
  int local_54;
  ULARGE_INTEGER local_50;
  DWORD local_48;
  int local_44;
  int local_40;
  HKEY local_3c;
  char *local_38;
  undefined1 local_34;
  undefined4 local_33;
  undefined4 local_2f;
  char *local_28;
  char *local_24;
  undefined4 uStack_20;
  undefined1 *local_1c;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_006732c8;
  puStack_10 = &LAB_00628dfc;
  local_14 = ExceptionList;
  uStack_20 = 0x54120a;
  local_1c = &stack0xffffde88;
  ExceptionList = &local_14;
  shell_init_localization_strings();
  game_single_instance_check(0);
  local_8 = 0;
  puVar7 = local_216c;
  for (iVar5 = 0x800; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar7 = 0xeeeeeeee;
    puVar7 = puVar7 + 1;
  }
  DAT_00721f08 = local_116c;
  VirtualProtect(DAT_00721f08,1,1,&DAT_00721f0c);
  uStack_20 = CONCAT13(1,(undefined3)uStack_20);
  pcVar3 = lpCmdLine;
  do {
    cVar1 = *pcVar3;
    pcVar3 = pcVar3 + 1;
  } while (cVar1 != '\0');
  iVar5 = (int)pcVar3 - (int)(lpCmdLine + 1);
  local_54 = iVar5;
  local_24 = (char *)memory_global_alloc();
  pcVar3 = lpCmdLine;
  pcVar6 = local_24;
  if (0 < iVar5) {
    do {
      cVar1 = *pcVar3;
      *pcVar6 = cVar1;
      pcVar3 = pcVar3 + 1;
      pcVar6 = pcVar6 + 1;
    } while (cVar1 != '\0');
  }
  local_24[iVar5] = '\0';
  SetLastError(0);
  DAT_006e35c0 = lpCmdLine;
  DAT_007461c4 = 0;
  DAT_007461c0 = hInstance;
  _DAT_007461cc = nCmdShow;
  DAT_007461d0 = &DAT_00541b30;
  DAT_007461c8 = 0;
  DAT_00746255 = 0;
  DAT_00746254 = 0;
  _DAT_007461d4 = 0x6f6c6148;
  DAT_007461d8 = 0;
  _DAT_00746214 = 0x6f6c6148;
  DAT_00746218 = 0;
  DAT_006e35bc = LoadCursorA((HINSTANCE)0x0,&DAT_00007f00);
  local_40 = 0;
  local_48 = 4;
  RegOpenKeyExA((HKEY)&DAT_80000001,"Software\\Microsoft\\Microsoft Games\\Halo",0,0x20019,&local_3c
               );
  RegQueryValueExA(local_3c,"FIRSTRUN",(LPDWORD)0x0,(LPDWORD)0x0,(LPBYTE)&local_40,&local_48);
  RegCloseKey(local_3c);
  if (local_40 == 0) {
    iVar5 = 0;
    local_44 = 0;
    hModule = LoadLibraryA("eula.dll");
    local_58 = hModule;
    if (hModule != (HMODULE)0x0) {
      local_60 = GetProcAddress(hModule,"EBUEula");
      if (local_60 != (FARPROC)0x0) {
        iVar5 = (*local_60)("Software\\Microsoft\\Microsoft Games\\Halo",&DAT_006f0870,0,1);
        local_44 = iVar5;
      }
      FreeLibrary(hModule);
    }
    if (iVar5 == 0) {
                    /* WARNING: Subroutine does not return */
      ExitProcess(1);
    }
  }
  DAT_00721e90 = command_line_parse_to_argv(&DAT_00721e94);
  cVar1 = command_line_check_flag("-window");
  if ((cVar1 == '\0') && (cVar1 = command_line_check_flag("-windowed"), cVar1 == '\0')) {
    DAT_0071d1ac = 0;
  }
  else {
    DAT_0071d1ac = 1;
  }
  uVar4 = command_line_check_flag("-nosound");
  DAT_007196e4 = uVar4 & 0xff;
  uVar4 = command_line_check_flag("-nonetwork");
  DAT_007196ec = uVar4 & 0xff;
  cVar1 = command_line_check_flag("-novideo");
  if ((cVar1 == '\0') && (cVar1 = command_line_check_flag("-connect"), cVar1 == '\0')) {
    DAT_007196e8 = 0;
  }
  else {
    DAT_007196e8 = 1;
  }
  uVar4 = command_line_check_flag("-nojoystick");
  DAT_00712c2c = uVar4 & 0xff;
  uVar4 = command_line_check_flag("-width640");
  DAT_007196f0 = uVar4 & 0xff;
  cVar1 = command_line_check_flag("-screenshots");
  if ((cVar1 == '\0') && (cVar1 = command_line_check_flag("-screenshot"), cVar1 == '\0')) {
    DAT_007196e0 = 0;
  }
  else {
    DAT_007196e0 = 1;
  }
  uVar4 = command_line_check_flag("-checkfpu");
  DAT_0071d1a4 = uVar4 & 0xff;
  cVar1 = command_line_check_flag("-timedemo");
  DAT_007196d8 = (uint)(cVar1 != '\0');
  cVar1 = command_line_check_flag("-nowinkey");
  if ((cVar1 == '\0') && (cVar1 = command_line_check_flag("-nowindowskey"), cVar1 == '\0')) {
    DAT_007196f8 = 0;
  }
  else {
    DAT_007196f8 = 1;
  }
  if (DAT_007196f4 == 0) {
    uVar4 = command_line_check_flag("-safemode");
    DAT_007196f4 = uVar4 & 0xff;
    if (DAT_007196f4 == 0) goto LAB_00541532;
  }
  DAT_007196f0 = 1;
  DAT_007196e8 = 1;
  DAT_00712c2c = 1;
  DAT_0071d1ac = 1;
  DAT_007196e4 = 1;
LAB_00541532:
  shell_detect_hardware_specs();
  if (DAT_00722ba8 < 0x81) {
    DAT_006869c4 = 0xc;
  }
  else if (DAT_00722ba8 < 0x101) {
    DAT_006869c4 = 0x10;
  }
  else {
    DAT_006869c4 = (-(ushort)(DAT_00722ba8 < 0x200) & 0xffe0) + 0x40;
  }
  _DAT_006869c8 = 0x30;
  cVar1 = command_line_check_flag(&DAT_00670fe8);
  if ((cVar1 == '\0') && (cVar1 = command_line_check_flag("-help"), cVar1 == '\0')) {
    cache_reserve_map_memory();
    DAT_00746264 = LoadLibraryA("d3d9.dll");
    DAT_00746274 = GetProcAddress(DAT_00746264,"Direct3DCreate9");
    if ((DAT_00746264 == (HMODULE)0x0) || (DAT_00746274 == (FARPROC)0x0)) {
      shell_display_fatal_error_dialog(0x6b,0x7a,1);
    }
    iVar5 = (*DAT_00746274)(0x1f);
    local_5c = iVar5;
    if (iVar5 == 0) {
      shell_display_fatal_error_dialog(0x81,0x82,1);
      iVar5 = DAT_00721e98;
    }
    else {
      local_68 = shell_parse_config_txt();
      if (local_68 != 0) {
        shell_display_fatal_error_dialog(0xffffffff,local_68,0);
      }
    }
    DAT_00721e98 = iVar5;
    SVar2 = GetAsyncKeyState(0x11);
    if (SVar2 < 0) {
      shell_display_fatal_error_dialog(0x87,0x7e,0);
    }
    local_64 = GetProcAddress(DAT_00746264,"DisableD3DSpy");
    if (local_64 != (FARPROC)0x0) {
      (*local_64)();
    }
    if (DAT_007196e4 == 0) {
      DAT_0074625c = LoadLibraryA("dsound.dll");
      DAT_00746270 = GetProcAddress(DAT_0074625c,"DirectSoundCreate8");
      if ((DAT_0074625c == (HMODULE)0x0) || (DAT_00746270 == (FARPROC)0x0)) {
        shell_display_fatal_error_dialog(0x7b,0x7a,1);
      }
    }
    else {
      DAT_0074625c = (HMODULE)0x0;
      DAT_00746270 = (FARPROC)0x0;
    }
    DAT_00746258 = LoadLibraryA("dinput8.dll");
    DAT_00746268 = GetProcAddress(DAT_00746258,"DirectInput8Create");
    if ((DAT_00746258 == (HMODULE)0x0) || (DAT_00746268 == (FARPROC)0x0)) {
      shell_display_fatal_error_dialog(0x7c,0x7a,1);
    }
    DAT_00746260 = LoadLibraryA("shfolder.dll");
    DAT_0074626c = GetProcAddress(DAT_00746260,"SHGetFolderPathA");
    if ((DAT_00746260 == (HMODULE)0x0) || (DAT_0074626c == (FARPROC)0x0)) {
      shell_display_fatal_error_dialog(0x7d,0x7e,1);
    }
    iVar5 = chimera__registry_check_2();
    if (iVar5 != 0) {
      shell_display_fatal_error_dialog(0x6a,0x73,0);
    }
    if (uStack_20._3_1_ == '\0') {
      cVar1 = command_line_check_flag("-testcrash");
      if (cVar1 == '\0') {
        MessageBoxA((HWND)0x0,"Corrupted Halo.exe","Halo",0);
                    /* WARNING: Subroutine does not return */
        ExitProcess(1);
      }
      DAT_00000000 = 0;
    }
    if (DAT_00722ba8 < DAT_006effcc - 0x10U) {
      shell_display_fatal_error_dialog(0x65,0x6e,0);
    }
    if (((DAT_00722bac < DAT_006effe0) &&
        (shell_detect_hardware_specs(), DAT_00722bac < DAT_006effe0)) &&
       (shell_detect_hardware_specs(), DAT_00722bac < DAT_006effe0)) {
      shell_display_fatal_error_dialog(0x66,0x6f,0);
    }
    GetTempPathA(0x104,local_16c);
    GetDiskFreeSpaceExA(local_16c,&local_50,(PULARGE_INTEGER)0x0,(PULARGE_INTEGER)0x0);
    if (((int)local_50.s.HighPart < 1) &&
       (((int)local_50.s.HighPart < 0 || (local_50.s.LowPart < (uint)(DAT_006effd4 << 0x14))))) {
      shell_display_fatal_error_dialog(0x6d,0x76,0);
    }
    DAT_007461a8 = (char *)FUN_0057f3f0();
    if (*DAT_007461a8 == '\0') {
      shell_display_fatal_error_dialog(0xa0,0x7e,1);
    }
    keystone_library_load();
    if ((uStack_20._3_1_ == '\0') || (iVar5 = engine_initialize_subsystems(), (char)iVar5 != '\0'))
    {
      local_28 = (char *)0x0;
      local_38 = (char *)0x0;
      DAT_006869b0 = 0;
      cVar1 = command_line_check_flag("-port");
      if ((cVar1 != '\0') && (local_28 != (char *)0x0)) {
        DAT_00698208 = _atol(local_28);
        DAT_0071c2d0 = 1;
      }
      cVar1 = command_line_check_flag("-cport");
      if ((cVar1 != '\0') && (local_28 != (char *)0x0)) {
        _DAT_0069820c = _atol(local_28);
        DAT_0071c2d0 = 1;
      }
      cVar1 = command_line_check_flag(&PTR_DAT_00670f9c);
      if (((cVar1 != '\0') && (local_38 != (char *)0x0)) &&
         (DAT_006869b0 = inet_addr(local_38), DAT_006869b0 != 0)) {
        _DAT_006869a4 =
             (DAT_006869b0 << 0x10 | DAT_006869b0 & 0xff00 | DAT_006869b0 >> 0x10 & 0xff) << 8 |
             DAT_006869b0 >> 0x18;
        DAT_006869b0 = _DAT_006869a4;
      }
      local_34 = 0x65;
      local_33 = 0x39645234;
      local_2f = 0x4a;
      FUN_00576100(DAT_00698208);
      game_state_save_core();
      engine_shutdown_subsystems();
    }
    memory_global_free();
    memory_global_free();
    keystone_library_unload();
    if (DAT_00721f08 != (undefined1 *)0x0) {
      VirtualProtect(DAT_00721f08,1,DAT_00721f0c,&DAT_00721f0c);
      DAT_00721f08 = (undefined1 *)0x0;
    }
    local_8 = 0xffffffff;
    shell_registry_set_exit_flag_clean();
    if (DAT_00721f00 != (HANDLE)0x0) {
      CloseHandle(DAT_00721f00);
      DAT_00721f00 = (HANDLE)0x0;
      _DAT_0069eab4 = 0xffffffff;
      DAT_00721f04 = 0xffffffff;
    }
  }
  else {
    MessageBoxA((HWND)0x0,(LPCSTR)&DAT_006f0890,"Halo",0);
  }
  ExceptionList = local_14;
  return 0;
}
#endif
