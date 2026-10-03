#include "halo/hs/records.hpp"
#include "halo/hs/hs2_commands.hpp"

#include "objects.h"
#include "effects.h"
#include "halo/effects/api.hpp"
#include "halo/hs/api.hpp"

#ifdef __cplusplus
extern "C" {
#endif
extern uint8_t *player_control_globals_ptr;
extern long lrint(double x);
#ifdef __cplusplus
}
#endif

namespace halo::hs {

/**
 * Evaluate handler of hs function "player_action_test_action"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f2d0
 */
void PlayerCommands::evaluate_player_action_test_action(int16_t function_index, uint32_t thread_index, char first)
{
    *(uint32_t *)(player_control_globals_ptr + 4) |= 1;
    *(uint32_t *)(player_control_globals_ptr + 8) |= 1;
    halo::hs::hs_thread_return((int32_t)(player_control_globals_ptr[0] & 1), thread_index);
}

/**
 * Evaluate handler of hs function "player_effect_start"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4808e0
 */
void PlayerCommands::evaluate_player_effect_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    float scaled = halo::hs::argument_real(arguments[1]) * 30.0f;
    int16_t ticks = (int16_t)lrint((double)scaled);

    *(int32_t *)&halo::effects::globals().player_effect_state->scripted_shake_intensity = arguments[0];
    halo::effects::globals().player_effect_state->scripted_shake_ticks = ticks;
    halo::effects::globals().player_effect_state->scripted_shake_duration = ticks;
    halo::effects::globals().player_effect_state->scripted_shake_flags = (halo::effects::globals().player_effect_state->scripted_shake_flags & 0xfffffffd) | 1;
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "player_effect_stop"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480970
 */
void PlayerCommands::evaluate_player_effect_stop(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    float scaled = halo::hs::argument_real(arguments[0]) * 30.0f;
    int16_t ticks = (int16_t)lrint((double)scaled);

    halo::effects::globals().player_effect_state->scripted_shake_ticks = ticks;
    halo::effects::globals().player_effect_state->scripted_shake_duration = ticks;
    halo::effects::globals().player_effect_state->scripted_shake_flags |= 2;
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "players_unzoom_all"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f100
 */
void PlayerCommands::evaluate_players_unzoom_all(int16_t function_index, uint32_t thread_index, char first)
{
    *(int16_t *)(player_control_globals_ptr + 0x34) = -1;
    halo::hs::hs_thread_return(0, thread_index);
}

}
