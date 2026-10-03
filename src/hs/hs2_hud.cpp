#include "halo/hs/records.hpp"
#include "halo/hs/hs2_commands.hpp"
#include "halo/shaders/api.hpp"

#include "game.h"
#include "interface.h"
#include "halo/core/datum.hpp"
#include "halo/hs/api.hpp"
#include "halo/shaders/shaders.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

#ifdef __cplusplus
extern "C" {
#endif
extern void player_help_screen_select_by_name(int16_t value);
extern hud_messaging_globals *hud_messaging;
extern uint8_t *hud_weapon_state;
extern uint8_t *hud_flags;
#ifdef __cplusplus
}
#endif

namespace halo::hs {

/**
 * Evaluate handler of hs function "display_scenario_help"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481510
 */
void HudCommands::evaluate_display_scenario_help(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::interface::player_help_screen_select_by_name(halo::hs::argument_short(arguments[0]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "enable_hud_help_flash"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480310
 */
void HudCommands::evaluate_enable_hud_help_flash(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    uint8_t enable = halo::hs::argument_byte(arguments[0]);

    if (enable && !hud_messaging->help_text_flashing) {
        hud_messaging->help_text_flash_start_time = halo::game::globals().game_time->game_time;
    }
    hud_messaging->help_text_flashing = enable;
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "hud_blink_health"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480a40
 */
void HudCommands::evaluate_hud_blink_health(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    if (halo::hs::argument_byte(arguments[0])) {
        halo::interface::globals().hud_unit_meters->flags |= 2;
    } else {
        halo::interface::globals().hud_unit_meters->flags &= 0xfffffffd;
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "hud_blink_motion_sensor"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480bc0
 */
void HudCommands::evaluate_hud_blink_motion_sensor(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    if (halo::hs::argument_byte(arguments[0])) {
        halo::interface::globals().hud_unit_meters->flags |= 0x20;
    } else {
        halo::interface::globals().hud_unit_meters->flags &= 0xffffffdf;
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "hud_blink_shield"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480b00
 */
void HudCommands::evaluate_hud_blink_shield(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    if (halo::hs::argument_byte(arguments[0])) {
        halo::interface::globals().hud_unit_meters->flags |= 8;
    } else {
        halo::interface::globals().hud_unit_meters->flags &= 0xfffffff7;
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "hud_clear_messages"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480c80
 */
void HudCommands::evaluate_hud_clear_messages(int16_t function_index, uint32_t thread_index, char first)
{
    int32_t slot;

    for (slot = 0; slot < 4; slot++) {
        hud_messaging->players[0].messages[slot].active = 0;
    }
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "hud_get_timer_ticks"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480f20
 */
void HudCommands::evaluate_hud_get_timer_ticks(int16_t function_index, uint32_t thread_index, char first)
{
    uint16_t ticks = 0;

    if (hud_messaging->timer_active) {
        uint16_t stored = hud_messaging->timer_ticks;

        if (stored == halo::k_word_none) {
            ticks = halo::k_word_none;
        } else if (hud_messaging->timer_paused) {
            ticks = stored;
        } else {
            ticks = (uint16_t)(static_cast<uint16_t>(hud_messaging->timer_start_time) - *(uint16_t *)&halo::game::globals().game_time->game_time + stored);
        }
    }
    halo::hs::hs_thread_return((int32_t)ticks, thread_index);
}

/**
 * Evaluate handler of hs function "hud_help_flash_restart"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480380
 */
void HudCommands::evaluate_hud_help_flash_restart(int16_t function_index, uint32_t thread_index, char first)
{
    if (hud_messaging->help_text_flashing) {
        hud_messaging->help_text_flash_start_time = halo::game::globals().game_time->game_time;
    }
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "hud_set_help_text"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480cb0
 */
void HudCommands::evaluate_hud_set_help_text(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::interface::hud_set_help_text(halo::hs::argument_short(arguments[0]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "hud_set_objective_text"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480d00
 */
void HudCommands::evaluate_hud_set_objective_text(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::interface::hud_set_objective_text(halo::hs::argument_short(arguments[0]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "hud_set_timer_position"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480e00
 */
void HudCommands::evaluate_hud_set_timer_position(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    int16_t corner = halo::hs::argument_short(arguments[2]);

    hud_messaging->timer_offset.x = halo::hs::argument_short(arguments[0]);
    hud_messaging->timer_offset.y = halo::hs::argument_short(arguments[1]);
    hud_messaging->timer_anchor = corner < 0 ? 0 : (corner > 4 ? 4 : corner);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "hud_set_timer_time"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480d50
 */
void HudCommands::evaluate_hud_set_timer_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::interface::hud_set_timer_time(halo::hs::argument_short(arguments[0]), halo::hs::argument_short(arguments[1]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "hud_show_crosshair"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480c20
 */
void HudCommands::evaluate_hud_show_crosshair(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    if (halo::hs::argument_byte(arguments[0])) {
        *(uint32_t *)(hud_weapon_state + 0x78) |= 1;
    } else {
        *(uint32_t *)(hud_weapon_state + 0x78) &= 0xfffffffe;
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "hud_show_health"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4809e0
 */
void HudCommands::evaluate_hud_show_health(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    if (halo::hs::argument_byte(arguments[0])) {
        halo::interface::globals().hud_unit_meters->flags &= 0xfffffffe;
    } else {
        halo::interface::globals().hud_unit_meters->flags |= 1;
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "hud_show_motion_sensor"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480b60
 */
void HudCommands::evaluate_hud_show_motion_sensor(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    if (halo::hs::argument_byte(arguments[0])) {
        halo::interface::globals().hud_unit_meters->flags &= 0xffffffef;
    } else {
        halo::interface::globals().hud_unit_meters->flags |= 0x10;
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "hud_show_shield"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480aa0
 */
void HudCommands::evaluate_hud_show_shield(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    if (halo::hs::argument_byte(arguments[0])) {
        halo::interface::globals().hud_unit_meters->flags &= 0xfffffffb;
    } else {
        halo::interface::globals().hud_unit_meters->flags |= 4;
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "numeric_countdown_timer_get"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47ae60
 */
void HudCommands::evaluate_numeric_countdown_timer_get(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_thread_return((int32_t)(uint16_t)(halo::shaders::numeric_countdown_timer::get_digit((int16_t)arguments[0])), thread_index);
    }
}

/**
 * Evaluate handler of hs function "numeric_countdown_timer_restart"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47aee0
 */
void HudCommands::evaluate_numeric_countdown_timer_restart(int16_t function_index, uint32_t thread_index, char first)
{
    halo::shaders::globals().numeric_countdown_timer_running = 1;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "numeric_countdown_timer_set"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47ae10
 */
void HudCommands::evaluate_numeric_countdown_timer_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::shaders::globals().numeric_countdown_timer_remaining_ms = arguments[0];
        halo::shaders::globals().numeric_countdown_timer_running = halo::hs::argument_byte(arguments[1]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "numeric_countdown_timer_stop"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47aec0
 */
void HudCommands::evaluate_numeric_countdown_timer_stop(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    halo::shaders::globals().numeric_countdown_timer_running = 0;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "pause_hud_timer"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480ee0
 */
void HudCommands::evaluate_pause_hud_timer(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::interface::hud_pause_timer(halo::hs::argument_byte(arguments[0]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "show_hud"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480250
 */
void HudCommands::evaluate_show_hud(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    uint8_t show = halo::hs::argument_byte(arguments[0]);

    hud_flags[0] = show;
    halo::hs::hs_thread_return((int32_t)show, thread_index);
    }
}

/**
 * Evaluate handler of hs function "show_hud_help_text"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4802b0
 */
void HudCommands::evaluate_show_hud_help_text(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    uint8_t show = halo::hs::argument_byte(arguments[0]);

    hud_flags[1] = show;
    halo::hs::hs_thread_return((int32_t)show, thread_index);
    }
}

/**
 * Evaluate handler of hs function "show_hud_timer"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480e90
 */
void HudCommands::evaluate_show_hud_timer(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    hud_messaging->timer_active = halo::hs::argument_byte(arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Table of the hs functions handled by HudCommands, in source order.
 */
EvaluateCommandTable HudCommands::commands() noexcept
{
    static constexpr EvaluateFn k_commands[] = {
        &HudCommands::evaluate_display_scenario_help,
        &HudCommands::evaluate_enable_hud_help_flash,
        &HudCommands::evaluate_hud_blink_health,
        &HudCommands::evaluate_hud_blink_motion_sensor,
        &HudCommands::evaluate_hud_blink_shield,
        &HudCommands::evaluate_hud_clear_messages,
        &HudCommands::evaluate_hud_get_timer_ticks,
        &HudCommands::evaluate_hud_help_flash_restart,
        &HudCommands::evaluate_hud_set_help_text,
        &HudCommands::evaluate_hud_set_objective_text,
        &HudCommands::evaluate_hud_set_timer_position,
        &HudCommands::evaluate_hud_set_timer_time,
        &HudCommands::evaluate_hud_set_timer_warning_time,
        &HudCommands::evaluate_hud_show_crosshair,
        &HudCommands::evaluate_hud_show_health,
        &HudCommands::evaluate_hud_show_motion_sensor,
        &HudCommands::evaluate_hud_show_shield,
        &HudCommands::evaluate_numeric_countdown_timer_get,
        &HudCommands::evaluate_numeric_countdown_timer_restart,
        &HudCommands::evaluate_numeric_countdown_timer_set,
        &HudCommands::evaluate_numeric_countdown_timer_stop,
        &HudCommands::evaluate_pause_hud_timer,
        &HudCommands::evaluate_show_hud,
        &HudCommands::evaluate_show_hud_help_text,
        &HudCommands::evaluate_show_hud_timer,
    };
    return {k_commands, static_cast<uint32_t>(sizeof(k_commands) / sizeof(k_commands[0]))};
}

}
