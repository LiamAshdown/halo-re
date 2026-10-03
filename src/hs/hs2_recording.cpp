#include "halo/hs/hs2_commands.hpp"

#include "objects.h"
#include "units.h"
#include "cutscene.h"
#include "halo/cutscene/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/networking/api.hpp"

#ifdef __cplusplus
extern "C" {
#endif
extern void player_update_history_play_local_player(int32_t target_update_id);
extern uint8_t playback_requested_00719768;
#ifdef __cplusplus
}
#endif

namespace halo::hs {

/**
 * Evaluate handler of hs function "play_update_history"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480730
 */
void RecordingCommands::evaluate_play_update_history(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::networking::player_update_history_play_local_player(arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "playback"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f6c0
 */
void RecordingCommands::evaluate_playback(int16_t function_index, uint32_t thread_index, char first)
{
    playback_requested_00719768 = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "recording_kill"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b070
 */
void RecordingCommands::evaluate_recording_kill(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    uint8_t *recording = (uint8_t *)halo::cutscene::recorded_animation_find_by_object((datum_index)arguments[0], 0);

    if (recording != 0) {
        recording[0xa] |= 3;
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "recording_play"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47af50
 */
void RecordingCommands::evaluate_recording_play(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_thread_return((int32_t)halo::cutscene::recorded_animation_start((datum_index)arguments[0], *(int16_t *)&arguments[1], 0), thread_index);
    }
}

/**
 * Evaluate handler of hs function "recording_play_and_delete"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47afb0
 */
void RecordingCommands::evaluate_recording_play_and_delete(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_thread_return((int32_t)halo::cutscene::recorded_animation_start((datum_index)arguments[0], *(int16_t *)&arguments[1], 8), thread_index);
    }
}

/**
 * Evaluate handler of hs function "recording_play_and_hover"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b010
 */
void RecordingCommands::evaluate_recording_play_and_hover(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        uint8_t result = halo::cutscene::recorded_animation_start((datum_index)arguments[0], (int16_t)*(uint16_t *)&arguments[1], 0x10);
        halo::hs::hs_thread_return((int32_t)result, thread_index);
    }
}

/**
 * Evaluate handler of hs function "recording_time"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b0c0
 */
void RecordingCommands::evaluate_recording_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];
    uint8_t *recording = (uint8_t *)halo::cutscene::recorded_animation_find_by_object(unit, 0);
    uint16_t ticks = 0;

    if (recording != 0 && *(datum_index *)(recording + 4) == unit) {
        ticks = *(uint16_t *)(recording + 8);
    }
    halo::hs::hs_thread_return((int32_t)ticks, thread_index);
    }
}

/**
 * Table of the hs functions handled by RecordingCommands, in source order.
 */
EvaluateCommandTable RecordingCommands::commands() noexcept
{
    static constexpr EvaluateFn k_commands[] = {
        &RecordingCommands::evaluate_play_update_history,
        &RecordingCommands::evaluate_playback,
        &RecordingCommands::evaluate_recording_kill,
        &RecordingCommands::evaluate_recording_play,
        &RecordingCommands::evaluate_recording_play_and_delete,
        &RecordingCommands::evaluate_recording_play_and_hover,
        &RecordingCommands::evaluate_recording_time,
    };
    return {k_commands, static_cast<uint32_t>(sizeof(k_commands) / sizeof(k_commands[0]))};
}

}
