#include "halo/hs/hs1_system_commands.hpp"
#include "halo/memory/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern void game_engine_send_team_allegiance_message(char broadcast);
extern int32_t console_message_head;
extern int32_t console_message_tail;
extern uint8_t main_globals_byte_00719752;
extern uint8_t main_globals_byte_00719753;
extern uint8_t main_globals_byte_00719751;
}

namespace halo::hs {

/**
 * Evaluate handler for hs function "change_team" (short -> void).
 *
 * @address 0x482490
 */
void SystemCommands::change_team(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::game::game_engine_send_team_allegiance_message((char)(uint8_t)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "checkpoint_load" (string -> void).
 *
 * @address 0x4826a0
 */
void SystemCommands::checkpoint_load(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::saved_games::saved_game_load_checkpoint((char *)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "checkpoint_save" ( -> void).
 *
 * @address 0x482690
 */
void SystemCommands::checkpoint_save(int16_t function_index, uint32_t thread_index, char first)
{
    halo::saved_games::game_checkpoint_save_new();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "cls" (-> void).
 *
 * @address 0x4826e0
 */
void SystemCommands::cls(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    if (halo::main::globals().terminal_initialized != 0) {
        console_message_head = -1;
        console_message_tail = -1;
        halo::memory::data_delete_all(halo::main::globals().terminal_messages);
        halo::interface::console_clear_screen();
    }
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "connect" (string, string -> void).
 *
 * @address 0x482720
 */
void SystemCommands::connect(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::main::network_game_client_connect_to_address_async((char *)arguments[0], (char *)arguments[1]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "core_load" (no parameters -> void).
 *
 * @address 0x4828f0
 */
void SystemCommands::core_load(int16_t function_index, uint32_t thread_index, char first)
{
    main_globals_byte_00719752 = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "core_load_at_startup" (no parameters -> void).
 *
 * @address 0x47fc00
 */
void SystemCommands::core_load_at_startup(int16_t function_index, uint32_t thread_index, char first)
{
    main_globals_byte_00719753 = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "core_save" (no parameters -> void).
 *
 * @address 0x482910
 */
void SystemCommands::core_save(int16_t function_index, uint32_t thread_index, char first)
{
    main_globals_byte_00719751 = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "crash" (string -> void).
 *
 * @address 0x47f5d0
 */
void SystemCommands::crash(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        *(volatile const char **)0 = "chucky was here! NULL belongs to me!!!!!";
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for hs function "debug_camera_load" ( -> void).
 *
 * @address 0x47f030
 */
void SystemCommands::debug_camera_load(int16_t function_index, uint32_t thread_index, char first)
{
    halo::camera::camera_debug_load_from_file();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "debug_camera_save" ( -> void).
 *
 * @address 0x47f020
 */
void SystemCommands::debug_camera_save(int16_t function_index, uint32_t thread_index, char first)
{
    halo::camera::camera_debug_save_to_file();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler for hs function "debug_sounds_enable" (string, boolean -> void).
 *
 * @address 0x47ff90
 */
void SystemCommands::debug_sounds_enable(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::sound::sound_class_set_muted_by_name(*(uint8_t *)&arguments[1], (char *)arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

namespace {
const ScriptCommandEntry k_system_commands_entries[] = {
    {"change_team", &SystemCommands::change_team},
    {"checkpoint_load", &SystemCommands::checkpoint_load},
    {"checkpoint_save", &SystemCommands::checkpoint_save},
    {"cls", &SystemCommands::cls},
    {"connect", &SystemCommands::connect},
    {"core_load", &SystemCommands::core_load},
    {"core_load_at_startup", &SystemCommands::core_load_at_startup},
    {"core_save", &SystemCommands::core_save},
    {"crash", &SystemCommands::crash},
    {"debug_camera_load", &SystemCommands::debug_camera_load},
    {"debug_camera_save", &SystemCommands::debug_camera_save},
    {"debug_sounds_enable", &SystemCommands::debug_sounds_enable},
};
constexpr ScriptCommandGroup k_system_commands_group(k_system_commands_entries, sizeof(k_system_commands_entries) / sizeof(k_system_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by SystemCommands, keyed by script function name.
 */
const ScriptCommandGroup &SystemCommands::commands()
{
    return k_system_commands_group;
}

}

namespace halo::hs {

void hs_evaluate_change_team(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::change_team(function_index, thread_index, first);
}

void hs_evaluate_checkpoint_load(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::checkpoint_load(function_index, thread_index, first);
}

void hs_evaluate_checkpoint_save(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::checkpoint_save(function_index, thread_index, first);
}

void hs_evaluate_cls(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::cls(function_index, thread_index, first);
}

void hs_evaluate_connect(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::connect(function_index, thread_index, first);
}

void hs_evaluate_core_load(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::core_load(function_index, thread_index, first);
}

void hs_evaluate_core_load_at_startup(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::core_load_at_startup(function_index, thread_index, first);
}

void hs_evaluate_core_save(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::core_save(function_index, thread_index, first);
}

void hs_evaluate_crash(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::crash(function_index, thread_index, first);
}

void hs_evaluate_debug_camera_load(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::debug_camera_load(function_index, thread_index, first);
}

void hs_evaluate_debug_camera_save(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::debug_camera_save(function_index, thread_index, first);
}

void hs_evaluate_debug_sounds_enable(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SystemCommands::debug_sounds_enable(function_index, thread_index, first);
}

}
