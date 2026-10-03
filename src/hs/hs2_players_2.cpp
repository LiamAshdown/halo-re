#include "halo/hs/hs2_commands.hpp"

#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" {
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index);
extern uint8_t *player_control_globals_ptr;
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first);
extern player_effect_globals *player_effect_globals_pointer;
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
    hs_thread_return((int32_t)(player_control_globals_ptr[0] & 1), thread_index);
}

/**
 * Evaluate handler of hs function "player_effect_start"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4808e0
 */
void PlayerCommands::evaluate_player_effect_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    float scaled = *(float *)&arguments[1] * 30.0f;
    int16_t ticks = (int16_t)lrint((double)scaled);

    *(int32_t *)&player_effect_globals_pointer->scripted_shake_intensity = arguments[0];
    player_effect_globals_pointer->scripted_shake_ticks = ticks;
    player_effect_globals_pointer->scripted_shake_duration = ticks;
    player_effect_globals_pointer->scripted_shake_flags = (player_effect_globals_pointer->scripted_shake_flags & 0xfffffffd) | 1;
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    float scaled = *(float *)&arguments[0] * 30.0f;
    int16_t ticks = (int16_t)lrint((double)scaled);

    player_effect_globals_pointer->scripted_shake_ticks = ticks;
    player_effect_globals_pointer->scripted_shake_duration = ticks;
    player_effect_globals_pointer->scripted_shake_flags |= 2;
    hs_thread_return(0, thread_index);
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
    hs_thread_return(0, thread_index);
}

}
