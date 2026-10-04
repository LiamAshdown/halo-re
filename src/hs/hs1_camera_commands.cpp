#include "halo/game/constants.hpp"
#include "halo/hs/records.hpp"
#include "halo/hs/script_globals.hpp"
#include "halo/main/main_globals_fields.hpp"
#include "halo/hs/hs1_camera_commands.hpp"
#include "camera.h"
#include "halo/sound/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/render/api.hpp"
#include "halo/main/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/cutscene/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/interface/vars.hpp"

static auto &director_camera_mode = halo::link::ref<int16_t>(halo::hs::vars().director_camera_mode);
static auto &director_camera_target = halo::link::ref<datum_index>(halo::hs::vars().director_camera_target);
static auto &camera_script_time_remaining = halo::link::ref<float>(halo::hs::vars().camera_script_time_remaining);
static auto &split_screen_quit_prompt_string = halo::link::ref<uint16_t>(halo::ui::vars().split_screen_quit_prompt_string);
static auto &cinematic_screen_effect_state = halo::link::ref<uint8_t *>(halo::cutscene::vars().cinematic_screen_effect_state);

namespace halo::hs {

/**
 * Evaluate handler for hs function "camera_control" (boolean -> void).
 *
 * @address 0x47edf0
 */
void CameraCommands::run_camera_control(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::camera::camera_control(halo::hs::argument_byte(arguments[0]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_set".
 *
 * @address 0x47ee40
 */
void CameraCommands::camera_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::camera::camera_debug_start(halo::hs::argument_short(arguments[0]), halo::hs::argument_short(arguments[1]), k_datum_index_none);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_set_animation" (animation_graph, string -> void).
 *
 * @address 0x47eee0
 */
void CameraCommands::camera_set_animation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::camera::camera_script_set_animation((datum_index)arguments[0], halo::hs::argument_string(arguments[1]));
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_set_dead" (unit -> void).
 *
 * @address 0x47ef90
 */
void CameraCommands::camera_set_dead(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            director_camera_mode = 3;
            halo::hs::fields::director_camera_target_changed = 1;
            director_camera_target = (datum_index)arguments[0];
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_set_first_person" (unit -> void).
 *
 * @address 0x47ef30
 */
void CameraCommands::camera_set_first_person(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            director_camera_mode = 2;
            halo::hs::fields::director_camera_target_changed = 1;
            director_camera_target = (datum_index)arguments[0];
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_set_relative" (cutscene_camera_point, short, object -> void).
 *
 * @address 0x47ee90
 */
void CameraCommands::camera_set_relative(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::camera::camera_debug_start(halo::hs::argument_short(arguments[0]), halo::hs::argument_short(arguments[1]), (datum_index)arguments[2]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "camera_time" (no parameters -> short).
 *
 * @address 0x47eff0
 */
void CameraCommands::camera_time(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)(uint16_t)(int16_t)(int32_t)(camera_script_time_remaining * halo::game::k_ticks_per_second_f), thread_index);
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
    split_screen_quit_prompt_string = halo::k_word_none;
    halo::networking::globals().join_error_reason = 0;
    halo::main::fields::revert_map_if_allowed = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cinematic_screen_effect_set_convolution" (short, short, real, real, real
 * -> void).
 *
 * @address 0x4811c0
 */
void CinematicCommands::run_cinematic_screen_effect_set_convolution(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::render::cinematic_screen_effect_set_convolution(halo::hs::argument_short(arguments[1]), halo::hs::argument_short(arguments[0]), halo::hs::argument_real(arguments[2]),
        halo::hs::argument_real(arguments[3]), halo::hs::argument_real(arguments[4]));
    halo::hs::hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::render::cinematic_screen_effect_set_filter(halo::hs::argument_real(arguments[0]), halo::hs::argument_real(arguments[1]), halo::hs::argument_real(arguments[2]),
        halo::hs::argument_real(arguments[3]), halo::hs::argument_byte(arguments[4]), halo::hs::argument_real(arguments[5]));
    halo::hs::hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    if (cinematic_screen_effect_state != 0) {
        *(int32_t *)(cinematic_screen_effect_state + 0x18) = arguments[1];
        *(int32_t *)(cinematic_screen_effect_state + 0x14) = arguments[0];
        *(int32_t *)(cinematic_screen_effect_state + 0x1c) = arguments[2];
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_screen_effect_set_video" (short, real -> void).
 *
 * @address 0x4812f0
 */
void CinematicCommands::run_cinematic_screen_effect_set_video(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::render::cinematic_screen_effect_set_video((int16_t)arguments[0], halo::hs::argument_real(arguments[1]));
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_screen_effect_start" (boolean -> void).
 *
 * @address 0x481150
 */
void CinematicCommands::cinematic_screen_effect_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    if (cinematic_screen_effect_state != 0) {
        if (halo::hs::argument_byte(arguments[0]) || !cinematic_screen_effect_state[0x39]) {
            int32_t i;

            for (i = 0; i < 0xe; i++) {
                ((uint32_t *)cinematic_screen_effect_state)[i] = 0;
            }
            cinematic_screen_effect_state[0x39] = 1;
        }
        cinematic_screen_effect_state[0x38] = 1;
    }
    halo::hs::hs_thread_return(0, thread_index);
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
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cinematic_set_near_clip_distance" (real -> void).
 *
 * @address 0x481360
 */
void CinematicCommands::cinematic_set_near_clip_distance(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    if (cinematic_screen_effect_state != 0) {
        *(int32_t *)(cinematic_screen_effect_state + 0x74) = arguments[0];
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_set_title" (cutscene_title -> void).
 *
 * @address 0x47f910
 */
void CinematicCommands::cinematic_set_title(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::cutscene::cutscene_title_queue(halo::hs::argument_short(arguments[0]), 0.0f);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_set_title_delayed" (cutscene_title, real -> void).
 *
 * @address 0x47f960
 */
void CinematicCommands::cinematic_set_title_delayed(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::cutscene::cutscene_title_queue(halo::hs::argument_short(arguments[0]), halo::hs::argument_real(arguments[1]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "cinematic_show_letterbox" (boolean -> void).
 *
 * @address 0x47f8b0
 */
void CinematicCommands::cinematic_show_letterbox(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    uint8_t show = halo::hs::argument_byte(arguments[0]);

    halo::cutscene::globals().cinematic_globals->show_letterbox = show;
    if (show) {
        halo::cutscene::globals().cinematic_globals->letterbox_last_tick = halo::game::globals().game_time->game_time;
    }
    halo::hs::hs_thread_return(0, thread_index);
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
    halo::hs::hs_thread_return(0, thread_index);
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
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cinematic_start" (no parameters -> void).
 *
 * @address 0x47f7f0
 */
void CinematicCommands::cinematic_start(int16_t function_index, uint32_t thread_index, char first)
{
    halo::cutscene::cutscene_start();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cinematic_stop" (no parameters -> void).
 *
 * @address 0x47f800
 */
void CinematicCommands::cinematic_stop(int16_t function_index, uint32_t thread_index, char first)
{
    halo::cutscene::cutscene_stop();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cinematic_suppress_bsp_object_creation" (boolean -> void).
 *
 * @address 0x47f9b0
 */
void CinematicCommands::cinematic_suppress_bsp_object_creation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::cutscene::globals().cinematic_globals->suppress_bsp_object_creation = halo::hs::argument_byte(arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
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

namespace halo::hs {

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
