#pragma once

#include "halo/shell/platform.hpp"

namespace halo::shell {

/**
 * Engine bring-up and tear-down around the main loop.
 */
class EngineLifecycle {
public:
    static uint8_t initialize();
    static void shutdown();
};

/**
 * The game's process entry: localization, single instance check, the guarded session (command line,
 * hardware detection, library loading, config.txt, requirement checks, the main loop) and the clean
 * exit bookkeeping. Faults inside the session go to the crash reporter.
 */
class Application {
public:
    static int32_t __stdcall winmain(void *instance, void *previous_instance, char *command_line, int32_t show_command);

private:
    static bool run_session(void *instance, char *command_line, int32_t show_command);
    static char *copy_command_line(const char *command_line);
    static void initialize_window_state(void *instance, char *command_line, int32_t show_command);
    static void parse_command_line_flags();
    static void measure_machine();
    static bool help_requested();
    static void load_direct3d_and_config();
    static void load_audio_input_libraries();
    static void check_requirements();
    static void run_engine();
};

}
