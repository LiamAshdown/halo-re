#include "halo/hs/hs2_commands.hpp"

#include "objects.h"
#include "units.h"
#include "game.h"
#include "halo/hs/api.hpp"

#ifdef __cplusplus
extern "C" {
#endif
extern player_control_globals *player_control_globals_ptr;
extern void unit_apply_starting_profile(int16_t starting_profile_index, datum_index unit_handle, uint8_t reset_stats);
extern uint8_t *player_effect_globals_pointer;
extern player_globals *local_player_globals;
#ifdef __cplusplus
}
#endif

namespace halo::hs {

/**
 * Evaluate handler of hs function "player_action_test_accept"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f310
 */
void PlayerCommands::evaluate_player_action_test_accept(int16_t function_index, uint32_t thread_index, char first)
{
    player_control_globals_ptr->action_flags_latched |= 4;
    player_control_globals_ptr->action_flags_edge |= 4;
    halo::hs::hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 2) & 1), thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_back"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f350
 */
void PlayerCommands::evaluate_player_action_test_back(int16_t function_index, uint32_t thread_index, char first)
{
    player_control_globals_ptr->action_flags_latched |= 8;
    player_control_globals_ptr->action_flags_edge |= 8;
    halo::hs::hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 3) & 1), thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_grenade_trigger"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f270
 */
void PlayerCommands::evaluate_player_action_test_grenade_trigger(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 5) & 1), thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_jump"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f210
 */
void PlayerCommands::evaluate_player_action_test_jump(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    uint8_t jumped = (uint8_t)((*(uint32_t *)player_control_globals_ptr >> 1) & 1);
    halo::hs::hs_thread_return((int32_t)jumped, thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_look_relative_all_directions"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f450
 */
void PlayerCommands::evaluate_player_action_test_look_relative_all_directions(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)((~*(uint32_t *)player_control_globals_ptr & 0x780) == 0), thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_look_relative_down"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f3c0
 */
void PlayerCommands::evaluate_player_action_test_look_relative_down(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 8) & 1), thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_look_relative_left"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f3f0
 */
void PlayerCommands::evaluate_player_action_test_look_relative_left(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 9) & 1), thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_look_relative_right"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f420
 */
void PlayerCommands::evaluate_player_action_test_look_relative_right(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 10) & 1), thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_look_relative_up"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f390
 */
void PlayerCommands::evaluate_player_action_test_look_relative_up(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 7) & 1), thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_move_relative_all_directions"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f480
 */
void PlayerCommands::evaluate_player_action_test_move_relative_all_directions(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)((~*(uint32_t *)player_control_globals_ptr & 0x7800) == 0), thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_primary_trigger"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f240
 */
void PlayerCommands::evaluate_player_action_test_primary_trigger(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 4) & 1), thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_reset"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f1f0
 */
void PlayerCommands::evaluate_player_action_test_reset(int16_t function_index, uint32_t thread_index, char first)
{
    *(uint32_t *)player_control_globals_ptr = 0;
    player_control_globals_ptr->action_flags_latched = 0;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "player_action_test_zoom"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f2a0
 */
void PlayerCommands::evaluate_player_action_test_zoom(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)((*(uint32_t *)player_control_globals_ptr >> 6) & 1), thread_index);
}

/**
 * Evaluate handler of hs function "player_add_equipment"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f4b0
 */
void PlayerCommands::evaluate_player_add_equipment(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    unit_apply_starting_profile(*(int16_t *)&arguments[1], (datum_index)arguments[0], *(uint8_t *)&arguments[2]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "player_camera_control"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f170
 */
void PlayerCommands::evaluate_player_camera_control(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t enable = *(uint8_t *)&arguments[0];

    if (enable) {
        player_control_globals_ptr->flags &= 0xfffffffe;
    } else {
        player_control_globals_ptr->flags |= 1;
    }
    halo::hs::hs_thread_return((int32_t)enable, thread_index);
    }
}

/**
 * Evaluate handler of hs function "player_effect_set_max_rotation"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480870
 */
void PlayerCommands::evaluate_player_effect_set_max_rotation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    *(float *)(player_effect_globals_pointer + 0x10c) = *(float *)&arguments[0] * 0.017453292f;
    *(float *)(player_effect_globals_pointer + 0x110) = *(float *)&arguments[1] * 0.017453292f;
    *(float *)(player_effect_globals_pointer + 0x114) = *(float *)&arguments[2] * 0.017453292f;
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "player_effect_set_max_translation"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480810
 */
void PlayerCommands::evaluate_player_effect_set_max_translation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    *(int32_t *)(player_effect_globals_pointer + 0x104) = arguments[1];
    *(int32_t *)(player_effect_globals_pointer + 0x100) = arguments[0];
    *(int32_t *)(player_effect_globals_pointer + 0x108) = arguments[2];
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "player_enable_input"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f120
 */
void PlayerCommands::evaluate_player_enable_input(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    local_player_globals->input_disabled = (uint8_t)(*(uint8_t *)&arguments[0] == 0);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "players"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a410
 */
void PlayerCommands::evaluate_players(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)halo::hs::hs_object_list_collect_player_units(), thread_index);
}

/**
 * Table of the hs functions handled by PlayerCommands, in source order.
 */
EvaluateCommandTable PlayerCommands::commands() noexcept
{
    static constexpr EvaluateFn k_commands[] = {
        &PlayerCommands::evaluate_player_action_test_accept,
        &PlayerCommands::evaluate_player_action_test_action,
        &PlayerCommands::evaluate_player_action_test_back,
        &PlayerCommands::evaluate_player_action_test_grenade_trigger,
        &PlayerCommands::evaluate_player_action_test_jump,
        &PlayerCommands::evaluate_player_action_test_look_relative_all_directions,
        &PlayerCommands::evaluate_player_action_test_look_relative_down,
        &PlayerCommands::evaluate_player_action_test_look_relative_left,
        &PlayerCommands::evaluate_player_action_test_look_relative_right,
        &PlayerCommands::evaluate_player_action_test_look_relative_up,
        &PlayerCommands::evaluate_player_action_test_move_relative_all_directions,
        &PlayerCommands::evaluate_player_action_test_primary_trigger,
        &PlayerCommands::evaluate_player_action_test_reset,
        &PlayerCommands::evaluate_player_action_test_zoom,
        &PlayerCommands::evaluate_player_add_equipment,
        &PlayerCommands::evaluate_player_camera_control,
        &PlayerCommands::evaluate_player_effect_set_max_rotation,
        &PlayerCommands::evaluate_player_effect_set_max_translation,
        &PlayerCommands::evaluate_player_effect_start,
        &PlayerCommands::evaluate_player_effect_stop,
        &PlayerCommands::evaluate_player_enable_input,
        &PlayerCommands::evaluate_players,
        &PlayerCommands::evaluate_players_unzoom_all,
    };
    return {k_commands, static_cast<uint32_t>(sizeof(k_commands) / sizeof(k_commands[0]))};
}

}
