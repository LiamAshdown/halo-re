#include "halo/main/main_globals_fields.hpp"
#include "halo/hs/hs2_commands.hpp"

#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"
#include "halo/core/datum.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/networking/api.hpp"

#ifdef __cplusplus
extern "C" {
#endif
extern network_client_globals *network_client;
extern void network_client_rejoin_check(int8_t machine_player_index);
extern uint8_t console_debug_flag_4;
extern uint8_t *player_effect_globals_pointer;
extern game_time_globals *game_time;
extern void network_game_host_start(char *map_name, char *variant_name, uint8_t disconnect_timeout_flag);
extern uint32_t game_safe_to_pause(void);
extern int16_t pending_difficulty;
extern int16_t local_player_count;
extern uint16_t split_screen_quit_prompt_string;
extern uint8_t main_globals_byte_0071973a;
extern int32_t game_state_revert_time;
extern uint8_t game_safe_to_save(void);
extern uint32_t game_no_player_is_dead(void);
extern uint8_t main_globals_byte_0071973d;
extern uint8_t main_globals_byte_0071973e;
extern int32_t main_globals_dword_00719740;
extern int32_t main_globals_dword_00719744;
extern int16_t main_globals_word_0071974c;
extern int16_t main_globals_word_0071976e;
extern uint8_t main_globals_byte_0071976c;
extern void game_engine_set_variant_by_name(const char *name);
extern uint8_t main_globals_byte_0071974e;
extern uint8_t profile_globals_block[0x60a4];
extern uint8_t ui_event_byte_0071975b;
extern uint8_t split_screen_quit_prompt_armed;
extern void rcon(int32_t argument_count, char **arguments);
extern void player_update_queue_flush_by_name(char *name);
#ifdef __cplusplus
}
#endif

namespace halo::hs {

/**
 * Evaluate handler of hs function "disconnect"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482770
 */
void GameCommands::evaluate_disconnect(int16_t function_index, uint32_t thread_index, char first)
{
    if (halo::networking::globals().client != 0 && halo::networking::globals().game_mode == 1) {
        halo::networking::network_client_rejoin_check(0);
    }
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "error_overflow_suppression"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4807d0
 */
void GameCommands::evaluate_error_overflow_suppression(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        console_debug_flag_4 = (uint8_t)arguments[0];
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "fade_in"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f6f0
 */
void GameCommands::evaluate_fade_in(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    *(int32_t *)(player_effect_globals_pointer + 0xf0) = arguments[1];
    *(int16_t *)(player_effect_globals_pointer + 0xfc) = *(int16_t *)&arguments[3];
    *(int32_t *)(player_effect_globals_pointer + 0xf4) = arguments[2];
    *(int32_t *)(player_effect_globals_pointer + 0xec) = arguments[0];
    player_effect_globals_pointer[0xfe] = 0;
    *(int32_t *)(player_effect_globals_pointer + 0xf8) = game_time->game_time;
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "fade_out"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f770
 */
void GameCommands::evaluate_fade_out(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    *(int32_t *)(player_effect_globals_pointer + 0xf0) = arguments[1];
    *(int16_t *)(player_effect_globals_pointer + 0xfc) = *(int16_t *)&arguments[3];
    *(int32_t *)(player_effect_globals_pointer + 0xf4) = arguments[2];
    *(int32_t *)(player_effect_globals_pointer + 0xec) = arguments[0];
    player_effect_globals_pointer[0xfe] = 1;
    *(int32_t *)(player_effect_globals_pointer + 0xf8) = game_time->game_time;
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "fast_setup_network_server"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4813b0
 */
void GameCommands::evaluate_fast_setup_network_server(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        network_game_host_start((char *)arguments[0], (char *)arguments[1], *(uint8_t *)&arguments[2]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "game_all_quiet"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fa60
 */
void GameCommands::evaluate_game_all_quiet(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)(uint8_t)game_safe_to_pause(), thread_index);
}

/**
 * Evaluate handler of hs function "game_difficulty_get"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f0a0
 */
void GameCommands::evaluate_game_difficulty_get(int16_t function_index, uint32_t thread_index, char first)
{
    int16_t difficulty = halo::main::globals().game_globals->difficulty;

    if (difficulty <= 1) {
        difficulty = 1;
    }
    halo::hs::hs_thread_return((int32_t)(uint16_t)difficulty, thread_index);
}

/**
 * Evaluate handler of hs function "game_difficulty_get_real"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f0d0
 */
void GameCommands::evaluate_game_difficulty_get_real(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)(uint16_t)halo::main::globals().game_globals->difficulty, thread_index);
}

/**
 * Evaluate handler of hs function "game_difficulty_set"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f580
 */
void GameCommands::evaluate_game_difficulty_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t difficulty = (int16_t)arguments[0];

        if (difficulty >= 0 && difficulty < 4) {
            pending_difficulty = difficulty;
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "game_is_cooperative"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47faa0
 */
void GameCommands::evaluate_game_is_cooperative(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)(local_player_count > 1), thread_index);
}

/**
 * Evaluate handler of hs function "game_lost"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fa20
 */
void GameCommands::evaluate_game_lost(int16_t function_index, uint32_t thread_index, char first)
{
    halo::networking::globals().join_error_reason = 0;
    halo::main::fields::lost_map = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "game_revert"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fbd0
 */
void GameCommands::evaluate_game_revert(int16_t function_index, uint32_t thread_index, char first)
{
    halo::networking::globals().join_error_reason = 0;
    halo::main::fields::lost_map = 0;
    split_screen_quit_prompt_string = halo::k_word_none;
    main_globals_byte_0071973a = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "game_reverted"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fcb0
 */
void GameCommands::evaluate_game_reverted(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)(game_state_revert_time == game_time->game_time), thread_index);
}

/**
 * Evaluate handler of hs function "game_safe_to_save"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fa40
 */
void GameCommands::evaluate_game_safe_to_save(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    uint8_t safe = game_safe_to_save();
    halo::hs::hs_thread_return((int32_t)safe, thread_index);
}

/**
 * Evaluate handler of hs function "game_safe_to_speak"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fa80
 */
void GameCommands::evaluate_game_safe_to_speak(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)(uint8_t)game_no_player_is_dead(), thread_index);
}

