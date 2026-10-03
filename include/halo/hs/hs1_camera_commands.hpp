#pragma once

#include <stddef.h>
#include <stdint.h>

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"
#include "game.h"
#include "hs.h"

#include "halo/hs/hs1_command.hpp"

namespace halo::hs {

/**
 * Evaluate handlers of the hs camera_* script commands.
 */
class CameraCommands {
public:
    static void run_camera_control(int16_t function_index, uint32_t thread_index, char first);
    static void camera_set(int16_t function_index, uint32_t thread_index, char first);
    static void camera_set_animation(int16_t function_index, uint32_t thread_index, char first);
    static void camera_set_dead(int16_t function_index, uint32_t thread_index, char first);
    static void camera_set_first_person(int16_t function_index, uint32_t thread_index, char first);
    static void camera_set_relative(int16_t function_index, uint32_t thread_index, char first);
    static void camera_time(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

/**
 * Evaluate handlers of the hs cinematic_* script commands.
 */
class CinematicCommands {
public:
    static void cinematic_abort(int16_t function_index, uint32_t thread_index, char first);
    static void run_cinematic_screen_effect_set_convolution(int16_t function_index, uint32_t thread_index, char first);
    static void run_cinematic_screen_effect_set_filter(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_screen_effect_set_filter_desaturation_tint(int16_t function_index, uint32_t thread_index, char first);
    static void run_cinematic_screen_effect_set_video(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_screen_effect_start(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_screen_effect_stop(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_set_near_clip_distance(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_set_title(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_set_title_delayed(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_show_letterbox(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_skip_start_internal(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_skip_stop_internal(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_start(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_stop(int16_t function_index, uint32_t thread_index, char first);
    static void cinematic_suppress_bsp_object_creation(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

}
