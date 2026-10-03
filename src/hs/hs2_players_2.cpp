#include "halo/hs/records.hpp"
#include "halo/hs/hs2_commands.hpp"

#include "objects.h"
#include "effects.h"
#include "game.h"
#include "halo/effects/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/game/api.hpp"

static auto &player_control_globals_ptr = halo::link::ref<player_control_globals *>(halo::game::vars().player_control_globals_ptr);

namespace halo::hs {

/**
 * Evaluate handler of hs function "player_action_test_action"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f2d0
 */
void PlayerCommands::evaluate_player_action_test_action(int16_t function_index, uint32_t thread_index, char first)
{
    player_control_globals_ptr->action_flags_latched |= 1;
    player_control_globals_ptr->action_flags_edge |= 1;
    halo::hs::hs_thread_return((int32_t)(static_cast<uint8_t>(player_control_globals_ptr->action_flags) & 1), thread_index);
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
    int16_t ticks = (int16_t)halo::libm::lrint((double)scaled);

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
    int16_t ticks = (int16_t)halo::libm::lrint((double)scaled);

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
    *(int16_t *)((uint8_t *)player_control_globals_ptr + 0x34) = -1;
    halo::hs::hs_thread_return(0, thread_index);
}

}
