#include "halo/hs/hs1_camera_commands.hpp"
#include "camera.h"
#include "halo/sound/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/render/api.hpp"

extern "C" {
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count, int16_t *expected_types, char first);
extern void hs_thread_return(int32_t value, uint32_t thread_index);
extern float camera_script_time_remaining;
extern uint16_t split_screen_quit_prompt_string;
extern uint8_t network_join_error_reason;
extern uint8_t unknown_0071973b;
extern uint8_t *cinematic_screen_effect_state;
extern game_time_globals *game_time;
}

namespace halo::hs {

/**
 * Evaluate handler for hs function "camera_control" (boolean -> void).
 *
 * @address 0x47edf0
 */
void CameraCommands::run_camera_control(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::camera::camera_control(*(uint8_t *)&arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_set".
 *
 * @address 0x47ee40
 */
void CameraCommands::camera_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::camera::camera_debug_start(*(int16_t *)&arguments[0], *(int16_t *)&arguments[1], k_datum_index_none);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_set_animation" (animation_graph, string -> void).
 *
 * @address 0x47eee0
 */
void CameraCommands::camera_set_animation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::camera::camera_script_set_animation((datum_index)arguments[0], (char *)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_set_dead" (unit -> void).
 *
 * @address 0x47ef90
 */
void CameraCommands::camera_set_dead(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != 0xffffffff) {
            halo::camera::globals().camera_script.mode = 3;
            halo::camera::globals().camera_script.changed = 1;
            halo::camera::globals().camera_script.object = (datum_index)arguments[0];
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_set_first_person" (unit -> void).
 *
 * @address 0x47ef30
 */
void CameraCommands::camera_set_first_person(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != 0xffffffff) {
            halo::camera::globals().camera_script.mode = 2;
            halo::camera::globals().camera_script.changed = 1;
            halo::camera::globals().camera_script.object = (datum_index)arguments[0];
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_set_relative" (cutscene_camera_point, short, object -> void).
 *
 * @address 0x47ee90
 */
void CameraCommands::camera_set_relative(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::camera::camera_debug_start(*(int16_t *)&arguments[0], *(int16_t *)&arguments[1], (datum_index)arguments[2]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_time" (no parameters -> short).
 *
 * @address 0x47eff0
 */
void CameraCommands::camera_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)(uint16_t)(int16_t)(int32_t)(camera_script_time_remaining * 30.0f), thread_index);
}

namespace {
const ScriptCommandEntry k_camera_commands_entries[] = {
    {"camera_control", &CameraCommands::run_camera_control},
    {"camera_set", &CameraCommands::camera_set},
    {"camera_set_animation", &CameraCommands::camera_set_animation},
    {"camera_set_dead", &CameraCommands::camera_set_dead},
    {"camera_set_first_person", &CameraCommands::camera_set_first_person},
    {"camera_set_relative", &CameraCommands::camera_set_relative},
    {"camera_time", &CameraCommands::camera_time},
};
constexpr ScriptCommandGroup k_camera_commands_group(k_camera_commands_entries, sizeof(k_camera_commands_entries) / sizeof(k_camera_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by CameraCommands, keyed by script function name.
 */
const ScriptCommandGroup &CameraCommands::commands()
{
    return k_camera_commands_group;
}

/**
 * Evaluate handler for hs function "cinematic_abort" (no parameters -> void).
 *
 * @address 0x47f810
 */
void CinematicCommands::cinematic_abort(int16_t function_index, uint32_t thread_index, char first)
{
    split_screen_quit_prompt_string = 0xffff;
    network_join_error_reason = 0;
    unknown_0071973b = 1;
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cinematic_screen_effect_set_convolution" (short, short, real, real, real
 * -> void).
 *
 * @address 0x4811c0
 */
void CinematicCommands::run_cinematic_screen_effect_set_convolution(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::render::cinematic_screen_effect_set_convolution(*(int16_t *)&arguments[1], *(int16_t *)&arguments[0], *(float *)&arguments[2],
        *(float *)&arguments[3], *(float *)&arguments[4]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_screen_effect_set_filter" (real, real, real, real, boolean,
 * real -> void).
 *
 * @address 0x481220
 */
void CinematicCommands::run_cinematic_screen_effect_set_filter(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::render::cinematic_screen_effect_set_filter(*(float *)&arguments[0], *(float *)&arguments[1], *(float *)&arguments[2],
        *(float *)&arguments[3], *(uint8_t *)&arguments[4], *(float *)&arguments[5]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_screen_effect_set_filter_desaturation_tint" (real, real, real
 * -> void).
 *
 * @address 0x481280
 */
void CinematicCommands::cinematic_screen_effect_set_filter_desaturation_tint(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (cinematic_screen_effect_state != 0) {
        *(int32_t *)(cinematic_screen_effect_state + 0x18) = arguments[1];
        *(int32_t *)(cinematic_screen_effect_state + 0x14) = arguments[0];
        *(int32_t *)(cinematic_screen_effect_state + 0x1c) = arguments[2];
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_screen_effect_set_video" (short, real -> void).
 *
 * @address 0x4812f0
 */
void CinematicCommands::run_cinematic_screen_effect_set_video(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::render::cinematic_screen_effect_set_video((int16_t)arguments[0], *(float *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_screen_effect_start" (boolean -> void).
 *
 * @address 0x481150
 */
void CinematicCommands::cinematic_screen_effect_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (cinematic_screen_effect_state != 0) {
        if (*(uint8_t *)&arguments[0] || !cinematic_screen_effect_state[0x39]) {
            int32_t i;

            for (i = 0; i < 0xe; i++) {
                ((uint32_t *)cinematic_screen_effect_state)[i] = 0;
            }
            cinematic_screen_effect_state[0x39] = 1;
        }
        cinematic_screen_effect_state[0x38] = 1;
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_screen_effect_stop" (no parameters -> void).
 *
 * @address 0x481340
 */
void CinematicCommands::cinematic_screen_effect_stop(int16_t function_index, uint32_t thread_index, char first)
{
    if (cinematic_screen_effect_state != 0) {
        cinematic_screen_effect_state[0x38] = 0;
    }
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cinematic_set_near_clip_distance" (real -> void).
 *
 * @address 0x481360
 */
void CinematicCommands::cinematic_set_near_clip_distance(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (cinematic_screen_effect_state != 0) {
        *(int32_t *)(cinematic_screen_effect_state + 0x74) = arguments[0];
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_set_title" (cutscene_title -> void).
 *
 * @address 0x47f910
 */
void CinematicCommands::cinematic_set_title(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::cutscene::cutscene_title_queue(*(int16_t *)&arguments[0], 0.0f);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_set_title_delayed" (cutscene_title, real -> void).
 *
 * @address 0x47f960
 */
void CinematicCommands::cinematic_set_title_delayed(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::cutscene::cutscene_title_queue(*(int16_t *)&arguments[0], *(float *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_show_letterbox" (boolean -> void).
 *
 * @address 0x47f8b0
 */
void CinematicCommands::cinematic_show_letterbox(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t show = *(uint8_t *)&arguments[0];

    halo::cutscene::globals().cinematic_globals->show_letterbox = show;
    if (show) {
        halo::cutscene::globals().cinematic_globals->letterbox_last_tick = game_time->game_time;
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_skip_start_internal" (no parameters -> void).
 *
 * @address 0x47f840
 */
void CinematicCommands::cinematic_skip_start_internal(int16_t function_index, uint32_t thread_index, char first)
{
    halo::cutscene::globals().cinematic_globals->skip_in_progress = 1;
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cinematic_skip_stop_internal" (no parameters -> void).
 *
 * @address 0x47f860
 */
void CinematicCommands::cinematic_skip_stop_internal(int16_t function_index, uint32_t thread_index, char first)
{
    if (!(halo::cutscene::globals().cinematic_saved_music_gain == -1.0f)) {
        halo::sound::sound_set_music_gain(halo::cutscene::globals().cinematic_saved_music_gain);
        halo::cutscene::globals().cinematic_saved_music_gain = -1.0f;
    }
    halo::cutscene::globals().cinematic_globals->skip_in_progress = 0;
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cinematic_start" (no parameters -> void).
 *
 * @address 0x47f7f0
 */
void CinematicCommands::cinematic_start(int16_t function_index, uint32_t thread_index, char first)
{
    halo::cutscene::cutscene_start();
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cinematic_stop" (no parameters -> void).
 *
 * @address 0x47f800
 */
void CinematicCommands::cinematic_stop(int16_t function_index, uint32_t thread_index, char first)
{
    halo::cutscene::cutscene_stop();
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cinematic_suppress_bsp_object_creation" (boolean -> void).
 *
 * @address 0x47f9b0
 */
void CinematicCommands::cinematic_suppress_bsp_object_creation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::cutscene::globals().cinematic_globals->suppress_bsp_object_creation = *(uint8_t *)&arguments[0];
    hs_thread_return(0, thread_index);
    }
}

namespace {
const ScriptCommandEntry k_cinematic_commands_entries[] = {
    {"cinematic_abort", &CinematicCommands::cinematic_abort},
    {"cinematic_screen_effect_set_convolution", &CinematicCommands::run_cinematic_screen_effect_set_convolution},
    {"cinematic_screen_effect_set_filter", &CinematicCommands::run_cinematic_screen_effect_set_filter},
    {"cinematic_screen_effect_set_filter_desaturation_tint", &CinematicCommands::cinematic_screen_effect_set_filter_desaturation_tint},
    {"cinematic_screen_effect_set_video", &CinematicCommands::run_cinematic_screen_effect_set_video},
    {"cinematic_screen_effect_start", &CinematicCommands::cinematic_screen_effect_start},
    {"cinematic_screen_effect_stop", &CinematicCommands::cinematic_screen_effect_stop},
    {"cinematic_set_near_clip_distance", &CinematicCommands::cinematic_set_near_clip_distance},
    {"cinematic_set_title", &CinematicCommands::cinematic_set_title},
    {"cinematic_set_title_delayed", &CinematicCommands::cinematic_set_title_delayed},
    {"cinematic_show_letterbox", &CinematicCommands::cinematic_show_letterbox},
    {"cinematic_skip_start_internal", &CinematicCommands::cinematic_skip_start_internal},
    {"cinematic_skip_stop_internal", &CinematicCommands::cinematic_skip_stop_internal},
    {"cinematic_start", &CinematicCommands::cinematic_start},
    {"cinematic_stop", &CinematicCommands::cinematic_stop},
    {"cinematic_suppress_bsp_object_creation", &CinematicCommands::cinematic_suppress_bsp_object_creation},
};
constexpr ScriptCommandGroup k_cinematic_commands_group(k_cinematic_commands_entries, sizeof(k_cinematic_commands_entries) / sizeof(k_cinematic_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by CinematicCommands, keyed by script function name.
 */
const ScriptCommandGroup &CinematicCommands::commands()
{
    return k_cinematic_commands_group;
}

}

extern "C" {

void hs_evaluate_camera_control(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CameraCommands::run_camera_control(function_index, thread_index, first);
}

void hs_evaluate_camera_set(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CameraCommands::camera_set(function_index, thread_index, first);
}

void hs_evaluate_camera_set_animation(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CameraCommands::camera_set_animation(function_index, thread_index, first);
}

void hs_evaluate_camera_set_dead(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CameraCommands::camera_set_dead(function_index, thread_index, first);
}

void hs_evaluate_camera_set_first_person(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CameraCommands::camera_set_first_person(function_index, thread_index, first);
}

void hs_evaluate_camera_set_relative(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CameraCommands::camera_set_relative(function_index, thread_index, first);
}

void hs_evaluate_camera_time(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CameraCommands::camera_time(function_index, thread_index, first);
}

void hs_evaluate_cinematic_abort(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_abort(function_index, thread_index, first);
}

void hs_evaluate_cinematic_screen_effect_set_convolution(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::run_cinematic_screen_effect_set_convolution(function_index, thread_index, first);
}

void hs_evaluate_cinematic_screen_effect_set_filter(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::run_cinematic_screen_effect_set_filter(function_index, thread_index, first);
}

void hs_evaluate_cinematic_screen_effect_set_filter_desaturation_tint(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_screen_effect_set_filter_desaturation_tint(function_index, thread_index, first);
}

void hs_evaluate_cinematic_screen_effect_set_video(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::run_cinematic_screen_effect_set_video(function_index, thread_index, first);
}

void hs_evaluate_cinematic_screen_effect_start(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_screen_effect_start(function_index, thread_index, first);
}

void hs_evaluate_cinematic_screen_effect_stop(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_screen_effect_stop(function_index, thread_index, first);
}

void hs_evaluate_cinematic_set_near_clip_distance(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_set_near_clip_distance(function_index, thread_index, first);
}

void hs_evaluate_cinematic_set_title(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_set_title(function_index, thread_index, first);
}

void hs_evaluate_cinematic_set_title_delayed(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_set_title_delayed(function_index, thread_index, first);
}

void hs_evaluate_cinematic_show_letterbox(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_show_letterbox(function_index, thread_index, first);
}

void hs_evaluate_cinematic_skip_start_internal(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_skip_start_internal(function_index, thread_index, first);
}

void hs_evaluate_cinematic_skip_stop_internal(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_skip_stop_internal(function_index, thread_index, first);
}

void hs_evaluate_cinematic_start(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_start(function_index, thread_index, first);
}

void hs_evaluate_cinematic_stop(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_stop(function_index, thread_index, first);
}

void hs_evaluate_cinematic_suppress_bsp_object_creation(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::CinematicCommands::cinematic_suppress_bsp_object_creation(function_index, thread_index, first);
}

}