/**
 * Evaluate handler of hs function "game_save"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fad0
 */
void GameCommands::evaluate_game_save(int16_t function_index, uint32_t thread_index, char first)
{
    if (halo::networking::globals().join_error_reason == 0 || main_globals_byte_0071973e != 0) {
        halo::networking::globals().join_error_reason = 1;
        main_globals_byte_0071973d = 1;
        main_globals_byte_0071973e = 1;
        main_globals_dword_00719740 = 0;
        main_globals_dword_00719744 = 0;
        main_globals_word_0071974c = 0;
    }
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "game_save_cancel"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fb20
 */
void GameCommands::evaluate_game_save_cancel(int16_t function_index, uint32_t thread_index, char first)
{
    halo::networking::globals().join_error_reason = 0;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "game_save_no_timeout"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fb40
 */
void GameCommands::evaluate_game_save_no_timeout(int16_t function_index, uint32_t thread_index, char first)
{
    if (halo::networking::globals().join_error_reason == 0 || main_globals_byte_0071973e != 0) {
        halo::networking::globals().join_error_reason = 1;
        main_globals_byte_0071973d = 1;
        main_globals_dword_00719740 = 0;
        main_globals_dword_00719744 = 0;
        main_globals_word_0071974c = 0;
    }
    main_globals_byte_0071973e = 0;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "game_save_totally_unsafe"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fb90
 */
void GameCommands::evaluate_game_save_totally_unsafe(int16_t function_index, uint32_t thread_index, char first)
{
    halo::networking::globals().join_error_reason = 1;
    main_globals_byte_0071973d = 0;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "game_saving"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fbb0
 */
void GameCommands::evaluate_game_saving(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)halo::networking::globals().join_error_reason, thread_index);
}

/**
 * Evaluate handler of hs function "game_skip_ticks"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fc60
 */
void GameCommands::evaluate_game_skip_ticks(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    int16_t ticks = *(int16_t *)&arguments[0];

    if (ticks <= 0xf) {
        main_globals_word_0071976e = ticks;
        main_globals_byte_0071976c = 1;
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "game_speed"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482930
 */
void GameCommands::evaluate_game_speed(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    game_time->speed = *(float *)&arguments[0];
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "game_variant"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f040
 */
void GameCommands::evaluate_game_variant(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        game_engine_set_variant_by_name((const char *)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "game_won"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fa00
 */
void GameCommands::evaluate_game_won(int16_t function_index, uint32_t thread_index, char first)
{
    halo::networking::globals().join_error_reason = 0;
    main_globals_byte_0071974e = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "garbage_collect_now"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b410
 */
void GameCommands::evaluate_garbage_collect_now(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    halo::objects::globals().object_globals->garbage_collect_requested = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "map_name"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482980
 */
void GameCommands::evaluate_map_name(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::main::main_queue_map_change((char *)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "map_reset"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f500
 */
void GameCommands::evaluate_map_reset(int16_t function_index, uint32_t thread_index, char first)
{
    halo::networking::globals().join_error_reason = 0;
    halo::main::fields::lost_map = 0;
    split_screen_quit_prompt_string = halo::k_word_none;
    halo::main::fields::reset_map = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "multiplayer_map_name"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47f530
 */
void GameCommands::evaluate_multiplayer_map_name(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::main::main_queue_map_change_by_name_or_clear((char *)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "profile_load"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4827a0
 */
void GameCommands::evaluate_profile_load(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::saved_games::saved_game_delete_by_display_name((const char *)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "profile_unlock_solo_levels"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481400
 */
void GameCommands::evaluate_profile_unlock_solo_levels(int16_t function_index, uint32_t thread_index, char first)
{
    int32_t level;

    for (level = 0; level < 10; level++) {
        profile_globals_block[0x11e + level] |= 0xf;
    }
    profile_globals_block[0x11c] |= 4;
    if (halo::saved_games::globals().player_profile_slots_handle != -1) {
        halo::saved_games::player_profile_write_data(halo::saved_games::globals().player_profile_slots_handle, (saved_player_profile *)profile_globals_block);
    }
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "quit"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482820
 */
void GameCommands::evaluate_quit(int16_t function_index, uint32_t thread_index, char first)
{
    ui_event_byte_0071975b = 1;
    halo::main::globals().movie_playback_abort = 1;
    split_screen_quit_prompt_armed = 0;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "rcon"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4829c0
 */
void GameCommands::evaluate_rcon(int16_t function_index, uint32_t thread_index, char first)
{
    uint32_t count = 0;
    int32_t *values = 0;

    if (halo::hs::hs_evaluate_variadic_arguments(thread_index, (int32_t)first, &count, &values) != 0) {
        halo::networking::rcon((int32_t)count, (char **)values);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "remote_player_stats"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4825b0
 */
void GameCommands::evaluate_remote_player_stats(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::networking::player_update_queue_flush_by_name((char *)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Table of the hs functions handled by GameCommands, in source order.
 */
EvaluateCommandTable GameCommands::commands() noexcept
{
    static constexpr EvaluateFn k_commands[] = {
        &GameCommands::evaluate_disconnect,
        &GameCommands::evaluate_error_overflow_suppression,
        &GameCommands::evaluate_fade_in,
        &GameCommands::evaluate_fade_out,
        &GameCommands::evaluate_fast_setup_network_server,
        &GameCommands::evaluate_game_all_quiet,
        &GameCommands::evaluate_game_difficulty_get,
        &GameCommands::evaluate_game_difficulty_get_real,
        &GameCommands::evaluate_game_difficulty_set,
        &GameCommands::evaluate_game_is_cooperative,
        &GameCommands::evaluate_game_lost,
        &GameCommands::evaluate_game_revert,
        &GameCommands::evaluate_game_reverted,
        &GameCommands::evaluate_game_safe_to_save,
        &GameCommands::evaluate_game_safe_to_speak,
        &GameCommands::evaluate_game_save,
        &GameCommands::evaluate_game_save_cancel,
        &GameCommands::evaluate_game_save_no_timeout,
        &GameCommands::evaluate_game_save_totally_unsafe,
        &GameCommands::evaluate_game_saving,
        &GameCommands::evaluate_game_skip_ticks,
        &GameCommands::evaluate_game_speed,
        &GameCommands::evaluate_game_time,
        &GameCommands::evaluate_game_variant,
        &GameCommands::evaluate_game_won,
        &GameCommands::evaluate_garbage_collect_now,
        &GameCommands::evaluate_map_name,
        &GameCommands::evaluate_map_reset,
        &GameCommands::evaluate_multiplayer_map_name,
        &GameCommands::evaluate_profile_load,
        &GameCommands::evaluate_profile_unlock_solo_levels,
        &GameCommands::evaluate_quit,
        &GameCommands::evaluate_rcon,
        &GameCommands::evaluate_remote_player_stats,
    };
    return {k_commands, static_cast<uint32_t>(sizeof(k_commands) / sizeof(k_commands[0]))};
}

}
