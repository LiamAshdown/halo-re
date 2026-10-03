#include "halo/hs/records.hpp"
#include "halo/game/legacy_globals.hpp"
#include "halo/hs/hs3_commands.hpp"
#include <stdio.h>
#include "halo/memory/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern void sv_ban(uint32_t argument_count, int32_t *arguments);
extern void network_banlist_print(void);
extern void chimera__console_out(void *color, char *format, ...);
extern void game_engine_begin_end_game_sequence(void);
extern void *global_white_argb;
extern void game_engine_find_player_by_name(char *source_name);
extern uint8_t message_delta_parameters_enabled;
extern char message_delta_config_text_buffer[];
extern growable_array ban_list;
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

    if (halo::hs::hs_evaluate_variadic_arguments(thread_index, (int32_t)first, &count, &values) != 0) {
        halo::networking::sv_ban(count, values);
        halo::hs::hs_thread_return(0, thread_index);
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
    halo::networking::network_banlist_print();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_end_game`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482bc0
 */
void ServerCommands::evaluate_sv_end_game(int16_t function_index, uint32_t thread_index, char first) const
{
    if (halo::networking::globals().game_mode == 2) {
        halo::game::fields::server_end_game_requested = 1;
        halo::game::game_engine_begin_end_game_sequence();
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, const_cast<char *>("Server is stopping the game..."));
    } else {
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, const_cast<char *>("sv_end_game is a server-only function!"));
    }
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_get_player_action_queue_length`: reads its typed arguments from
 * the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x4825f0
 */
void ServerCommands::evaluate_sv_get_player_action_queue_length(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::game::game_engine_find_player_by_name(halo::hs::argument_string(arguments[0]));
        halo::hs::hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::networking::sv_kick(halo::hs::argument_string(arguments[0]));
        halo::hs::hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::networking::sv_map((uint32_t)arguments[0], (uint16_t **)arguments[1]);
        halo::hs::hs_thread_return(0, thread_index);
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
    halo::interface::chimera__console_out(0, const_cast<char *>("sv_map_next is a dedicated server-only function!"));
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_map_reset`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482ac0
 */
void ServerCommands::evaluate_sv_map_reset(int16_t function_index, uint32_t thread_index, char first) const
{
    halo::networking::sv_map_reset();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_mapcycle`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482e10
 */
void ServerCommands::evaluate_sv_mapcycle(int16_t function_index, uint32_t thread_index, char first) const
{
    halo::interface::chimera__console_out(0, const_cast<char *>("sv_mapcycle is a dedicated server-only function!"));
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_mapcycle_add`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482e30
 */
void ServerCommands::evaluate_sv_mapcycle_add(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::interface::chimera__console_out(0, const_cast<char *>("sv_mapcycle_add is a dedicated server-only function!"));
        halo::hs::hs_thread_return(0, thread_index);
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
    halo::interface::chimera__console_out(0, const_cast<char *>("sv_mapcycle_begin is a dedicated server-only function!"));
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_mapcycle_del`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482e80
 */
void ServerCommands::evaluate_sv_mapcycle_del(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::interface::chimera__console_out(0, const_cast<char *>("sv_mapcycle_del is a dedicated server-only function!"));
        halo::hs::hs_thread_return(0, thread_index);
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
    halo::hs::hs_thread_return(0, thread_index);
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
        halo::networking::message_delta_parameters_protocol_reload_from_config_file();
        halo::networking::message_delta_definitions_invoke_field_bindings();
        halo::networking::message_delta_parameters_protocol_send_update();
    }
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_players`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482c00
 */
void ServerCommands::evaluate_sv_players(int16_t function_index, uint32_t thread_index, char first) const
{
    halo::networking::sv_players();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_status`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482c50
 */
void ServerCommands::evaluate_sv_status(int16_t function_index, uint32_t thread_index, char first) const
{
    halo::networking::sv_status();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of the hs script function `sv_unban`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x482a20
 */
void ServerCommands::evaluate_sv_unban(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int32_t index = arguments[0];

        if (index >= 0 && index < ban_list.count) {
            halo::interface::chimera__console_out(0, const_cast<char *>("Unbanning %s."), (uint8_t *)ban_list.data + index * 0x38);
            halo::memory::growable_array_remove_element(&ban_list, (uint32_t)index);
            halo::networking::network_banlist_save();
        }
        halo::hs::hs_thread_return(0, thread_index);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::map_list_matching_substring(argument_count, (char **)arguments);

        halo::hs::hs_thread_return(0, thread);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::sv_ban_penalty(argument_count, arguments);

        halo::hs::hs_thread_return(0, thread);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::sv_banlist_file(argument_count, arguments);

        halo::hs::hs_thread_return(0, thread);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::sv_friendly_fire(argument_count, arguments);

        halo::hs::hs_thread_return(0, thread);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::sv_maxplayers(argument_count, arguments);

        halo::hs::hs_thread_return(0, thread);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::sv_name(argument_count, (char **)arguments);

        halo::hs::hs_thread_return(0, thread);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::sv_password(argument_count, (char **)arguments);

        halo::hs::hs_thread_return(0, thread);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::sv_rcon_password(argument_count, arguments);

        halo::hs::hs_thread_return(0, thread);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::sv_single_flag_force_reset(argument_count, (char **)arguments);

        halo::hs::hs_thread_return(0, thread);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::sv_timelimit(argument_count, arguments);

        halo::hs::hs_thread_return(0, thread);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::sv_tk_cooldown(argument_count, arguments);

        halo::hs::hs_thread_return(0, thread);
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
    ready = halo::hs::hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        halo::networking::sv_tk_grace(argument_count, arguments);

        halo::hs::hs_thread_return(0, thread);
    }
}

}
