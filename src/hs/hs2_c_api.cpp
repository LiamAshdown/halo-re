#include "halo/hs/hs2_commands.hpp"

extern "C" {

/**
 * C entry point for halo::hs::DeviceCommands::evaluate_device_set_position; forwards to the C++ implementation unchanged.
 *
 * @address 0x47ca40
 */
void hs_evaluate_device_set_position(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::evaluate_device_set_position(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DeviceCommands::evaluate_device_set_position_immediate; forwards to the C++ implementation unchanged.
 *
 * @address 0x47cb60
 */
void hs_evaluate_device_set_position_immediate(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::evaluate_device_set_position_immediate(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DeviceCommands::evaluate_device_set_power; forwards to the C++ implementation unchanged.
 *
 * @address 0x47c930
 */
void hs_evaluate_device_set_power(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DeviceCommands::evaluate_device_set_power(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_disconnect; forwards to the C++ implementation unchanged.
 *
 * @address 0x482770
 */
void hs_evaluate_disconnect(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_disconnect(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_error_overflow_suppression; forwards to the C++ implementation unchanged.
 *
 * @address 0x4807d0
 */
void hs_evaluate_error_overflow_suppression(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_error_overflow_suppression(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_fade_in; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f6f0
 */
void hs_evaluate_fade_in(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_fade_in(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_fade_out; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f770
 */
void hs_evaluate_fade_out(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_fade_out(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_fast_setup_network_server; forwards to the C++ implementation unchanged.
 *
 * @address 0x4813b0
 */
void hs_evaluate_fast_setup_network_server(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_fast_setup_network_server(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_all_quiet; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fa60
 */
void hs_evaluate_game_all_quiet(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_all_quiet(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_difficulty_get; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f0a0
 */
void hs_evaluate_game_difficulty_get(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_difficulty_get(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_difficulty_get_real; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f0d0
 */
void hs_evaluate_game_difficulty_get_real(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_difficulty_get_real(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_difficulty_set; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f580
 */
void hs_evaluate_game_difficulty_set(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_difficulty_set(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_is_cooperative; forwards to the C++ implementation unchanged.
 *
 * @address 0x47faa0
 */
void hs_evaluate_game_is_cooperative(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_is_cooperative(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_lost; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fa20
 */
void hs_evaluate_game_lost(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_lost(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_revert; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fbd0
 */
void hs_evaluate_game_revert(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_revert(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_reverted; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fcb0
 */
void hs_evaluate_game_reverted(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_reverted(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_safe_to_save; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fa40
 */
void hs_evaluate_game_safe_to_save(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_safe_to_save(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_safe_to_speak; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fa80
 */
void hs_evaluate_game_safe_to_speak(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_safe_to_speak(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_save; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fad0
 */
void hs_evaluate_game_save(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_save(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_save_cancel; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fb20
 */
void hs_evaluate_game_save_cancel(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_save_cancel(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_save_no_timeout; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fb40
 */
void hs_evaluate_game_save_no_timeout(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_save_no_timeout(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_save_totally_unsafe; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fb90
 */
void hs_evaluate_game_save_totally_unsafe(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_save_totally_unsafe(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_saving; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fbb0
 */
void hs_evaluate_game_saving(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_saving(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_skip_ticks; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fc60
 */
void hs_evaluate_game_skip_ticks(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_skip_ticks(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_speed; forwards to the C++ implementation unchanged.
 *
 * @address 0x482930
 */
void hs_evaluate_game_speed(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_speed(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_variant; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f040
 */
void hs_evaluate_game_variant(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_variant(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_won; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fa00
 */
void hs_evaluate_game_won(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_won(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_garbage_collect_now; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b410
 */
void hs_evaluate_garbage_collect_now(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_garbage_collect_now(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_map_name; forwards to the C++ implementation unchanged.
 *
 * @address 0x482980
 */
void hs_evaluate_map_name(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_map_name(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_map_reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f500
 */
void hs_evaluate_map_reset(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_map_reset(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_multiplayer_map_name; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f530
 */
void hs_evaluate_multiplayer_map_name(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_multiplayer_map_name(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_profile_load; forwards to the C++ implementation unchanged.
 *
 * @address 0x4827a0
 */
void hs_evaluate_profile_load(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_profile_load(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_profile_unlock_solo_levels; forwards to the C++ implementation unchanged.
 *
 * @address 0x481400
 */
void hs_evaluate_profile_unlock_solo_levels(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_profile_unlock_solo_levels(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_quit; forwards to the C++ implementation unchanged.
 *
 * @address 0x482820
 */
void hs_evaluate_quit(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_quit(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_rcon; forwards to the C++ implementation unchanged.
 *
 * @address 0x4829c0
 */
void hs_evaluate_rcon(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_rcon(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_remote_player_stats; forwards to the C++ implementation unchanged.
 *
 * @address 0x4825b0
 */
void hs_evaluate_remote_player_stats(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_remote_player_stats(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::GameCommands::evaluate_game_time; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f080
 */
void hs_evaluate_game_time(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::GameCommands::evaluate_game_time(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_display_scenario_help; forwards to the C++ implementation unchanged.
 *
 * @address 0x481510
 */
void hs_evaluate_display_scenario_help(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_display_scenario_help(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_enable_hud_help_flash; forwards to the C++ implementation unchanged.
 *
 * @address 0x480310
 */
void hs_evaluate_enable_hud_help_flash(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_enable_hud_help_flash(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_blink_health; forwards to the C++ implementation unchanged.
 *
 * @address 0x480a40
 */
void hs_evaluate_hud_blink_health(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_blink_health(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_blink_motion_sensor; forwards to the C++ implementation unchanged.
 *
 * @address 0x480bc0
 */
void hs_evaluate_hud_blink_motion_sensor(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_blink_motion_sensor(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_blink_shield; forwards to the C++ implementation unchanged.
 *
 * @address 0x480b00
 */
void hs_evaluate_hud_blink_shield(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_blink_shield(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_clear_messages; forwards to the C++ implementation unchanged.
 *
 * @address 0x480c80
 */
void hs_evaluate_hud_clear_messages(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_clear_messages(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_get_timer_ticks; forwards to the C++ implementation unchanged.
 *
 * @address 0x480f20
 */
void hs_evaluate_hud_get_timer_ticks(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_get_timer_ticks(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_help_flash_restart; forwards to the C++ implementation unchanged.
 *
 * @address 0x480380
 */
void hs_evaluate_hud_help_flash_restart(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_help_flash_restart(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_set_help_text; forwards to the C++ implementation unchanged.
 *
 * @address 0x480cb0
 */
void hs_evaluate_hud_set_help_text(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_set_help_text(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_set_objective_text; forwards to the C++ implementation unchanged.
 *
 * @address 0x480d00
 */
void hs_evaluate_hud_set_objective_text(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_set_objective_text(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_set_timer_position; forwards to the C++ implementation unchanged.
 *
 * @address 0x480e00
 */
void hs_evaluate_hud_set_timer_position(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_set_timer_position(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_set_timer_time; forwards to the C++ implementation unchanged.
 *
 * @address 0x480d50
 */
void hs_evaluate_hud_set_timer_time(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_set_timer_time(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_show_crosshair; forwards to the C++ implementation unchanged.
 *
 * @address 0x480c20
 */
void hs_evaluate_hud_show_crosshair(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_show_crosshair(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_show_health; forwards to the C++ implementation unchanged.
 *
 * @address 0x4809e0
 */
void hs_evaluate_hud_show_health(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_show_health(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_show_motion_sensor; forwards to the C++ implementation unchanged.
 *
 * @address 0x480b60
 */
void hs_evaluate_hud_show_motion_sensor(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_show_motion_sensor(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_show_shield; forwards to the C++ implementation unchanged.
 *
 * @address 0x480aa0
 */
void hs_evaluate_hud_show_shield(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_show_shield(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_numeric_countdown_timer_get; forwards to the C++ implementation unchanged.
 *
 * @address 0x47ae60
 */
void hs_evaluate_numeric_countdown_timer_get(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_numeric_countdown_timer_get(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_numeric_countdown_timer_restart; forwards to the C++ implementation unchanged.
 *
 * @address 0x47aee0
 */
void hs_evaluate_numeric_countdown_timer_restart(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_numeric_countdown_timer_restart(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_numeric_countdown_timer_set; forwards to the C++ implementation unchanged.
 *
 * @address 0x47ae10
 */
void hs_evaluate_numeric_countdown_timer_set(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_numeric_countdown_timer_set(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_numeric_countdown_timer_stop; forwards to the C++ implementation unchanged.
 *
 * @address 0x47aec0
 */
void hs_evaluate_numeric_countdown_timer_stop(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_numeric_countdown_timer_stop(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_pause_hud_timer; forwards to the C++ implementation unchanged.
 *
 * @address 0x480ee0
 */
void hs_evaluate_pause_hud_timer(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_pause_hud_timer(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_show_hud; forwards to the C++ implementation unchanged.
 *
 * @address 0x480250
 */
void hs_evaluate_show_hud(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_show_hud(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_show_hud_help_text; forwards to the C++ implementation unchanged.
 *
 * @address 0x4802b0
 */
void hs_evaluate_show_hud_help_text(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_show_hud_help_text(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_show_hud_timer; forwards to the C++ implementation unchanged.
 *
 * @address 0x480e90
 */
void hs_evaluate_show_hud_timer(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_show_hud_timer(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::HudCommands::evaluate_hud_set_timer_warning_time; forwards to the C++ implementation unchanged.
 *
 * @address 0x480da0
 */
void hs_evaluate_hud_set_timer_warning_time(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::HudCommands::evaluate_hud_set_timer_warning_time(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_effect_new; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a9c0
 */
void hs_evaluate_effect_new(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_effect_new(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_effect_new_on_object_marker; forwards to the C++ implementation unchanged.
 *
 * @address 0x47aa10
 */
void hs_evaluate_effect_new_on_object_marker(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_effect_new_on_object_marker(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_magic_melee_attack; forwards to the C++ implementation unchanged.
 *
 * @address 0x47c2d0
 */
void hs_evaluate_magic_melee_attack(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_magic_melee_attack(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_magic_seat_name; forwards to the C++ implementation unchanged.
 *
 * @address 0x47c210
 */
void hs_evaluate_magic_seat_name(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_magic_seat_name(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_beautify; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b4b0
 */
void hs_evaluate_object_beautify(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_beautify(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_can_take_damage; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b470
 */
void hs_evaluate_object_can_take_damage(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_can_take_damage(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_cannot_take_damage; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b430
 */
void hs_evaluate_object_cannot_take_damage(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_cannot_take_damage(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_cast; forwards to the C++ implementation unchanged.
 *
 * @address 0x489c80
 */
void hs_evaluate_object_cast(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_cast(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_create; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a5b0
 */
void hs_evaluate_object_create(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_create(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_create_anew; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a670
 */
void hs_evaluate_object_create_anew(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_create_anew(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_create_anew_containing; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a710
 */
void hs_evaluate_object_create_anew_containing(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_create_anew_containing(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_create_containing; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a6c0
 */
void hs_evaluate_object_create_containing(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_create_containing(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_destroy; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a610
 */
void hs_evaluate_object_destroy(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_destroy(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_destroy_all; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a7b0
 */
void hs_evaluate_object_destroy_all(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_destroy_all(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_destroy_containing; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a760
 */
void hs_evaluate_object_destroy_containing(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_destroy_containing(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_pvs_clear; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b680
 */
void hs_evaluate_object_pvs_clear(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_pvs_clear(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_pvs_set_camera; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b640
 */
void hs_evaluate_object_pvs_set_camera(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_pvs_set_camera(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_pvs_set_object; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b5d0
 */
void hs_evaluate_object_pvs_set_object(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_pvs_set_object(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_set_collideable; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b240
 */
void hs_evaluate_object_set_collideable(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_set_collideable(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_set_facing; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a810
 */
void hs_evaluate_object_set_facing(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_set_facing(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_set_melee_attack_inhibited; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b1b0
 */
void hs_evaluate_object_set_melee_attack_inhibited(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_set_melee_attack_inhibited(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_set_permutation; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a8b0
 */
void hs_evaluate_object_set_permutation(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_set_permutation(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_set_ranged_attack_inhibited; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b130
 */
void hs_evaluate_object_set_ranged_attack_inhibited(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_set_ranged_attack_inhibited(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_set_scale; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b2c0
 */
void hs_evaluate_object_set_scale(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_set_scale(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_set_shield; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a860
 */
void hs_evaluate_object_set_shield(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_set_shield(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_teleport; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a7c0
 */
void hs_evaluate_object_teleport(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_teleport(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_object_type_predict; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b570
 */
void hs_evaluate_object_type_predict(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_object_type_predict(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_objects_attach; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b310
 */
void hs_evaluate_objects_attach(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_objects_attach(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_objects_can_see_flag; forwards to the C++ implementation unchanged.
 *
 * @address 0x47ab60
 */
void hs_evaluate_objects_can_see_flag(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_objects_can_see_flag(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_objects_can_see_object; forwards to the C++ implementation unchanged.
 *
 * @address 0x47ab00
 */
void hs_evaluate_objects_can_see_object(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_objects_can_see_object(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_objects_delete_by_definition; forwards to the C++ implementation unchanged.
 *
 * @address 0x47abc0
 */
void hs_evaluate_objects_delete_by_definition(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_objects_delete_by_definition(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_objects_detach; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b390
 */
void hs_evaluate_objects_detach(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_objects_detach(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_objects_dump_memory; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b230
 */
void hs_evaluate_objects_dump_memory(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_objects_dump_memory(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_objects_predict; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b530
 */
void hs_evaluate_objects_predict(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_objects_predict(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_scenery_animation_start; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b760
 */
void hs_evaluate_scenery_animation_start(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_scenery_animation_start(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_scenery_animation_start_at_frame; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b7b0
 */
void hs_evaluate_scenery_animation_start_at_frame(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_scenery_animation_start_at_frame(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::ObjectCommands::evaluate_scenery_get_animation_time; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b700
 */
void hs_evaluate_scenery_get_animation_time(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ObjectCommands::evaluate_scenery_get_animation_time(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_equality; forwards to the C++ implementation unchanged.
 *
 * @address 0x4893e0
 */
void hs_evaluate_equality(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::FlowCommands::evaluate_equality(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_expression; forwards to the C++ implementation unchanged.
 *
 * @address 0x48a250
 */
int32_t hs_evaluate_expression(datum_index node)
{
    return halo::hs::FlowCommands::evaluate_expression(node);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_if; forwards to the C++ implementation unchanged.
 *
 * @address 0x488e60
 */
void hs_evaluate_if(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::FlowCommands::evaluate_if(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_ignore_arguments; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fc20
 */
void hs_evaluate_ignore_arguments(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::FlowCommands::evaluate_ignore_arguments(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_not; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a360
 */
void hs_evaluate_not(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::FlowCommands::evaluate_not(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_nothing; forwards to the C++ implementation unchanged.
 *
 * @address 0x47cf20
 */
void hs_evaluate_nothing(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::FlowCommands::evaluate_nothing(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_random; forwards to the C++ implementation unchanged.
 *
 * @address 0x488c60
 */
void hs_evaluate_random(hs_thread *thread, uint32_t thread_index, char first)
{
    halo::hs::FlowCommands::evaluate_random(thread, thread_index, first);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_set; forwards to the C++ implementation unchanged.
 *
 * @address 0x488fd0
 */
void hs_evaluate_set(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::FlowCommands::evaluate_set(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_sleep; forwards to the C++ implementation unchanged.
 *
 * @address 0x489800
 */
void hs_evaluate_sleep(uint32_t unused_param_1, uint32_t thread_index, char first)
{
    halo::hs::FlowCommands::evaluate_sleep(unused_param_1, thread_index, first);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_sleep_ticks; forwards to the C++ implementation unchanged.
 *
 * @address 0x489650
 */
void hs_evaluate_sleep_ticks(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::FlowCommands::evaluate_sleep_ticks(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_random_range; forwards to the C++ implementation unchanged.
 *
 * @address 0x47ad00
 */
void hs_evaluate_random_range(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::FlowCommands::evaluate_random_range(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::FlowCommands::evaluate_real_random_range; forwards to the C++ implementation unchanged.
 *
 * @address 0x47ad90
 */
void hs_evaluate_real_random_range(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::FlowCommands::evaluate_real_random_range(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_digital_forward_throttle; forwards to the C++ implementation unchanged.
 *
 * @address 0x481b90
 */
void hs_evaluate_get_digital_forward_throttle(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_digital_forward_throttle(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_digital_pitch_increment; forwards to the C++ implementation unchanged.
 *
 * @address 0x481e80
 */
void hs_evaluate_get_digital_pitch_increment(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_digital_pitch_increment(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_digital_strafe_throttle; forwards to the C++ implementation unchanged.
 *
 * @address 0x481ca0
 */
void hs_evaluate_get_digital_strafe_throttle(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_digital_strafe_throttle(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_digital_yaw_increment; forwards to the C++ implementation unchanged.
 *
 * @address 0x481db0
 */
void hs_evaluate_get_digital_yaw_increment(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_digital_yaw_increment(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_gamepad_forward_threshold; forwards to the C++ implementation unchanged.
 *
 * @address 0x482250
 */
void hs_evaluate_get_gamepad_forward_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_gamepad_forward_threshold(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_gamepad_strafe_threshold; forwards to the C++ implementation unchanged.
 *
 * @address 0x4822f0
 */
void hs_evaluate_get_gamepad_strafe_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_gamepad_strafe_threshold(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_gamepad_yaw_scale; forwards to the C++ implementation unchanged.
 *
 * @address 0x482390
 */
void hs_evaluate_get_gamepad_yaw_scale(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_gamepad_yaw_scale(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_mouse_forward_threshold; forwards to the C++ implementation unchanged.
 *
 * @address 0x481f50
 */
void hs_evaluate_get_mouse_forward_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_mouse_forward_threshold(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_mouse_pitch_scale; forwards to the C++ implementation unchanged.
 *
 * @address 0x482180
 */
void hs_evaluate_get_mouse_pitch_scale(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_mouse_pitch_scale(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_mouse_strafe_threshold; forwards to the C++ implementation unchanged.
 *
 * @address 0x482000
 */
void hs_evaluate_get_mouse_strafe_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_mouse_strafe_threshold(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_mouse_yaw_scale; forwards to the C++ implementation unchanged.
 *
 * @address 0x4820b0
 */
void hs_evaluate_get_mouse_yaw_scale(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_mouse_yaw_scale(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_pitch_rate; forwards to the C++ implementation unchanged.
 *
 * @address 0x481a80
 */
void hs_evaluate_get_pitch_rate(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_pitch_rate(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_get_yaw_rate; forwards to the C++ implementation unchanged.
 *
 * @address 0x481a30
 */
void hs_evaluate_get_yaw_rate(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_get_yaw_rate(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_player0_joystick_set_is_normal; forwards to the C++ implementation unchanged.
 *
 * @address 0x4814a0
 */
void hs_evaluate_player0_joystick_set_is_normal(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_player0_joystick_set_is_normal(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_player0_look_invert_pitch; forwards to the C++ implementation unchanged.
 *
 * @address 0x481440
 */
void hs_evaluate_player0_look_invert_pitch(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_player0_look_invert_pitch(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_player0_look_pitch_is_inverted; forwards to the C++ implementation unchanged.
 *
 * @address 0x481480
 */
void hs_evaluate_player0_look_pitch_is_inverted(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_player0_look_pitch_is_inverted(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_digital_forward_throttle; forwards to the C++ implementation unchanged.
 *
 * @address 0x481c30
 */
void hs_evaluate_set_digital_forward_throttle(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_digital_forward_throttle(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_digital_pitch_increment; forwards to the C++ implementation unchanged.
 *
 * @address 0x481ee0
 */
void hs_evaluate_set_digital_pitch_increment(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_digital_pitch_increment(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_digital_strafe_throttle; forwards to the C++ implementation unchanged.
 *
 * @address 0x481d40
 */
void hs_evaluate_set_digital_strafe_throttle(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_digital_strafe_throttle(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_digital_yaw_increment; forwards to the C++ implementation unchanged.
 *
 * @address 0x481e10
 */
void hs_evaluate_set_digital_yaw_increment(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_digital_yaw_increment(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_gamepad_forward_threshold; forwards to the C++ implementation unchanged.
 *
 * @address 0x4822a0
 */
void hs_evaluate_set_gamepad_forward_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_gamepad_forward_threshold(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_gamepad_strafe_threshold; forwards to the C++ implementation unchanged.
 *
 * @address 0x482340
 */
void hs_evaluate_set_gamepad_strafe_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_gamepad_strafe_threshold(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_mouse_forward_threshold; forwards to the C++ implementation unchanged.
 *
 * @address 0x481fa0
 */
void hs_evaluate_set_mouse_forward_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_mouse_forward_threshold(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_mouse_pitch_scale; forwards to the C++ implementation unchanged.
 *
 * @address 0x4821e0
 */
void hs_evaluate_set_mouse_pitch_scale(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_mouse_pitch_scale(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_mouse_strafe_threshold; forwards to the C++ implementation unchanged.
 *
 * @address 0x482050
 */
void hs_evaluate_set_mouse_strafe_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_mouse_strafe_threshold(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_mouse_yaw_scale; forwards to the C++ implementation unchanged.
 *
 * @address 0x482110
 */
void hs_evaluate_set_mouse_yaw_scale(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_mouse_yaw_scale(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_pitch_rate; forwards to the C++ implementation unchanged.
 *
 * @address 0x481b30
 */
void hs_evaluate_set_pitch_rate(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_pitch_rate(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputSettingsCommands::evaluate_set_yaw_rate; forwards to the C++ implementation unchanged.
 *
 * @address 0x481ad0
 */
void hs_evaluate_set_yaw_rate(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputSettingsCommands::evaluate_set_yaw_rate(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_help; forwards to the C++ implementation unchanged.
 *
 * @address 0x4827e0
 */
void hs_evaluate_help(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_help(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_inspect; forwards to the C++ implementation unchanged.
 *
 * @address 0x489b80
 */
void hs_evaluate_inspect(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_inspect(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_list_count; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a950
 */
void hs_evaluate_list_count(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_list_count(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_list_get; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a900
 */
void hs_evaluate_list_get(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_list_get(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_message_metrics_dump; forwards to the C++ implementation unchanged.
 *
 * @address 0x480780
 */
void hs_evaluate_message_metrics_dump(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_message_metrics_dump(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_net_graph_clear; forwards to the C++ implementation unchanged.
 *
 * @address 0x4806b0
 */
void hs_evaluate_net_graph_clear(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_net_graph_clear(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_net_graph_show; forwards to the C++ implementation unchanged.
 *
 * @address 0x4806d0
 */
void hs_evaluate_net_graph_show(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_net_graph_show(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_print; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a3c0
 */
void hs_evaluate_print(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_print(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_print_binds; forwards to the C++ implementation unchanged.
 *
 * @address 0x482480
 */
void hs_evaluate_print_binds(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_print_binds(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_rasterizer_fixed_function_ambient; forwards to the C++ implementation unchanged.
 *
 * @address 0x480ff0
 */
void hs_evaluate_rasterizer_fixed_function_ambient(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_rasterizer_fixed_function_ambient(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_rasterizer_lights_reset_for_new_map; forwards to the C++ implementation unchanged.
 *
 * @address 0x4810c0
 */
void hs_evaluate_rasterizer_lights_reset_for_new_map(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_rasterizer_lights_reset_for_new_map(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_rasterizer_model_ambient_reflection_tint; forwards to the C++ implementation unchanged.
 *
 * @address 0x481050
 */
void hs_evaluate_rasterizer_model_ambient_reflection_tint(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_rasterizer_model_ambient_reflection_tint(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_render_lights; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b6a0
 */
void hs_evaluate_render_lights(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_render_lights(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_script_doc; forwards to the C++ implementation unchanged.
 *
 * @address 0x47acf0
 */
void hs_evaluate_script_doc(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_script_doc(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_script_recompile; forwards to the C++ implementation unchanged.
 *
 * @address 0x47acd0
 */
void hs_evaluate_script_recompile(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_script_recompile(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_script_screen_effect_set_value; forwards to the C++ implementation unchanged.
 *
 * @address 0x4810f0
 */
void hs_evaluate_script_screen_effect_set_value(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_script_screen_effect_set_value(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::DebugCommands::evaluate_set_gamma; forwards to the C++ implementation unchanged.
 *
 * @address 0x480fa0
 */
void hs_evaluate_set_gamma(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::DebugCommands::evaluate_set_gamma(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputDeviceCommands::evaluate_input_activate_joy; forwards to the C++ implementation unchanged.
 *
 * @address 0x481890
 */
void hs_evaluate_input_activate_joy(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputDeviceCommands::evaluate_input_activate_joy(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputDeviceCommands::evaluate_input_find_default; forwards to the C++ implementation unchanged.
 *
 * @address 0x4819f0
 */
void hs_evaluate_input_find_default(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputDeviceCommands::evaluate_input_find_default(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputDeviceCommands::evaluate_input_find_joystick; forwards to the C++ implementation unchanged.
 *
 * @address 0x481990
 */
void hs_evaluate_input_find_joystick(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputDeviceCommands::evaluate_input_find_joystick(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputDeviceCommands::evaluate_input_get_joy_count; forwards to the C++ implementation unchanged.
 *
 * @address 0x4817f0
 */
void hs_evaluate_input_get_joy_count(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputDeviceCommands::evaluate_input_get_joy_count(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputDeviceCommands::evaluate_input_is_joy_active; forwards to the C++ implementation unchanged.
 *
 * @address 0x481820
 */
void hs_evaluate_input_is_joy_active(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputDeviceCommands::evaluate_input_is_joy_active(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputDeviceCommands::evaluate_input_show_joystick_info; forwards to the C++ implementation unchanged.
 *
 * @address 0x4819e0
 */
void hs_evaluate_input_show_joystick_info(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputDeviceCommands::evaluate_input_show_joystick_info(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::InputDeviceCommands::evaluate_input_deactivate_joy; forwards to the C++ implementation unchanged.
 *
 * @address 0x481920
 */
void hs_evaluate_input_deactivate_joy(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::InputDeviceCommands::evaluate_input_deactivate_joy(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::RecordingCommands::evaluate_play_update_history; forwards to the C++ implementation unchanged.
 *
 * @address 0x480730
 */
void hs_evaluate_play_update_history(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::RecordingCommands::evaluate_play_update_history(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::RecordingCommands::evaluate_playback; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f6c0
 */
void hs_evaluate_playback(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::RecordingCommands::evaluate_playback(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::RecordingCommands::evaluate_recording_kill; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b070
 */
void hs_evaluate_recording_kill(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::RecordingCommands::evaluate_recording_kill(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::RecordingCommands::evaluate_recording_play; forwards to the C++ implementation unchanged.
 *
 * @address 0x47af50
 */
void hs_evaluate_recording_play(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::RecordingCommands::evaluate_recording_play(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::RecordingCommands::evaluate_recording_play_and_delete; forwards to the C++ implementation unchanged.
 *
 * @address 0x47afb0
 */
void hs_evaluate_recording_play_and_delete(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::RecordingCommands::evaluate_recording_play_and_delete(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::RecordingCommands::evaluate_recording_play_and_hover; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b010
 */
void hs_evaluate_recording_play_and_hover(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::RecordingCommands::evaluate_recording_play_and_hover(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::RecordingCommands::evaluate_recording_time; forwards to the C++ implementation unchanged.
 *
 * @address 0x47b0c0
 */
void hs_evaluate_recording_time(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::RecordingCommands::evaluate_recording_time(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_accept; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f310
 */
void hs_evaluate_player_action_test_accept(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_accept(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_back; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f350
 */
void hs_evaluate_player_action_test_back(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_back(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_grenade_trigger; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f270
 */
void hs_evaluate_player_action_test_grenade_trigger(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_grenade_trigger(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_jump; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f210
 */
void hs_evaluate_player_action_test_jump(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_jump(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_look_relative_all_directions; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f450
 */
void hs_evaluate_player_action_test_look_relative_all_directions(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_look_relative_all_directions(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_look_relative_down; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f3c0
 */
void hs_evaluate_player_action_test_look_relative_down(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_look_relative_down(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_look_relative_left; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f3f0
 */
void hs_evaluate_player_action_test_look_relative_left(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_look_relative_left(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_look_relative_right; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f420
 */
void hs_evaluate_player_action_test_look_relative_right(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_look_relative_right(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_look_relative_up; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f390
 */
void hs_evaluate_player_action_test_look_relative_up(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_look_relative_up(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_move_relative_all_directions; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f480
 */
void hs_evaluate_player_action_test_move_relative_all_directions(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_move_relative_all_directions(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_primary_trigger; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f240
 */
void hs_evaluate_player_action_test_primary_trigger(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_primary_trigger(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f1f0
 */
void hs_evaluate_player_action_test_reset(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_reset(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_zoom; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f2a0
 */
void hs_evaluate_player_action_test_zoom(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_zoom(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_add_equipment; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f4b0
 */
void hs_evaluate_player_add_equipment(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_add_equipment(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_camera_control; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f170
 */
void hs_evaluate_player_camera_control(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_camera_control(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_effect_set_max_rotation; forwards to the C++ implementation unchanged.
 *
 * @address 0x480870
 */
void hs_evaluate_player_effect_set_max_rotation(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_effect_set_max_rotation(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_effect_set_max_translation; forwards to the C++ implementation unchanged.
 *
 * @address 0x480810
 */
void hs_evaluate_player_effect_set_max_translation(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_effect_set_max_translation(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_enable_input; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f120
 */
void hs_evaluate_player_enable_input(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_enable_input(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_players; forwards to the C++ implementation unchanged.
 *
 * @address 0x47a410
 */
void hs_evaluate_players(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_players(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_action_test_action; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f2d0
 */
void hs_evaluate_player_action_test_action(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_action_test_action(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_effect_start; forwards to the C++ implementation unchanged.
 *
 * @address 0x4808e0
 */
void hs_evaluate_player_effect_start(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_effect_start(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_player_effect_stop; forwards to the C++ implementation unchanged.
 *
 * @address 0x480970
 */
void hs_evaluate_player_effect_stop(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_player_effect_stop(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::PlayerCommands::evaluate_players_unzoom_all; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f100
 */
void hs_evaluate_players_unzoom_all(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::PlayerCommands::evaluate_players_unzoom_all(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_cache_dump_to_file; forwards to the C++ implementation unchanged.
 *
 * @address 0x47f6e0
 */
void hs_evaluate_sound_cache_dump_to_file(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_cache_dump_to_file(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_class_set_gain; forwards to the C++ implementation unchanged.
 *
 * @address 0x47ffe0
 */
void hs_evaluate_sound_class_set_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_class_set_gain(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_eax_enabled; forwards to the C++ implementation unchanged.
 *
 * @address 0x4815b0
 */
void hs_evaluate_sound_eax_enabled(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_eax_enabled(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_enable; forwards to the C++ implementation unchanged.
 *
 * @address 0x480030
 */
void hs_evaluate_sound_enable(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_enable(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_enable_eax; forwards to the C++ implementation unchanged.
 *
 * @address 0x481560
 */
void hs_evaluate_sound_enable_eax(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_enable_eax(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_enable_hardware; forwards to the C++ implementation unchanged.
 *
 * @address 0x481650
 */
void hs_evaluate_sound_enable_hardware(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_enable_hardware(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_get_effects_gain; forwards to the C++ implementation unchanged.
 *
 * @address 0x4801a0
 */
void hs_evaluate_sound_get_effects_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_get_effects_gain(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_get_gain; forwards to the C++ implementation unchanged.
 *
 * @address 0x47ac70
 */
void hs_evaluate_sound_get_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_get_gain(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_get_master_gain; forwards to the C++ implementation unchanged.
 *
 * @address 0x4800c0
 */
void hs_evaluate_sound_get_master_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_get_master_gain(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_get_music_gain; forwards to the C++ implementation unchanged.
 *
 * @address 0x480130
 */
void hs_evaluate_sound_get_music_gain(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_get_music_gain(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_get_supplementary_buffers; forwards to the C++ implementation unchanged.
 *
 * @address 0x481720
 */
void hs_evaluate_sound_get_supplementary_buffers(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_get_supplementary_buffers(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_impulse_start; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fce0
 */
void hs_evaluate_sound_impulse_start(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_impulse_start(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_impulse_stop; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fda0
 */
void hs_evaluate_sound_impulse_stop(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_impulse_stop(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_impulse_time; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fd30
 */
void hs_evaluate_sound_impulse_time(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_impulse_time(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_looping_predict; forwards to the C++ implementation unchanged.
 *
 * @address 0x47fe20
 */
void hs_evaluate_sound_looping_predict(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_looping_predict(function_index, thread_index, first);
}

/**
 * C entry point for halo::hs::SoundCommands::evaluate_sound_looping_set_alternate; forwards to the C++ implementation unchanged.
 *
 * @address 0x47ff40
 */
void hs_evaluate_sound_looping_set_alternate(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::SoundCommands::evaluate_sound_looping_set_alternate(function_index, thread_index, first);
}

}
