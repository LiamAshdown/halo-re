#include "halo/shell/application.hpp"
#include "halo/shell/config.hpp"
#include "halo/shell/diagnostics.hpp"
#include "halo/shell/hardware.hpp"
#include "halo/shell/system.hpp"
#include "halo/shell/window.hpp"
#include <excpt.h>
#include "halo/math/api.hpp"

extern "C" {
extern char *shell_command_line;
extern void *shell_window;
extern void *shell_instance;
extern int32_t shell_show_command;
extern uint32_t shell_window_proc;
extern uint8_t code_address_shell_window_procedure[];
extern void *rasterizer_window_handle;
extern uint8_t shell_window_maximized;
extern uint8_t shell_window_minimized;
extern char shell_window_class_name[k_shell_window_name_length];
extern char shell_window_title[k_shell_window_name_length];
extern void *shell_arrow_cursor;
extern char eula_file_name[k_shell_eula_name_length];

extern char **shell_argv;
extern int32_t shell_argc;
extern void *shell_direct3d;
extern void *d3d9_module;
extern void *direct3d_create9;
extern void *dsound_module;
extern void *direct_sound_create8;
extern void *dinput8_module;
extern void *direct_input8_create;
extern void *shfolder_module;
extern void *sh_get_folder_path;

extern int32_t screenshots;
extern int32_t shell_nosound;
extern int32_t novideo_or_connect;
extern int32_t network_disabled_flag;
extern int32_t width640;
extern int32_t safe_mode;
extern int32_t nowindowskey;
extern int32_t nojoystick;
extern int32_t checkfpu;
extern int32_t windowed;
extern int32_t game_time_force_single_tick;

extern uint32_t physical_memory;
extern int16_t sound_cache_size_megabytes;
extern int16_t sound_cache_unknown_c8;
extern int32_t required_memory;
extern uint32_t cpu_speed;
extern int32_t required_cpu_speed;
extern int32_t required_disk_space;
extern char *shell_product_id;
extern char strings_dll_invalid_text[k_shell_strings_dll_error_length];

extern void *shell_stack_guard_page;
extern uint32_t shell_stack_guard_old_protect;

extern uint32_t network_game_socket_port;
extern uint32_t game_cport;
extern uint8_t port_overridden;
extern uint32_t network_local_address;
extern uint32_t connect_address;

extern void *memory_global_alloc(uint32_t size);
extern void memory_global_free(void *block);
extern void cache_reserve_map_memory(void);
extern void main_loop(void);
extern void network_session_host_start_info_set(char *game_name, char *secret_key, char *ip_address, int32_t port);

extern large_integer performance_frequency;
extern char profile_directory[0x105];
extern uint8_t sound_disabled;
extern uint8_t console_debug_flag_0;
extern uint8_t error_file_enabled;
extern uint8_t console_debug_flag_4;
extern uint8_t console_debug_flag_5;
extern uint8_t debug_log_level;
extern uint16_t console_debug_word_8;

extern uint32_t global_scenario_index;
extern uint16_t global_structure_bsp_index;
extern uint16_t *global_scenario_game_globals;
extern uint32_t global_scenario;
extern ScenarioStructureBSP *global_structure_bsp;
extern uint32_t global_structure_collision_bsp;
extern uint32_t global_collision_bsp;
extern uint32_t global_globals;
extern uint32_t external_00686b4c;
extern uint8_t external_00686b50;
extern void *external_00686b58;
extern void *external_00686b5c;
extern uint32_t external_00686b54;

extern uint8_t data_file_open(void);
extern void directory_create_recursive(char *path);
extern void profile_path_initialize(void);
extern void input_directinput_initialize(void);
extern uint32_t render_initialize(void);
extern void game_state_startup(void);
extern uint32_t sound_initialize(void);
extern void cache_file_unload(void);
extern void data_file_close(void);
extern void input_directinput_release_devices(void);
extern void rasterizer_shutdown(void);
extern void sound_dispose(void);
}

