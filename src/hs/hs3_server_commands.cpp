#include "halo/game/legacy_globals.hpp"
#include "halo/hs/hs3_commands.hpp"
#include <stdio.h>
#include "halo/memory/api.hpp"

extern "C" {
extern void hs_thread_return(int32_t value, uint32_t thread_index);
extern char hs_evaluate_variadic_arguments(uint32_t thread_index, int32_t value, uint32_t *out_count, int32_t **out_values);
extern void sv_ban(uint32_t argument_count, int32_t *arguments);
extern void network_banlist_print(void);
extern void chimera__console_out(void *color, char *format, ...);
extern int16_t network_game_mode;
extern void game_engine_begin_end_game_sequence(void);
extern void *global_white_argb;
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first);
extern void game_engine_find_player_by_name(char *source_name);
extern void sv_kick(char *name_or_index);
extern void sv_map(uint32_t argument_count, uint16_t **arguments);
extern void sv_map_reset(void);
extern uint8_t message_delta_parameters_enabled;
extern char message_delta_config_text_buffer[];
extern void message_delta_parameters_protocol_reload_from_config_file(void);
extern void message_delta_definitions_invoke_field_bindings(void);
extern void message_delta_parameters_protocol_send_update(void);
extern void sv_players(void);
extern void sv_status(void);
extern growable_array ban_list;
extern void network_banlist_save(void);
extern void map_list_matching_substring(uint32_t argument_count, int32_t *arguments);
extern void sv_ban_penalty(uint32_t argument_count, int32_t *arguments);
extern void sv_banlist_file(uint32_t argument_count, int32_t *arguments);
extern void sv_friendly_fire(uint32_t argument_count, int32_t *arguments);
extern void sv_maxplayers(uint32_t argument_count, int32_t *arguments);
extern void sv_name(uint32_t argument_count, int32_t *arguments);
extern void sv_password(uint32_t argument_count, int32_t *arguments);
extern void sv_rcon_password(uint32_t argument_count, int32_t *arguments);
extern void sv_single_flag_force_reset(uint32_t argument_count, int32_t *arguments);
extern void sv_timelimit(uint32_t argument_count, int32_t *arguments);
extern void sv_tk_cooldown(uint32_t argument_count, int32_t *arguments);
extern void sv_tk_grace(uint32_t argument_count, int32_t *arguments);
}

