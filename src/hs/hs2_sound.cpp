#include "halo/hs/hs2_commands.hpp"

#include "cache.h"
#include "game.h"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/hs/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/sound/vars.hpp"

static auto &global_sound_effect_object = halo::link::ref<uint8_t *>(halo::game::vars().global_sound_effect_object);
static auto &sound_effects_gain = halo::link::ref<float>(halo::sound::vars().sound_effects_gain);
static auto &sound_master_gain = halo::link::ref<float>(halo::ui::vars().sound_master_gain);
static auto &sound_supplementary_buffers_00746122 = halo::link::ref<int16_t>(halo::hs::vars().sound_supplementary_buffers_00746122);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);

namespace halo::hs {

/**
 * Evaluate handler of hs function "sound_cache_dump_to_file"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f6e0
 */
void SoundCommands::evaluate_sound_cache_dump_to_file(int16_t function_index, uint32_t thread_index, char first)
{
    halo::cache::sound_cache_dump_to_file();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "sound_class_set_gain"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47ffe0
 */
void SoundCommands::evaluate_sound_class_set_gain(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::sound::sound_class_set_gain_by_name((char *)arguments[0], *(float *)&arguments[1], *(int16_t *)&arguments[2]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "sound_eax_enabled"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4815b0
 */
void SoundCommands::evaluate_sound_eax_enabled(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t enabled = 0;

    if (global_sound_effect_object != 0) {
        int32_t mode = *(int32_t *)(global_sound_effect_object + 4);

        enabled = (uint8_t)(mode == 0 || mode == 1 || mode == 2);
    }
    halo::hs::hs_thread_return((int32_t)enabled, thread_index);
}

/**
 * Evaluate handler of hs function "sound_enable"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480030
 */
void SoundCommands::evaluate_sound_enable(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::sound::globals().enabled = *(uint8_t *)&arguments[0];
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "sound_enable_eax"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481560
 */
void SoundCommands::evaluate_sound_enable_eax(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::sound::sound_effects_object_reinitialize((int)*(uint8_t *)&arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "sound_enable_hardware"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481650
 */
void SoundCommands::evaluate_sound_enable_hardware(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::sound::sound_driver_set_eax_enabled((uint8_t)arguments[0], (uint8_t)arguments[1]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "sound_get_effects_gain"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4801a0
 */
void SoundCommands::evaluate_sound_get_effects_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return(*(int32_t *)&sound_effects_gain, thread_index);
}

/**
 * Evaluate handler of hs function "sound_get_gain"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47ac70
 */
void SoundCommands::evaluate_sound_get_gain(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        float *gain = halo::hs::hs_sound_get_gain_reference((char *)arguments[0]);

        halo::hs::hs_thread_return(gain != 0 ? *(int32_t *)gain : 0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "sound_get_master_gain"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4800c0
 */
void SoundCommands::evaluate_sound_get_master_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return(*(int32_t *)&sound_master_gain, thread_index);
}

/**
 * Evaluate handler of hs function "sound_get_music_gain"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480130
 */
void SoundCommands::evaluate_sound_get_music_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return(*(int32_t *)&halo::sound::globals().music_gain, thread_index);
}

/**
 * Evaluate handler of hs function "sound_get_supplementary_buffers"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481720
 */
void SoundCommands::evaluate_sound_get_supplementary_buffers(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)(uint16_t)(sound_supplementary_buffers_00746122), thread_index);
}

/**
 * Evaluate handler of hs function "sound_impulse_start"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fce0
 */
void SoundCommands::evaluate_sound_impulse_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::sound::sound_impulse_start((datum_index)arguments[1], (datum_index)arguments[0], *(float *)&arguments[2]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "sound_impulse_stop"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fda0
 */
void SoundCommands::evaluate_sound_impulse_stop(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index sound = (datum_index)arguments[0];

    if (sound != k_datum_index_none) {
        uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[sound & halo::k_slot_mask].data;

        if (*(datum_index *)(definition + 0x94) != k_datum_index_none) {
            halo::sound::sound_impulse_fade_out(*(datum_index *)(definition + 0x94));
            *(datum_index *)(definition + 0x94) = k_datum_index_none;
            *(datum_index *)(definition + 0x90) = k_datum_index_none;
        }
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "sound_impulse_time"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fd30
 */
void SoundCommands::evaluate_sound_impulse_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index sound = (datum_index)arguments[0];
    int32_t ticks = 0;

    if (sound != k_datum_index_none) {
        int32_t end_time = *(int32_t *)((uint8_t *)halo::cache::globals().tag_instances[sound & halo::k_slot_mask].data + 0x90);

        if (end_time != -1) {
            ticks = end_time - halo::game::globals().game_time->game_time;
            if (ticks <= 0) {
                ticks = 0;
            }
        }
    }
    halo::hs::hs_thread_return(ticks, thread_index);
    }
}

/**
 * Evaluate handler of hs function "sound_looping_predict"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fe20
 */
void SoundCommands::evaluate_sound_looping_predict(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::sound::sound_looping_predict((datum_index)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "sound_looping_set_alternate"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47ff40
 */
void SoundCommands::evaluate_sound_looping_set_alternate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::sound::sound_looping_set_alternate((datum_index)arguments[0], *(uint8_t *)&arguments[1]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Table of the hs functions handled by SoundCommands, in source order.
 */
EvaluateCommandTable SoundCommands::commands() noexcept
{
    static constexpr EvaluateFn k_commands[] = {
        &SoundCommands::evaluate_sound_cache_dump_to_file,
        &SoundCommands::evaluate_sound_class_set_gain,
        &SoundCommands::evaluate_sound_eax_enabled,
        &SoundCommands::evaluate_sound_enable,
        &SoundCommands::evaluate_sound_enable_eax,
        &SoundCommands::evaluate_sound_enable_hardware,
        &SoundCommands::evaluate_sound_get_effects_gain,
        &SoundCommands::evaluate_sound_get_gain,
        &SoundCommands::evaluate_sound_get_master_gain,
        &SoundCommands::evaluate_sound_get_music_gain,
        &SoundCommands::evaluate_sound_get_supplementary_buffers,
        &SoundCommands::evaluate_sound_impulse_start,
        &SoundCommands::evaluate_sound_impulse_stop,
        &SoundCommands::evaluate_sound_impulse_time,
        &SoundCommands::evaluate_sound_looping_predict,
        &SoundCommands::evaluate_sound_looping_set_alternate,
    };
    return {k_commands, static_cast<uint32_t>(sizeof(k_commands) / sizeof(k_commands[0]))};
}

}
