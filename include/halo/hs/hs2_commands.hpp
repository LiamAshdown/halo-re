#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

namespace halo::hs {

/**
 * Signature shared by the hs evaluate handlers: the index of the hs function being run, the datum index of the
 * thread running it, and a flag that is set on the first call for a given stack frame.
 */
using EvaluateFn = void (*)(int16_t function_index, uint32_t thread_index, char first);

/**
 * A contiguous, read-only run of the evaluate handlers of one command group, in source order.
 */
struct EvaluateCommandTable {
    const EvaluateFn *data;
    uint32_t count;
};

/**
 * Command group for the hs evaluate handlers "devices". Each static member is the handler of one hs function and
 * is reached through the extern "C" shim that keeps the original symbol name.
 */
class DeviceCommands {
public:
    [[nodiscard]] static EvaluateCommandTable commands() noexcept;

    static void evaluate_device_set_position(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_device_set_position_immediate(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_device_set_power(int16_t function_index, uint32_t thread_index, char first);
};

/**
 * Command group for the hs evaluate handlers "game". Each static member is the handler of one hs function and
 * is reached through the extern "C" shim that keeps the original symbol name.
 */
class GameCommands {
public:
    [[nodiscard]] static EvaluateCommandTable commands() noexcept;

    static void evaluate_disconnect(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_error_overflow_suppression(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_fade_in(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_fade_out(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_fast_setup_network_server(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_all_quiet(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_difficulty_get(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_difficulty_get_real(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_difficulty_set(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_is_cooperative(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_lost(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_revert(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_reverted(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_safe_to_save(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_safe_to_speak(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_save(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_save_cancel(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_save_no_timeout(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_save_totally_unsafe(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_saving(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_skip_ticks(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_speed(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_time(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_variant(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_game_won(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_garbage_collect_now(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_map_name(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_map_reset(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_multiplayer_map_name(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_profile_load(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_profile_unlock_solo_levels(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_quit(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_rcon(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_remote_player_stats(int16_t function_index, uint32_t thread_index, char first);
};

/**
 * Command group for the hs evaluate handlers "hud". Each static member is the handler of one hs function and
 * is reached through the extern "C" shim that keeps the original symbol name.
 */
class HudCommands {
public:
    [[nodiscard]] static EvaluateCommandTable commands() noexcept;

    static void evaluate_display_scenario_help(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_enable_hud_help_flash(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_blink_health(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_blink_motion_sensor(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_blink_shield(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_clear_messages(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_get_timer_ticks(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_help_flash_restart(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_set_help_text(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_set_objective_text(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_set_timer_position(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_set_timer_time(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_set_timer_warning_time(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_show_crosshair(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_show_health(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_show_motion_sensor(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_hud_show_shield(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_numeric_countdown_timer_get(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_numeric_countdown_timer_restart(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_numeric_countdown_timer_set(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_numeric_countdown_timer_stop(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_pause_hud_timer(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_show_hud(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_show_hud_help_text(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_show_hud_timer(int16_t function_index, uint32_t thread_index, char first);
};

/**
 * Command group for the hs evaluate handlers "objects". Each static member is the handler of one hs function and
 * is reached through the extern "C" shim that keeps the original symbol name.
 */
class ObjectCommands {
public:
    [[nodiscard]] static EvaluateCommandTable commands() noexcept;

    static void evaluate_effect_new(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_effect_new_on_object_marker(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_magic_melee_attack(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_magic_seat_name(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_beautify(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_can_take_damage(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_cannot_take_damage(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_cast(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_create(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_create_anew(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_create_anew_containing(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_create_containing(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_destroy(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_destroy_all(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_destroy_containing(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_pvs_clear(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_pvs_set_camera(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_pvs_set_object(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_set_collideable(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_set_facing(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_set_melee_attack_inhibited(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_set_permutation(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_set_ranged_attack_inhibited(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_set_scale(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_set_shield(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_teleport(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_object_type_predict(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_objects_attach(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_objects_can_see_flag(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_objects_can_see_object(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_objects_delete_by_definition(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_objects_detach(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_objects_dump_memory(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_objects_predict(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_scenery_animation_start(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_scenery_animation_start_at_frame(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_scenery_get_animation_time(int16_t function_index, uint32_t thread_index, char first);
};

/**
 * Command group for the hs evaluate handlers "flow". Each static member is the handler of one hs function and
 * is reached through the extern "C" shim that keeps the original symbol name.
 */
class FlowCommands {
public:
    [[nodiscard]] static EvaluateCommandTable commands() noexcept;

    static void evaluate_equality(int16_t function_index, uint32_t thread_index, char first);
    static int32_t evaluate_expression(datum_index node);
    static void evaluate_if(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_ignore_arguments(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_not(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_nothing(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_random(hs_thread *thread, uint32_t thread_index, char first);
    static void evaluate_random_range(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_real_random_range(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sleep(uint32_t unused_param_1, uint32_t thread_index, char first);
    static void evaluate_sleep_ticks(int16_t function_index, uint32_t thread_index, char first);
};

/**
 * Command group for the hs evaluate handlers "input_settings". Each static member is the handler of one hs function and
 * is reached through the extern "C" shim that keeps the original symbol name.
 */
class InputSettingsCommands {
public:
    [[nodiscard]] static EvaluateCommandTable commands() noexcept;

    static void evaluate_get_digital_forward_throttle(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_digital_pitch_increment(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_digital_strafe_throttle(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_digital_yaw_increment(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_gamepad_forward_threshold(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_gamepad_strafe_threshold(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_gamepad_yaw_scale(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_mouse_forward_threshold(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_mouse_pitch_scale(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_mouse_strafe_threshold(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_mouse_yaw_scale(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_pitch_rate(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_get_yaw_rate(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player0_joystick_set_is_normal(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player0_look_invert_pitch(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player0_look_pitch_is_inverted(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_digital_forward_throttle(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_digital_pitch_increment(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_digital_strafe_throttle(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_digital_yaw_increment(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_gamepad_forward_threshold(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_gamepad_strafe_threshold(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_mouse_forward_threshold(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_mouse_pitch_scale(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_mouse_strafe_threshold(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_mouse_yaw_scale(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_pitch_rate(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_yaw_rate(int16_t function_index, uint32_t thread_index, char first);
};

/**
 * Command group for the hs evaluate handlers "debug". Each static member is the handler of one hs function and
 * is reached through the extern "C" shim that keeps the original symbol name.
 */
class DebugCommands {
public:
    [[nodiscard]] static EvaluateCommandTable commands() noexcept;

    static void evaluate_help(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_inspect(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_list_count(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_list_get(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_message_metrics_dump(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_net_graph_clear(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_net_graph_show(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_print(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_print_binds(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_rasterizer_fixed_function_ambient(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_rasterizer_lights_reset_for_new_map(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_rasterizer_model_ambient_reflection_tint(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_render_lights(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_script_doc(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_script_recompile(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_script_screen_effect_set_value(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_set_gamma(int16_t function_index, uint32_t thread_index, char first);
};

/**
 * Command group for the hs evaluate handlers "input_devices". Each static member is the handler of one hs function and
 * is reached through the extern "C" shim that keeps the original symbol name.
 */
class InputDeviceCommands {
public:
    [[nodiscard]] static EvaluateCommandTable commands() noexcept;

    static void evaluate_input_activate_joy(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_input_deactivate_joy(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_input_find_default(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_input_find_joystick(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_input_get_joy_count(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_input_is_joy_active(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_input_show_joystick_info(int16_t function_index, uint32_t thread_index, char first);
};

/**
 * Command group for the hs evaluate handlers "recording". Each static member is the handler of one hs function and
 * is reached through the extern "C" shim that keeps the original symbol name.
 */
class RecordingCommands {
public:
    [[nodiscard]] static EvaluateCommandTable commands() noexcept;

    static void evaluate_play_update_history(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_playback(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_recording_kill(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_recording_play(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_recording_play_and_delete(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_recording_play_and_hover(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_recording_time(int16_t function_index, uint32_t thread_index, char first);
};

/**
 * Command group for the hs evaluate handlers "players". Each static member is the handler of one hs function and
 * is reached through the extern "C" shim that keeps the original symbol name.
 */
class PlayerCommands {
public:
    [[nodiscard]] static EvaluateCommandTable commands() noexcept;

    static void evaluate_player_action_test_accept(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_action(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_back(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_grenade_trigger(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_jump(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_look_relative_all_directions(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_look_relative_down(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_look_relative_left(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_look_relative_right(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_look_relative_up(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_move_relative_all_directions(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_primary_trigger(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_reset(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_action_test_zoom(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_add_equipment(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_camera_control(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_effect_set_max_rotation(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_effect_set_max_translation(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_effect_start(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_effect_stop(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_player_enable_input(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_players(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_players_unzoom_all(int16_t function_index, uint32_t thread_index, char first);
};

/**
 * Command group for the hs evaluate handlers "sound". Each static member is the handler of one hs function and
 * is reached through the extern "C" shim that keeps the original symbol name.
 */
class SoundCommands {
public:
    [[nodiscard]] static EvaluateCommandTable commands() noexcept;

    static void evaluate_sound_cache_dump_to_file(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_class_set_gain(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_eax_enabled(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_enable(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_enable_eax(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_enable_hardware(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_get_effects_gain(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_get_gain(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_get_master_gain(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_get_music_gain(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_get_supplementary_buffers(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_impulse_start(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_impulse_stop(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_impulse_time(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_looping_predict(int16_t function_index, uint32_t thread_index, char first);
    static void evaluate_sound_looping_set_alternate(int16_t function_index, uint32_t thread_index, char first);
};

}