namespace halo::hs::part3 {

/**
 * Evaluate handler of the hs script function `sv_ban`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482c60
 */
void ServerCommands::evaluate_sv_ban(int16_t function_index, uint32_t thread_index, char first) const
{
    uint32_t count = 0;
    int32_t *values = 0;

    if (hs_evaluate_variadic_arguments(thread_index, (int32_t)first, &count, &values) != 0) {
        sv_ban(count, values);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sv_banlist`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482a10
 */
void ServerCommands::evaluate_sv_banlist(int16_t function_index, uint32_t thread_index, char first) const
{
    network_banlist_print();
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_end_game`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482bc0
 */
void ServerCommands::evaluate_sv_end_game(int16_t function_index, uint32_t thread_index, char first) const
{
    if (network_game_mode == 2) {
        halo::game::globals::server_end_game_requested = 1;
        game_engine_begin_end_game_sequence();
        chimera__console_out(global_white_argb, (char *)"Server is stopping the game...");
    } else {
        chimera__console_out(global_white_argb, (char *)"sv_end_game is a server-only function!");
    }
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_get_player_action_queue_length`: reads its typed arguments from
 * the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x4825f0
 */
void ServerCommands::evaluate_sv_get_player_action_queue_length(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        game_engine_find_player_by_name((char *)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sv_kick`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482c10
 */
void ServerCommands::evaluate_sv_kick(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        sv_kick((char *)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sv_map`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482ad0
 */
void ServerCommands::evaluate_sv_map(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        sv_map((uint32_t)arguments[0], (uint16_t **)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sv_map_next`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482aa0
 */
void ServerCommands::evaluate_sv_map_next(int16_t function_index, uint32_t thread_index, char first) const
{
    chimera__console_out(0, (char *)"sv_map_next is a dedicated server-only function!");
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_map_reset`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482ac0
 */
void ServerCommands::evaluate_sv_map_reset(int16_t function_index, uint32_t thread_index, char first) const
{
    sv_map_reset();
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_mapcycle`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482e10
 */
void ServerCommands::evaluate_sv_mapcycle(int16_t function_index, uint32_t thread_index, char first) const
{
    chimera__console_out(0, (char *)"sv_mapcycle is a dedicated server-only function!");
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_mapcycle_add`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482e30
 */
void ServerCommands::evaluate_sv_mapcycle_add(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        chimera__console_out(0, (char *)"sv_mapcycle_add is a dedicated server-only function!");
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sv_mapcycle_begin`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482df0
 */
void ServerCommands::evaluate_sv_mapcycle_begin(int16_t function_index, uint32_t thread_index, char first) const
{
    chimera__console_out(0, (char *)"sv_mapcycle_begin is a dedicated server-only function!");
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_mapcycle_del`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482e80
 */
void ServerCommands::evaluate_sv_mapcycle_del(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        chimera__console_out(0, (char *)"sv_mapcycle_del is a dedicated server-only function!");
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sv_parameters_dump`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482510
 */
void ServerCommands::evaluate_sv_parameters_dump(int16_t function_index, uint32_t thread_index, char first) const
{
    if (message_delta_parameters_enabled != 0 && message_delta_parameters_enabled == 1) {
        FILE *file = fopen("parameters.cfg", "wb");

        if (file != 0) {
            fprintf(file, message_delta_config_text_buffer);
            fclose(file);
        }
    }
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_parameters_reload`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x4824e0
 */
void ServerCommands::evaluate_sv_parameters_reload(int16_t function_index, uint32_t thread_index, char first) const
{
    if (message_delta_parameters_enabled != 0) {
        message_delta_parameters_protocol_reload_from_config_file();
        message_delta_definitions_invoke_field_bindings();
        message_delta_parameters_protocol_send_update();
    }
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_players`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482c00
 */
void ServerCommands::evaluate_sv_players(int16_t function_index, uint32_t thread_index, char first) const
{
    sv_players();
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_status`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482c50
 */
void ServerCommands::evaluate_sv_status(int16_t function_index, uint32_t thread_index, char first) const
{
    sv_status();
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_unban`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482a20
 */
void ServerCommands::evaluate_sv_unban(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int32_t index = arguments[0];

        if (index >= 0 && index < ban_list.count) {
            chimera__console_out(0, (char *)"Unbanning %s.", (uint8_t *)ban_list.data + index * 0x38);
            halo::memory::growable_array_remove_element(&ban_list, (uint32_t)index);
            network_banlist_save();
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for the map_list_matching_substring external HS function: decodes its single argument and
 * forwards (evaluated_count, value) to map_list_matching_substring, then pops the evaluation frame.
 *
 * @address 0x483010
 */
void ServerCommands::map_list_matching_substring_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        map_list_matching_substring(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

/**
 * Evaluate handler for the sv_ban_penalty external HS function: decodes its single argument and forwards it
 * straight to sv_ban_penalty, then pops the evaluation frame.
 *
 * @address 0x482cb0
 */
void ServerCommands::sv_ban_penalty_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_ban_penalty(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

/**
 * Evaluate handler for the sv_banlist_file external HS function: decodes its single argument and forwards it
 * straight to sv_banlist_file, then pops the evaluation frame.
 *
 * @address 0x482da0
 */
void ServerCommands::sv_banlist_file_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_banlist_file(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

/**
 * Evaluate handler for the sv_friendly_fire external HS function: decodes its single argument and forwards it
 * straight to sv_friendly_fire, then pops the evaluation frame.
 *
 * @address 0x482f70
 */
void ServerCommands::sv_friendly_fire_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_friendly_fire(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

/**
 * Evaluate handler for the sv_maxplayers external HS function: decodes its single argument and forwards it
 * straight to sv_maxplayers, then pops the evaluation frame.
 *
 * @address 0x482b70
 */
void ServerCommands::sv_maxplayers_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_maxplayers(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

/**
 * Evaluate handler for the sv_name external HS function: decodes its single argument and forwards it straight to
 * sv_name, then pops the evaluation frame.
 *
 * @address 0x482ed0
 */
void ServerCommands::sv_name_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_name(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

/**
 * Evaluate handler for the sv_password external HS function: decodes its single argument and forwards it
 * straight to sv_password, then pops the evaluation frame.
 *
 * @address 0x482f20
 */
void ServerCommands::sv_password_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_password(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

/**
 * Evaluate handler for the sv_rcon_password external HS function: decodes its single argument and forwards it
 * straight to sv_rcon_password, then pops the evaluation frame.
 *
 * @address 0x482b20
 */
void ServerCommands::sv_rcon_password_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_rcon_password(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

/**
 * Evaluate handler for the sv_single_flag_force_reset external HS function: decodes its single argument and
 * forwards (evaluated_count, value) to sv_single_flag_force_reset, then pops the evaluation frame.
 *
 * @address 0x4830b0
 */
void ServerCommands::sv_single_flag_force_reset_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_single_flag_force_reset(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

/**
 * Evaluate handler for the sv_timelimit external HS function: decodes its single argument and forwards it
 * straight to sv_timelimit, then pops the evaluation frame.
 *
 * @address 0x482fc0
 */
void ServerCommands::sv_timelimit_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_timelimit(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

/**
 * Evaluate handler for the sv_tk_cooldown external HS function: decodes its single argument and forwards it
 * straight to sv_tk_cooldown, then pops the evaluation frame.
 *
 * @address 0x482d50
 */
void ServerCommands::sv_tk_cooldown_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_tk_cooldown(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

/**
 * Evaluate handler for the sv_tk_grace external HS function: decodes its single argument and forwards it
 * straight to sv_tk_grace, then pops the evaluation frame.
 *
 * @address 0x482d00
 */
void ServerCommands::sv_tk_grace_evaluate(int16_t function_index, datum_index thread, char first) const
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_tk_grace(argument_count, arguments);

        hs_thread_return(0, thread);
    }
}

}
