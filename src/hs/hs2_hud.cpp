#include "halo/hs/hs2_commands.hpp"
#include "halo/shaders/api.hpp"

#include "game.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" {
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first);
extern void hs_thread_return(int32_t value, uint32_t thread_index);
extern void player_help_screen_select_by_name(int16_t value);
extern uint8_t *hud_messaging;
extern game_time_globals *game_time;
extern hud_unit_meter_globals *hud_unit_meters;
extern void hud_set_help_text(int16_t message_index);
extern void hud_set_objective_text(int16_t message_index);
extern void hud_set_timer_time(int32_t minutes, int32_t seconds);
extern uint8_t *hud_weapon_state;
extern uint8_t numeric_countdown_timer_running;
extern int32_t numeric_countdown_timer_remaining_ms;
extern void hud_pause_timer(uint8_t paused);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    player_help_screen_select_by_name(*(int16_t *)&arguments[0]);
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t enable = *(uint8_t *)&arguments[0];

    if (enable && !hud_messaging[0x464]) {
        *(int32_t *)(hud_messaging + 0x460) = game_time->game_time;
    }
    hud_messaging[0x464] = enable;
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (*(uint8_t *)&arguments[0]) {
        hud_unit_meters->flags |= 2;
    } else {
        hud_unit_meters->flags &= 0xfffffffd;
    }
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (*(uint8_t *)&arguments[0]) {
        hud_unit_meters->flags |= 0x20;
    } else {
        hud_unit_meters->flags &= 0xffffffdf;
    }
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (*(uint8_t *)&arguments[0]) {
        hud_unit_meters->flags |= 8;
    } else {
        hud_unit_meters->flags &= 0xfffffff7;
    }
    hs_thread_return(0, thread_index);
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
        hud_messaging[0x82 + slot * 0x8c] = 0;
    }
    hs_thread_return(0, thread_index);
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

    if (hud_messaging[0x487]) {
        uint16_t stored = *(uint16_t *)(hud_messaging + 0x47c);

        if (stored == 0xffff) {
            ticks = 0xffff;
        } else if (hud_messaging[0x486]) {
            ticks = stored;
        } else {
            ticks = (uint16_t)(*(uint16_t *)(hud_messaging + 0x478) - *(uint16_t *)&game_time->game_time + stored);
        }
    }
    hs_thread_return((int32_t)ticks, thread_index);
}

/**
 * Evaluate handler of hs function "hud_help_flash_restart"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480380
 */
void HudCommands::evaluate_hud_help_flash_restart(int16_t function_index, uint32_t thread_index, char first)
{
    if (hud_messaging[0x464]) {
        *(int32_t *)(hud_messaging + 0x460) = game_time->game_time;
    }
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "hud_set_help_text"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480cb0
 */
void HudCommands::evaluate_hud_set_help_text(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hud_set_help_text(*(int16_t *)&arguments[0]);
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hud_set_objective_text(*(int16_t *)&arguments[0]);
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    int16_t corner = *(int16_t *)&arguments[2];

    *(int16_t *)(hud_messaging + 0x480) = *(int16_t *)&arguments[0];
    *(int16_t *)(hud_messaging + 0x482) = *(int16_t *)&arguments[1];
    *(int16_t *)(hud_messaging + 0x484) = corner < 0 ? 0 : (corner > 4 ? 4 : corner);
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hud_set_timer_time(*(int16_t *)&arguments[0], *(int16_t *)&arguments[1]);
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (*(uint8_t *)&arguments[0]) {
        *(uint32_t *)(hud_weapon_state + 0x78) |= 1;
    } else {
        *(uint32_t *)(hud_weapon_state + 0x78) &= 0xfffffffe;
    }
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (*(uint8_t *)&arguments[0]) {
        hud_unit_meters->flags &= 0xfffffffe;
    } else {
        hud_unit_meters->flags |= 1;
    }
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (*(uint8_t *)&arguments[0]) {
        hud_unit_meters->flags &= 0xffffffef;
    } else {
        hud_unit_meters->flags |= 0x10;
    }
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if (*(uint8_t *)&arguments[0]) {
        hud_unit_meters->flags &= 0xfffffffb;
    } else {
        hud_unit_meters->flags |= 4;
    }
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_thread_return((int32_t)(uint16_t)(halo::shaders::numeric_countdown_timer_get_digit((int16_t)arguments[0])), thread_index);
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
    numeric_countdown_timer_running = 1;
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "numeric_countdown_timer_set"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47ae10
 */
void HudCommands::evaluate_numeric_countdown_timer_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        numeric_countdown_timer_remaining_ms = arguments[0];
        numeric_countdown_timer_running = *(uint8_t *)&arguments[1];
        hs_thread_return(0, thread_index);
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
    numeric_countdown_timer_running = 0;
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "pause_hud_timer"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x480ee0
 */
void HudCommands::evaluate_pause_hud_timer(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hud_pause_timer(*(uint8_t *)&arguments[0]);
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t show = *(uint8_t *)&arguments[0];

    hud_flags[0] = show;
    hs_thread_return((int32_t)show, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t show = *(uint8_t *)&arguments[0];

    hud_flags[1] = show;
    hs_thread_return((int32_t)show, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hud_messaging[0x487] = *(uint8_t *)&arguments[0];
    hs_thread_return(0, thread_index);
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
