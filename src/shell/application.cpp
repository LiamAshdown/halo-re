#include "win32.h"
#include "halo/shell/application.hpp"
#include "halo/shell/messages.hpp"
#include "halo/shell/layout.hpp"
#include "halo/shell/config.hpp"
#include "halo/shell/diagnostics.hpp"
#include "halo/shell/hardware.hpp"
#include "halo/shell/system.hpp"
#include "halo/shell/window.hpp"
#include "halo/platform/audio.hpp"
#include <excpt.h>
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/render/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/cseries/cseries.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/devices.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/saved_games/vars.hpp"
#include "halo/shell/vars.hpp"
#include "halo/hs/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/platform/time.hpp"
#include "halo/platform/file.hpp"
#include "halo/platform/memory.hpp"
#include "halo/platform/system.hpp"

static auto &shell_command_line = halo::link::ref<char *>(halo::shell::vars().shell_command_line);
static auto &shell_window = halo::link::ref<void *>(halo::shell::vars().shell_window);
static auto &shell_instance = halo::link::ref<void *>(halo::shell::vars().shell_instance);
static auto &shell_show_command = halo::link::ref<int32_t>(halo::shell::vars().shell_show_command);
static auto &shell_window_proc = halo::link::ref<uint32_t>(halo::rasterizer::vars().shell_window_proc);
static auto &shell_window_maximized = halo::link::ref<uint8_t>(halo::shell::vars().shell_window_maximized);
static auto &shell_window_minimized = halo::link::ref<uint8_t>(halo::shell::vars().shell_window_minimized);
static auto &shell_window_class_name = halo::link::ref<char [k_shell_window_name_length]>(halo::rasterizer::vars().shell_window_class_name);
static auto &shell_window_title = halo::link::ref<char [k_shell_window_name_length]>(halo::rasterizer::vars().shell_window_title);
static auto &shell_arrow_cursor = halo::link::ref<void *>(halo::shell::vars().shell_arrow_cursor);
static auto &shell_argv = halo::link::ref<char **>(halo::shell::vars().shell_argv);
static auto &shell_argc = halo::link::ref<int32_t>(halo::shell::vars().shell_argc);
static auto &shell_direct3d = halo::link::ref<void *>(halo::rasterizer::vars().shell_direct3d);
static auto &d3d9_module = halo::link::ref<void *>(halo::shell::vars().d3d9_module);
static auto &direct3d_create9 = halo::link::ref<void *>(halo::shell::vars().direct3d_create9);
static auto &dsound_module = halo::link::ref<void *>(halo::shell::vars().dsound_module);
static auto &direct_sound_create8 = halo::link::ref<void *>(halo::shell::vars().direct_sound_create8);
static auto &shfolder_module = halo::link::ref<void *>(halo::shell::vars().shfolder_module);
static auto &sh_get_folder_path = halo::link::ref<void *>(halo::shell::vars().sh_get_folder_path);
static auto &screenshots = halo::link::ref<int32_t>(halo::main::vars().screenshots);
static auto &shell_nosound = halo::link::ref<int32_t>(halo::shell::vars().shell_nosound);
static auto &novideo_or_connect = halo::link::ref<int32_t>(halo::main::vars().novideo_or_connect);
static auto &network_disabled_flag = halo::link::ref<int32_t>(halo::ui::vars().network_disabled_flag);
static auto &width640 = halo::link::ref<int32_t>(halo::rasterizer::vars().width640);
static auto &safe_mode = halo::link::ref<int32_t>(halo::shell::vars().safe_mode);
static auto &nowindowskey = halo::link::ref<int32_t>(halo::shell::vars().nowindowskey);
static auto &nojoystick = halo::link::ref<int32_t>(halo::shell::vars().nojoystick);
static auto &checkfpu = halo::link::ref<int32_t>(halo::main::vars().checkfpu);
static auto &windowed = halo::link::ref<int32_t>(halo::rasterizer::vars().windowed);
static auto &physical_memory = halo::link::ref<uint32_t>(halo::shell::vars().physical_memory);
static auto &sound_cache_size_megabytes = halo::link::ref<int16_t>(halo::shell::vars().sound_cache_size_megabytes);
static auto &sound_cache_unknown_c8 = halo::link::ref<int16_t>(halo::shell::vars().sound_cache_unknown_c8);
static auto &required_memory = halo::link::ref<int32_t>(halo::shell::vars().required_memory);
static auto &cpu_speed = halo::link::ref<uint32_t>(halo::shell::vars().cpu_speed);
static auto &required_cpu_speed = halo::link::ref<int32_t>(halo::shell::vars().required_cpu_speed);
static auto &required_disk_space = halo::link::ref<int32_t>(halo::shell::vars().required_disk_space);
static auto &shell_product_id = halo::link::ref<char *>(halo::networking::vars().shell_product_id);
static auto &shell_stack_guard_page = halo::link::ref<void *>(halo::shell::vars().shell_stack_guard_page);
static auto &shell_stack_guard_old_protect = halo::link::ref<uint32_t>(halo::shell::vars().shell_stack_guard_old_protect);
static auto &game_cport = halo::link::ref<uint32_t>(halo::ui::vars().game_cport);
static auto &port_overridden = halo::link::ref<uint8_t>(halo::ui::vars().port_overridden);
static auto &network_local_address = halo::link::ref<uint32_t>(halo::networking::vars().network_local_address);
static auto &connect_address = halo::link::ref<uint32_t>(halo::shell::vars().connect_address);
static auto &profile_directory = halo::link::ref<char [0x105]>(halo::saved_games::vars().profile_directory);
static auto &console_debug_flag_0 = halo::link::ref<uint8_t>(halo::main::vars().console_debug_flag_0);
static auto &error_file_enabled = halo::link::ref<uint8_t>(halo::shell::vars().error_file_enabled);
static auto &console_debug_flag_4 = halo::link::ref<uint8_t>(halo::shell::vars().console_debug_flag_4);
static auto &console_debug_flag_5 = halo::link::ref<uint8_t>(halo::main::vars().console_debug_flag_5);
static auto &console_debug_word_8 = halo::link::ref<uint16_t>(halo::main::vars().console_debug_word_8);
static auto &global_scenario_index = halo::link::ref<uint32_t>(halo::game::vars().global_scenario_index);
static auto &global_structure_bsp_index = halo::link::ref<uint16_t>(halo::game::vars().global_structure_bsp_index);
static auto &global_scenario_game_globals = halo::link::ref<uint16_t *>(halo::shell::vars().global_scenario_game_globals);
static auto &global_scenario = halo::link::ref<uint32_t>(halo::hs::vars().global_scenario);
static auto &global_structure_collision_bsp = halo::link::ref<uint32_t>(halo::physics::vars().global_structure_collision_bsp);
static auto &global_collision_bsp = halo::link::ref<uint32_t>(halo::physics::vars().global_collision_bsp);
static auto &global_globals = halo::link::ref<uint32_t>(halo::game::vars().global_globals);
static auto &external_00686b4c = halo::link::ref<uint32_t>(halo::shell::vars().external_00686b4c);
static auto &external_00686b50 = halo::link::ref<uint8_t>(halo::shell::vars().external_00686b50);
static auto &external_00686b58 = halo::link::ref<void *>(halo::shell::vars().external_00686b58);
static auto &external_00686b5c = halo::link::ref<void *>(halo::shell::vars().external_00686b5c);
static auto &external_00686b54 = halo::link::ref<uint32_t>(halo::shell::vars().external_00686b54);

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

    halo::platform::timer_resolution_begin(1);
    halo::platform::read_performance_frequency(&halo::cseries::globals().performance_frequency);

    for (i = 0; i < k_profile_directory_buffer_size; i++) {
        profile_directory[i] = 0;
    }

    halo::cseries::profile_path::initialize();

    if (direct3d_create9 == 0) {
        d3d9_module = halo::platform::library_open("d3d9.dll");
        direct3d_create9 = halo::platform::library_symbol(d3d9_module, "Direct3DCreate9");

        if (shell_nosound == 0) {
            dsound_module = 0;
            direct_sound_create8 = (void *)halo::platform::audio_device_create;
        } else {
            dsound_module = 0;
            direct_sound_create8 = 0;
        }

        shfolder_module = halo::platform::library_open("shfolder.dll");
        sh_get_folder_path = halo::platform::library_symbol(shfolder_module, "SHGetFolderPathA");
    }

    halo::cseries::directory_create_recursive(profile_directory);

    halo::cseries::globals().debug_log_level = 0;
    error_file_enabled = 1;
    console_debug_flag_4 = 1;
    console_debug_flag_5 = 0;
    console_debug_flag_0 = 0;
    console_debug_word_8 = 0;

    halo::cache::data_file_open();
    halo::math::math_initialize();
    halo::saved_games::game_state_startup();

    startup_ok = halo::render::render_initialize();
    if ((uint8_t)startup_ok != 0) {
        halo::input::InputDevices::initialize();
        halo::sound::globals().disabled = (uint8_t)shell_nosound;
        halo::sound::sound_initialize();
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
    halo::cache::cache_file_unload();
    global_scenario_index = k_dword_none;
    global_structure_bsp_index = k_word_none;
    *global_scenario_game_globals = k_word_none;
    global_scenario = 0;
    halo::scenario::globals().structure_bsp = 0;
    global_structure_collision_bsp = 0;
    global_collision_bsp = 0;
    global_globals = 0;

    halo::input::InputDevices::release();
    halo::rasterizer::rasterizer_shutdown();
    halo::platform::heap_free(halo::math::globals().sphere_point_table);
    halo::math::periodic_function_tables_free();
    halo::cache::data_file_close();
    halo::sound::sound_dispose();

    external_00686b4c = k_dword_none;
    external_00686b50 = 0;
    if (external_00686b58 != 0) {
        halo::platform::heap_free(external_00686b58);
    }
    if (external_00686b5c != 0) {
        halo::platform::heap_free(external_00686b5c);
    }
    external_00686b54 = 0;
    external_00686b58 = 0;
    external_00686b5c = 0;

    halo::platform::timer_resolution_end(1);
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
    command_line_copy = (char *)halo::cseries::global_memory::alloc((uint32_t)command_line_length + 1);
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
    halo::platform::set_last_error(0);
    shell_command_line = command_line;
    shell_window = 0;
    shell_instance = instance;
    shell_show_command = show_command;
    halo::rasterizer::globals().window_handle = 0;
    shell_window_maximized = 0;
    shell_window_minimized = 0;
    memcpy(shell_window_class_name, "Halo", 5);
    memcpy(shell_window_title, "Halo", 5);
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
    halo::game::globals().time_force_single_tick = CommandLine::has_flag("-timedemo", 0) != 0;
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
    if (physical_memory <= k_memory_class_small_mb) {
        sound_cache_size_megabytes = 12;
    } else if (physical_memory <= k_memory_class_medium_mb) {
        sound_cache_size_megabytes = 16;
    } else {
        sound_cache_size_megabytes = physical_memory < k_memory_class_large_mb ? 32 : 64;
    }
    sound_cache_unknown_c8 = 0x30;
}

