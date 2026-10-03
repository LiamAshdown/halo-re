#pragma once

#include <stddef.h>
#include <stdint.h>

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#include "halo/hs/hs1_command.hpp"

namespace halo::hs {

/**
 * Evaluate handlers of the hs console and engine-state commands (cls, crash, core save/load, checkpoints, debug toggles).
 */
class SystemCommands {
public:
    static void change_team(int16_t function_index, uint32_t thread_index, char first);
    static void checkpoint_load(int16_t function_index, uint32_t thread_index, char first);
    static void checkpoint_save(int16_t function_index, uint32_t thread_index, char first);
    static void cls(int16_t function_index, uint32_t thread_index, char first);
    static void connect(int16_t function_index, uint32_t thread_index, char first);
    static void core_load(int16_t function_index, uint32_t thread_index, char first);
    static void core_load_at_startup(int16_t function_index, uint32_t thread_index, char first);
    static void core_save(int16_t function_index, uint32_t thread_index, char first);
    static void crash(int16_t function_index, uint32_t thread_index, char first);
    static void debug_camera_load(int16_t function_index, uint32_t thread_index, char first);
    static void debug_camera_save(int16_t function_index, uint32_t thread_index, char first);
    static void debug_sounds_enable(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

}