typedef int32_t (__cdecl *eula_show_fn)(const char *registry_path, const char *eula_file, int32_t unknown_2, int32_t unknown_3);
typedef void *(__stdcall *direct3d_create9_fn)(uint32_t sdk_version);

namespace halo::shell {

/**
 * Sets the timer resolution, resolves the Direct3D / DirectSound / DirectInput / shell folder entry
 * points when config loading has not yet, and starts the data file, math, game state, render, input and
 * sound subsystems. Returns 1 when the renderer started, otherwise its failure result.
 *
 * @address 0x540ee0
 */
uint8_t EngineLifecycle::initialize()
{
    int32_t i;
    uint32_t startup_ok;

    timeBeginPeriod(1);
    QueryPerformanceFrequency((LARGE_INTEGER *)&performance_frequency);

    for (i = 0; i < 0x105; i++) {
        profile_directory[i] = 0;
    }

    profile_path_initialize();

    if (direct3d_create9 == 0) {
        d3d9_module = LoadLibraryA("d3d9.dll");
        direct3d_create9 = GetProcAddress((HMODULE)d3d9_module, "Direct3DCreate9");

        if (shell_nosound == 0) {
            dsound_module = LoadLibraryA("dsound.dll");
            direct_sound_create8 = GetProcAddress((HMODULE)dsound_module, "DirectSoundCreate8");
        } else {
            dsound_module = 0;
            direct_sound_create8 = 0;
        }

        dinput8_module = LoadLibraryA("dinput8.dll");
        direct_input8_create = GetProcAddress((HMODULE)dinput8_module, "DirectInput8Create");

        shfolder_module = LoadLibraryA("shfolder.dll");
        sh_get_folder_path = GetProcAddress((HMODULE)shfolder_module, "SHGetFolderPathA");
    }

    directory_create_recursive(profile_directory);

    debug_log_level = 0;
    error_file_enabled = 1;
    console_debug_flag_4 = 1;
    console_debug_flag_5 = 0;
    console_debug_flag_0 = 0;
    console_debug_word_8 = 0;

    data_file_open();
    halo::math::math_initialize();
    game_state_startup();

    startup_ok = render_initialize();
    if ((uint8_t)startup_ok != 0) {
        input_directinput_initialize();
        sound_disabled = (uint8_t)shell_nosound;
        sound_initialize();
        return 1;
    }
    return (uint8_t)startup_ok;
}

/**
 * Tears down the engine subsystems: unloads the cache file, clears the scenario globals, releases
 * input, shuts down the rasterizer, frees the sound and lookup buffers and ends the timer resolution.
 *
 * @address 0x541010
 */
void EngineLifecycle::shutdown()
{
    cache_file_unload();
    global_scenario_index = 0xffffffff;
    global_structure_bsp_index = 0xffff;
    *global_scenario_game_globals = 0xffff;
    global_scenario = 0;
    global_structure_bsp = 0;
    global_structure_collision_bsp = 0;
    global_collision_bsp = 0;
    global_globals = 0;

    input_directinput_release_devices();
    rasterizer_shutdown();
    GlobalFree(halo::math::globals().sphere_point_table);
    halo::math::periodic_function_tables_free();
    data_file_close();
    sound_dispose();

    external_00686b4c = 0xffffffff;
    external_00686b50 = 0;
    if (external_00686b58 != 0) {
        GlobalFree(external_00686b58);
    }
    if (external_00686b5c != 0) {
        GlobalFree(external_00686b5c);
    }
    external_00686b54 = 0;
    external_00686b58 = 0;
    external_00686b5c = 0;

    timeEndPeriod(1);
}

/**
 * Copies the command line into a GlobalAlloc'd buffer the tokenizer may edit.
 */
char *Application::copy_command_line(const char *command_line)
{
    int32_t command_line_length;
    char *command_line_copy;

    command_line_length = 0;
    while (command_line[command_line_length] != 0) {
        command_line_length++;
    }
    command_line_copy = (char *)memory_global_alloc((uint32_t)command_line_length + 1);
    if (command_line_length > 0) {
        const char *source = command_line;
        char *dest = command_line_copy;
        do {
            *dest++ = *source;
        } while (*source++ != 0);
    }
    command_line_copy[command_line_length] = 0;
    return command_line_copy;
}

/**
 * Records the instance, show command and window procedure and sets up the window class and title
 * names and the arrow cursor.
 */
void Application::initialize_window_state(void *instance, char *command_line, int32_t show_command)
{
    SetLastError(0);
    shell_command_line = command_line;
    shell_window = 0;
    shell_instance = instance;
    shell_show_command = show_command;
    shell_window_proc = (uint32_t)code_address_shell_window_procedure;
    rasterizer_window_handle = 0;
    shell_window_maximized = 0;
    shell_window_minimized = 0;
    memcpy(shell_window_class_name, "Halo", 5);
    memcpy(shell_window_title, "Halo", 5);
    shell_arrow_cursor = LoadCursorA(0, (const char *)0x7f00);
}

/**
 * On the first run (no FIRSTRUN setting) shows the EULA from eula.dll and exits when it is declined.
 */
void Application::show_eula_on_first_run()
{
    uint32_t firstrun;
    uint32_t firstrun_size;
    int32_t eula_accepted;

    firstrun = 0;
    firstrun_size = 4;
    SettingsStore::current().read_value(SettingsScope::user, "FIRSTRUN", 0, &firstrun, &firstrun_size);
    if (firstrun == 0) {
        eula_accepted = 0;
        {
            DynamicLibrary eula("eula.dll");
            if (eula.loaded()) {
                eula_show_fn eula_show = (eula_show_fn)eula.symbol("EBUEula");
                if (eula_show != 0) {
                    eula_accepted = eula_show("Software\\Microsoft\\Microsoft Games\\Halo", eula_file_name, 0, 1);
                }
            }
        }
        if (eula_accepted == 0) {
            ExitProcess(1);
        }
    }
}

/**
 * Tokenizes the command line and sets the switch globals; safe mode forces the minimal options.
 */
void Application::parse_command_line_flags()
{
    windowed = CommandLine::has_flag("-window", 0) || CommandLine::has_flag("-windowed", 0);
    shell_nosound = CommandLine::has_flag("-nosound", 0);
    network_disabled_flag = CommandLine::has_flag("-nonetwork", 0);
    novideo_or_connect = CommandLine::has_flag("-novideo", 0) || CommandLine::has_flag("-connect", 0);
    nojoystick = CommandLine::has_flag("-nojoystick", 0);
    width640 = CommandLine::has_flag("-width640", 0);
    screenshots = CommandLine::has_flag("-screenshots", 0) || CommandLine::has_flag("-screenshot", 0);
    checkfpu = CommandLine::has_flag("-checkfpu", 0);
    game_time_force_single_tick = CommandLine::has_flag("-timedemo", 0) != 0;
    nowindowskey = CommandLine::has_flag("-nowinkey", 0) || CommandLine::has_flag("-nowindowskey", 0);
    if (safe_mode != 0 || (safe_mode = CommandLine::has_flag("-safemode", 0)) != 0) {
        width640 = 1;
        novideo_or_connect = 1;
        nojoystick = 1;
        windowed = 1;
        shell_nosound = 1;
    }
}

/**
 * Detects the hardware and sizes the sound cache from the physical memory.
 */
void Application::measure_machine()
{
    HardwareProbe::current().detect();
    if (physical_memory <= 0x80) {
        sound_cache_size_megabytes = 12;
    } else if (physical_memory <= 0x100) {
        sound_cache_size_megabytes = 16;
    } else {
        sound_cache_size_megabytes = physical_memory < 0x200 ? 32 : 64;
    }
    sound_cache_unknown_c8 = 0x30;
}

/**
 * Shows the strings.dll usage text for -? or -help. Returns true when it was requested.
 */
bool Application::help_requested()
{
    if (CommandLine::has_flag("-?", 0) || CommandLine::has_flag("-help", 0)) {
        MessageBoxA(0, strings_dll_invalid_text, "Halo", 0);
        return true;
    }
    return false;
}

/**
 * Reserves map memory, loads d3d9.dll, creates the Direct3D object and applies config.txt for the
 * primary adapter, then disables the D3D spy hook when present. Missing pieces are fatal errors.
 */
void Application::load_direct3d_and_config()
{
    d3d9_interface *direct3d9;
    char *config_error;
    void (*disable_d3dspy)(void);

    cache_reserve_map_memory();
    d3d9_module = LoadLibraryA("d3d9.dll");
    direct3d_create9 = GetProcAddress((HMODULE)d3d9_module, "Direct3DCreate9");
    if (d3d9_module == 0 || direct3d_create9 == 0) {
        FatalError::show(0x6b, (uint32_t)((const char *)0x7a), 1);
    }
    direct3d9 = (d3d9_interface *)((direct3d_create9_fn)direct3d_create9)(0x1f);
    if (direct3d9 != 0) {
        config_error = ConfigLoader::parse(0, direct3d9);
        if (config_error != 0) {
            FatalError::show(0xffffffff, (uint32_t)(config_error), 0);
        }
        shell_direct3d = direct3d9;
    } else {
        FatalError::show(0x81, (uint32_t)((const char *)0x82), 1);
    }

    if (GetAsyncKeyState(0x11) < 0) {
        FatalError::show(0x87, (uint32_t)((const char *)0x7e), 0);
    }
    disable_d3dspy = (void (*)(void))GetProcAddress((HMODULE)d3d9_module, "DisableD3DSpy");
    if (disable_d3dspy != 0) {
        disable_d3dspy();
    }
}

/**
 * Loads dsound.dll (unless sound is off), dinput8.dll and shfolder.dll and resolves their entry
 * points; a missing one is a fatal error.
 */
void Application::load_audio_input_libraries()
{
    if (shell_nosound != 0) {
        dsound_module = 0;
        direct_sound_create8 = 0;
    } else {
        dsound_module = LoadLibraryA("dsound.dll");
        direct_sound_create8 = GetProcAddress((HMODULE)dsound_module, "DirectSoundCreate8");
        if (dsound_module == 0 || direct_sound_create8 == 0) {
            FatalError::show(0x7b, (uint32_t)((const char *)0x7a), 1);
        }
    }
    dinput8_module = LoadLibraryA("dinput8.dll");
    direct_input8_create = GetProcAddress((HMODULE)dinput8_module, "DirectInput8Create");
    if (dinput8_module == 0 || direct_input8_create == 0) {
        FatalError::show(0x7c, (uint32_t)((const char *)0x7a), 1);
    }
    shfolder_module = LoadLibraryA("shfolder.dll");
    sh_get_folder_path = GetProcAddress((HMODULE)shfolder_module, "SHGetFolderPathA");
    if (shfolder_module == 0 || sh_get_folder_path == 0) {
        FatalError::show(0x7d, (uint32_t)((const char *)0x7e), 1);
    }
}

/**
 * Checks the minimum memory, CPU speed (re-measured up to twice) and temp disk space, and builds the
 * product id string; each failure is a fatal error.
 */
void Application::check_requirements()
{
    char temp_path[k_shell_path_length];
    large_integer free_bytes_available;

    if (physical_memory < (uint32_t)(required_memory - 0x10)) {
        FatalError::show(0x65, (uint32_t)((const char *)0x6e), 0);
    }
    if (cpu_speed < (uint32_t)required_cpu_speed) {
        HardwareProbe::current().detect();
        if (cpu_speed < (uint32_t)required_cpu_speed) {
            HardwareProbe::current().detect();
            if (cpu_speed < (uint32_t)required_cpu_speed) {
                FatalError::show(0x66, (uint32_t)((const char *)0x6f), 0);
            }
        }
    }
    GetTempPathA(sizeof(temp_path), temp_path);
    GetDiskFreeSpaceExA(temp_path, (PULARGE_INTEGER)&free_bytes_available, 0, 0);
    if (free_bytes_available.parts.high_part <= 0 &&
        (free_bytes_available.parts.high_part < 0 ||
         free_bytes_available.parts.low_part < (uint32_t)(required_disk_space << 20))) {
        FatalError::show(0x6d, (uint32_t)((const char *)0x76), 0);
    }

    shell_product_id = ProductId::build_string();
    if (*shell_product_id == 0) {
        FatalError::show(0xa0, (uint32_t)((const char *)0x7e), 1);
    }
}

/**
 * Applies the -port, -cport and -ip overrides, registers the host start info and runs the main loop,
 * then shuts the engine down.
 */
void Application::run_engine()
{
    const char *port_value;
    const char *ip_value;
    char secret_key[9];

    port_value = 0;
    ip_value = 0;
    network_local_address = 0;
    if (CommandLine::has_flag("-port", &port_value) && port_value != 0) {
        network_game_socket_port = (uint32_t)atol(port_value);
        port_overridden = 1;
    }
    if (CommandLine::has_flag("-cport", &port_value) && port_value != 0) {
        game_cport = (uint32_t)atol(port_value);
        port_overridden = 1;
    }
    if (CommandLine::has_flag("-ip", &ip_value) && ip_value != 0) {
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
    EngineLifecycle::shutdown();
}

/**
 * The body of the guarded session. An 8 KB stack buffer is filled and one of its pages made
 * inaccessible so stray writes into it fault into the crash reporter. Returns false when the run
 * ended early (usage text shown), in which case the exit bookkeeping is skipped.
 */
bool Application::run_session(void *instance, char *command_line, int32_t show_command)
{
    uint32_t stack_guard_buffer[0x800];
    volatile uint8_t integrity_ok;
    char *command_line_copy;
    int32_t i;

    for (i = 0; i < 0x800; i++) {
        stack_guard_buffer[i] = 0xeeeeeeee;
    }
    shell_stack_guard_page = (uint8_t *)stack_guard_buffer + 0x1000;
    VirtualProtect(shell_stack_guard_page, 1, 1, (PDWORD)&shell_stack_guard_old_protect);
    integrity_ok = 1;

    command_line_copy = copy_command_line(command_line);
    initialize_window_state(instance, command_line, show_command);
    show_eula_on_first_run();

    shell_argv = CommandLine::parse_to_argv(command_line_copy, &shell_argc);
    parse_command_line_flags();
    measure_machine();

    if (help_requested()) {
        return false;
    }

    load_direct3d_and_config();
    load_audio_input_libraries();

    if (ExitFlag::previous_run_crashed() != 0) {
        FatalError::show(0x6a, (uint32_t)((const char *)0x73), 0);
    }

    if (integrity_ok == 0) {
        if (CommandLine::has_flag("-testcrash", 0) == 0) {
            MessageBoxA(0, "Corrupted Halo.exe", "Halo", 0);
            ExitProcess(1);
        }
        *(volatile uint8_t *)0 = 0;
    }

    check_requirements();

    KeystoneLibrary::load();

    if (integrity_ok == 0 || EngineLifecycle::initialize() != 0) {
        run_engine();
    }

    memory_global_free(command_line_copy);
    memory_global_free(shell_argv);
    KeystoneLibrary::unload();
    if (shell_stack_guard_page != 0) {
        VirtualProtect(shell_stack_guard_page, 1, shell_stack_guard_old_protect, (PDWORD)&shell_stack_guard_old_protect);
        shell_stack_guard_page = 0;
    }
    return true;
}

/**
 * The game's WinMain: loads the localized strings, takes the single instance mutex, runs the guarded
 * session and then records the clean exit and releases the mutex. An unhandled exception in the
 * session goes to the crash reporter.
 *
 * @address 0x5411e0
 */
int32_t __stdcall Application::winmain(void *instance, void *previous_instance, char *command_line, int32_t show_command)
{
    bool completed;

    (void)previous_instance;

    Localization::initialize();
    SingleInstance::check(k_shell_instance_mode_single);

    completed = true;
    __try {
        completed = run_session(instance, command_line, show_command);
    } __except (CrashReporter::current().handle_exception((win32_exception_pointers *)GetExceptionInformation())) {
    }
    if (!completed) {
        return 0;
    }

    ExitFlag::set_clean();
    SingleInstance::release();
    return 0;
}

}