/**
 * Shows the command-line usage text for -? or -help. Returns true when it was requested.
 */
bool Application::help_requested()
{
    if (CommandLine::has_flag("-?", 0) || CommandLine::has_flag("-help", 0)) {
        halo::platform::message_box(0, halo::shell::shell_usage_text(), "Halo", 0);
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

    halo::cache::cache_reserve_map_memory();
    d3d9_module = halo::platform::library_open("d3d9.dll");
    direct3d_create9 = halo::platform::library_symbol(d3d9_module, "Direct3DCreate9");
    if (d3d9_module == 0 || direct3d_create9 == 0) {
        FatalError::show(k_string_d3d9_missing, k_help_file_directx, 1);
    }
    direct3d9 = (d3d9_interface *)((direct3d_create9_fn)direct3d_create9)(k_d3d_sdk_version);
    if (direct3d9 != 0) {
        config_error = ConfigLoader::parse(0, direct3d9);
        if (config_error != 0) {
            FatalError::show(k_dword_none, (uint32_t)(config_error), 0);
        }
        shell_direct3d = direct3d9;
    } else {
        FatalError::show(k_string_direct3d_create_failed, k_help_file_direct3d, 1);
    }

    if (GetAsyncKeyState(k_vk_control) < 0) {
        FatalError::show(k_string_safe_mode_requested, k_help_file_general, 0);
    }
    disable_d3dspy = (void (*)(void))halo::platform::library_symbol(d3d9_module, "DisableD3DSpy");
    if (disable_d3dspy != 0) {
        disable_d3dspy();
    }
}

/**
 * Points the sound code at the platform's audio device (unless sound is off) and loads shfolder.dll; a missing
 * shfolder.dll is a fatal error.
 */
void Application::load_audio_input_libraries()
{
    if (shell_nosound != 0) {
        dsound_module = 0;
        direct_sound_create8 = 0;
    } else {
        dsound_module = 0;
        direct_sound_create8 = (void *)halo::platform::audio_device_create;
    }
    shfolder_module = halo::platform::library_open("shfolder.dll");
    sh_get_folder_path = halo::platform::library_symbol(shfolder_module, "SHGetFolderPathA");
    if (shfolder_module == 0 || sh_get_folder_path == 0) {
        FatalError::show(k_string_shfolder_missing, k_help_file_general, 1);
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
        FatalError::show(k_string_insufficient_memory, k_help_file_memory, 0);
    }
    if (cpu_speed < (uint32_t)required_cpu_speed) {
        HardwareProbe::current().detect();
        if (cpu_speed < (uint32_t)required_cpu_speed) {
            HardwareProbe::current().detect();
            if (cpu_speed < (uint32_t)required_cpu_speed) {
                FatalError::show(k_string_insufficient_cpu, k_help_file_cpu, 0);
            }
        }
    }
    halo::platform::temp_directory(temp_path, sizeof(temp_path));
    halo::platform::disk_free_space(temp_path, reinterpret_cast<uint64_t *>(&free_bytes_available), nullptr, nullptr);
    if (free_bytes_available.parts.high_part <= 0 &&
        (free_bytes_available.parts.high_part < 0 ||
         free_bytes_available.parts.low_part < (uint32_t)(required_disk_space << 20))) {
        FatalError::show(k_string_insufficient_disk_space, k_help_file_disk_space, 0);
    }

    shell_product_id = ProductId::build_string();
    if (*shell_product_id == 0) {
        FatalError::show(k_string_product_id_missing, k_help_file_general, 1);
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
        halo::networking::globals().game_socket_port = (uint32_t)atol(port_value);
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
    halo::networking::network_session_host_start_info_set("halor", secret_key, (char *)ip_value, (int32_t)halo::networking::globals().game_socket_port);
    halo::main::main_loop();
    EngineLifecycle::shutdown();
}

/**
 * The body of the guarded session. An 8 KB stack buffer is filled and one of its pages made
 * inaccessible so stray writes into it fault into the crash reporter. Returns false when the run
 * ended early (usage text shown), in which case the exit bookkeeping is skipped.
 */
bool Application::run_session(void *instance, char *command_line, int32_t show_command)
{
    uint32_t stack_guard_buffer[k_stack_guard_words];
    volatile uint8_t integrity_ok;
    char *command_line_copy;
    int32_t i;

    for (i = 0; i < k_stack_guard_words; i++) {
        stack_guard_buffer[i] = k_stack_guard_marker;
    }
    shell_stack_guard_page = (uint8_t *)stack_guard_buffer + k_stack_guard_page_offset;
    halo::platform::memory_protect(shell_stack_guard_page, 1, 1, &shell_stack_guard_old_protect);
    integrity_ok = 1;

    command_line_copy = copy_command_line(command_line);
    initialize_window_state(instance, command_line, show_command);

    shell_argv = CommandLine::parse_to_argv(command_line_copy, &shell_argc);
    parse_command_line_flags();
    measure_machine();

    if (help_requested()) {
        return false;
    }

    load_direct3d_and_config();
    load_audio_input_libraries();

    if (ExitFlag::previous_run_crashed() != 0) {
        FatalError::show(k_string_previous_run_crashed, k_help_file_crash, 0);
    }

    if (integrity_ok == 0) {
        if (CommandLine::has_flag("-testcrash", 0) == 0) {
            halo::platform::message_box(0, "Corrupted Halo.exe", "Halo", 0);
            halo::platform::process_exit(1);
        }
        *(volatile uint8_t *)0 = 0;
    }

    check_requirements();

    CodepageLocale::apply();

    if (integrity_ok == 0 || EngineLifecycle::initialize() != 0) {
        run_engine();
    }

    halo::cseries::global_memory::release(command_line_copy);
    halo::cseries::global_memory::release(shell_argv);
    if (shell_stack_guard_page != 0) {
        halo::platform::memory_protect(shell_stack_guard_page, 1, shell_stack_guard_old_protect, &shell_stack_guard_old_protect);
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
