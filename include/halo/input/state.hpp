/**
 * @file include/halo/input/state.hpp
 * The input module's internal engine variables as a service object (halo::input::input_state()). The variables live in the data
 * image (standalone/data) under their original link names; State holds a reference to each, so no input file declares
 * them. The members other modules need are in halo::input::Globals (api.hpp).
 */
#pragma once

#include <stdint.h>

/* The including file has already included the engine type headers (tags.h, memory.h, math.h, game.h, networking.h,
   interface.h, saved_games.h, input.h, win32.h): they carry no include guards. */

struct Globals;

namespace halo::input {

/** References to the input module's internal engine variables (bindings, device tables, key state, event queue). */
struct State {
    uint16_t (&missing_string_text)[];
    char (&input_action_names)[k_input_action_count][0x10];
    char (&pov_direction_names)[8][10];
    char (&joystick_button_prefix)[0x18];
    char (&decimal_suffixes)[0x20][3];
    char (&joystick_pov_prefix)[0x18];
    char (&joystick_axis_prefix)[0x18];
    input_device (&input_devices)[8];
    int16_t (&keyboard_bindings)[k_control_keyboard_key_count];
    int16_t (&mouse_button_bindings)[k_control_mouse_button_count];
    int16_t (&mouse_axis_bindings)[k_control_mouse_axis_count][2];
    int16_t (&gamepad_button_bindings)[k_control_gamepad_count][k_control_gamepad_button_count];
    int16_t (&gamepad_axis_bindings)[k_control_gamepad_count][k_control_gamepad_axis_count][2];
    int16_t (&gamepad_pov_bindings)[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count];
    uint8_t &game_engine_teams_enabled_flag;
    ::Globals *&global_globals;
    uint32_t &current_game_engine;
    uint8_t (&g_control_binding_region_e4)[0xa0];
    uint8_t (&g_control_binding_region_ec)[0x3c0];
    uint8_t (&g_control_binding_region_e0)[0x3c0];
    uint32_t &control_word_primary;
    uint32_t &control_binding_device_type;
    uint32_t &control_word_secondary;
    input_abstraction_globals &input_globals;
    joystick_state (&joystick_states)[4];
    joystick_state &joystick_neutral_state;
    int32_t &last_input_device;
    int16_t (&gamepad_action_buttons)[k_control_gamepad_count][2];
    void *&mouse_device;
    mouse_state &live_mouse_state;
    mouse_state &mouse_neutral_state;
    int32_t (&g_control_binding_id)[];
    int16_t (&g_control_binding_value)[];
    int32_t &input_device_count;
    void * (&joystick_devices)[8];
    uint8_t &input_acquired;
    void *&keyboard_device;
    int32_t &mouse_wheel_granularity;
    void *&direct_input8_create;
    input_guid &iid_directinput8a;
    void *&direct_input;
    int16_t &key_event_read_index;
    int16_t &key_event_count;
    uint8_t (&key_frames)[0x6d];
    uint8_t (&key_release_pending)[0x6d];
    int16_t (&scan_code_to_key)[0x100];
    int32_t &game_time_force_single_tick;
    di_data_format &joystick_data_format;
    int32_t &input_last_error;
    key_block_timer (&key_block_timers)[k_input_key_block_timer_count];
    int16_t (&system_keys)[k_input_system_key_count];
    ui_key_event (&key_events)[k_input_key_event_capacity];
    input_guid &guid_sys_keyboard;
    int16_t (&mouse_button_map)[k_input_mouse_button_count];
    input_guid &guid_sys_mouse;
    int16_t (&virtual_key_to_key)[0x100];
    int16_t (&character_to_key)[0x80];
    uint8_t (&mouse_axis_frames)[k_input_mouse_axis_count][2];
    uint8_t (&joystick_axis_frames)[4][0x20][2];
    uint8_t (&joystick_pov_frames)[4][0x10][8];
    real (&look_yaw_rate_setting)[k_maximum_local_players];
    real (&look_pitch_rate_setting)[k_maximum_local_players];
    float &mouse_acceleration;
    float &mouse_acceleration_cached;
    mouse_acceleration_point (&mouse_acceleration_defaults)[k_input_mouse_acceleration_point_count];
    mouse_acceleration_point (&mouse_acceleration_points)[k_input_mouse_acceleration_point_count];
    player_globals *&local_player_globals;
    data_array *&player_data;
    di_object_data_format (&joystick_objects)[k_input_joystick_object_count];
    int32_t &nojoystick;
    uint32_t &input_menu_exit_deadline;
    input_event_queue &input_event_queue_active;
    menu_repeat_state (&menu_repeat_states)[4];
    uint32_t &mouse_double_click_time;
    uint32_t &input_queue_sample_time;
};

/** The input state singleton (Meyers); input_state() is the short form the module uses. */
class StateService {
public:
    static State &instance();
};

inline State &input_state() { return StateService::instance(); }

}  // namespace halo::input
