#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

// Every pointer field in input.h is held as uint32_t, so the sizes below are exact on any host.
#define CHECK(name, cond) typedef char check_##name[(cond) ? 1 : -1]
#define OFF(t, f) __builtin_offsetof(t, f)

// struct sizes the binary states
CHECK(joystick_state, sizeof(joystick_state) == 0x28 * 4);
CHECK(joystick_raw_state, sizeof(joystick_raw_state) == 0xe0);
CHECK(di_mouse_state2, sizeof(di_mouse_state2) == 0x14);
CHECK(mouse_state, sizeof(mouse_state) == 7 * 4);
CHECK(input_device, sizeof(input_device) == 0x90 * 4);
CHECK(key_block_timer, sizeof(key_block_timer) == 8);
CHECK(menu_repeat_state, sizeof(menu_repeat_state) == 8);
CHECK(mouse_acceleration_point, sizeof(mouse_acceleration_point) == 0x10);
CHECK(input_event_queue, sizeof(input_event_queue) == 0x43 * 4);
CHECK(input_abstraction_globals, sizeof(input_abstraction_globals) == 0x97c * 4);
CHECK(di_object_data_format, sizeof(di_object_data_format) == 0x10);
CHECK(di_data_format, sizeof(di_data_format) == 0x18);
CHECK(di_property_dword, sizeof(di_property_dword) == 0x14);
CHECK(di_property_range, sizeof(di_property_range) == 0x18);
CHECK(di_device_object_data, sizeof(di_device_object_data) == 0x14);
CHECK(di_device_caps, sizeof(di_device_caps) == 0x2c);
CHECK(di_device_instance, sizeof(di_device_instance) == 0x244);
CHECK(di_device_object_instance, sizeof(di_device_object_instance) == 0x13c);
// types reused from other headers, at the sizes this module relies on
CHECK(player_control_settings, sizeof(player_control_settings) == 0x85c);
CHECK(local_player_input_state, sizeof(local_player_input_state) == 0x28);
CHECK(control_binding_descriptor, sizeof(control_binding_descriptor) == 0x0c);
CHECK(controls_gamepad_record, sizeof(controls_gamepad_record) == 0x220);
CHECK(ui_input_event, sizeof(ui_input_event) == 8);
CHECK(ui_key_event, sizeof(ui_key_event) == 4);

// joystick_state / joystick_raw_state (0x491fd0)
CHECK(js_axes, OFF(joystick_state, axes) == 0x20);
CHECK(js_povs, OFF(joystick_state, povs) == 0x60);
CHECK(jr_povs, OFF(joystick_raw_state, povs) == 0x80);
CHECK(jr_buttons, OFF(joystick_raw_state, buttons) == 0xc0);
// the neutral state POV block the initializer fills with -1 (0x006b2d58)
CHECK(js_neutral_povs, 0x006b2cf8 + OFF(joystick_state, povs) == 0x006b2d58);
// the scan compares axes at 0x006b2d18 (neutral) and 0x00712564 (baseline 0)
CHECK(js_neutral_axes, 0x006b2cf8 + OFF(joystick_state, axes) == 0x006b2d18);
CHECK(js_baseline_axes, 0x00710328 + OFF(input_abstraction_globals, scan_baselines) + OFF(joystick_state, axes) == 0x00712564);

// mouse_state (0x491bc0, and 0x006b1818 / 0x006b1820 in the readers)
CHECK(ms_wheel, OFF(mouse_state, wheel) == 0x08);
CHECK(ms_frames, 0x006b180c + OFF(mouse_state, button_frames) == 0x006b1818);
CHECK(ms_pressed, 0x006b180c + OFF(mouse_state, button_pressed) == 0x006b1820);
CHECK(ms_neutral_end, 0x006b1828 + sizeof(mouse_state) == 0x006b1844);
CHECK(dm_buttons, OFF(di_mouse_state2, buttons) == 0x0c);

// input_device (0x006b1868 base; +0x20c key at 0x006b1a74, slot at 0x006b1a98, counts)
CHECK(dev_key, 0x006b1868 + OFF(input_device, record) + OFF(controls_gamepad_record, product_guid) == 0x006b1a74);
CHECK(dev_instance, OFF(input_device, instance_guid) == 0x220);
CHECK(dev_slot, 0x006b1868 + OFF(input_device, slot) == 0x006b1a98);
CHECK(dev_axes, 0x006b1868 + OFF(input_device, axis_count) == 0x006b1a9c);
CHECK(dev_buttons, 0x006b1868 + OFF(input_device, button_count) == 0x006b1aa0);
CHECK(dev_povs, 0x006b1868 + OFF(input_device, pov_count) == 0x006b1aa4);
CHECK(dev_table_end, 0x006b1868 + k_input_maximum_devices * sizeof(input_device) == 0x006b2a68);
CHECK(js_table_end, 0x006b2a68 + 4 * sizeof(joystick_state) == 0x006b2ce8);
CHECK(js_neutral_end, 0x006b2cf8 + sizeof(joystick_state) == 0x006b2d98);

// key tables (0x006b1600 .. 0x006b1800)
CHECK(kbt_key, 0x006b1600 + OFF(key_block_timer, key) == 0x006b1604);
CHECK(kbt_end, 0x006b1600 + k_input_key_block_timer_count * sizeof(key_block_timer) == 0x006b1620);
CHECK(key_release, 0x006b1620 + k_control_keyboard_key_count == 0x006b168d);
CHECK(key_ring_end, 0x006b16fe + k_input_key_event_capacity * sizeof(ui_key_event) == 0x006b17fe);

// menu_repeat_state (0x0068e4fc .. 0x0068e51c) and the acceleration tables
CHECK(menu_key, 0x0068e4fc + OFF(menu_repeat_state, key) == 0x0068e500);
CHECK(menu_last, 0x0068e4fc + 3 * sizeof(menu_repeat_state) + OFF(menu_repeat_state, key) == 0x0068e518);
CHECK(menu_end, 0x0068e4fc + 4 * sizeof(menu_repeat_state) == 0x0068e51c);
CHECK(accel_defaults_end, 0x0068e41c + k_input_mouse_acceleration_point_count * sizeof(mouse_acceleration_point) == 0x0068e48c);
CHECK(accel_points_end, 0x0068e48c + k_input_mouse_acceleration_point_count * sizeof(mouse_acceleration_point) == 0x0068e4fc);
CHECK(accel_boost, OFF(mouse_acceleration_point, boost) == 0x0c);

// DirectInput object table (0x00879f60 .. 0x0087a460)
CHECK(joy_objects_end, 0x00879f60 + k_input_joystick_object_count * sizeof(di_object_data_format) == 0x0087a460);
CHECK(dif_objects, OFF(di_data_format, objects) == 0x14);
CHECK(dii_product, OFF(di_device_instance, product_guid) == 0x14);
CHECK(dii_name, OFF(di_device_instance, instance_name) == 0x28);
CHECK(doi_type, OFF(di_device_object_instance, type) == 0x18);
CHECK(doi_name, OFF(di_device_object_instance, name) == 0x20);
CHECK(caps_axes, OFF(di_device_caps, axis_count) == 0x0c);
CHECK(caps_povs, OFF(di_device_caps, pov_count) == 0x14);

// input_event_queue (0x00712cc0; pop scans 0x00712d04, push shifts from 0x00712cd4)
CHECK(q_last, 0x00712cc0 + OFF(input_event_queue, last_event_time) == 0x00712cc4);
CHECK(q_start, 0x00712cc0 + OFF(input_event_queue, start_time) == 0x00712cc8);
CHECK(q_events, 0x00712cc0 + OFF(input_event_queue, events) == 0x00712ccc);
CHECK(q_slot1, 0x00712cc0 + OFF(input_event_queue, events) + sizeof(ui_input_event) == 0x00712cd4);
CHECK(q_slot7, 0x00712cc0 + OFF(input_event_queue, events) + 7 * sizeof(ui_input_event) == 0x00712d04);
CHECK(q_stride, 8 * sizeof(ui_input_event) == 0x40);

// input_abstraction_globals (base 0x00710328)
#define G(f) (0x00710328 + OFF(input_abstraction_globals, f))
CHECK(g_states, G(states) == 0x00712498);
CHECK(g_time, G(time_base) == 0x00712538);
CHECK(g_2214, G(unknown_2214) == 0x0071253c);
CHECK(g_idle, G(idle) == 0x00712540);
CHECK(g_2219, G(unknown_2219) == 0x00712541);
CHECK(g_mode, G(mode_flags) == 0x00712542);
CHECK(g_baselines, G(scan_baselines) == 0x00712544);
CHECK(g_scan, G(scan_result) == 0x007127c4);
CHECK(g_system, G(system_key_states) == 0x007127d0);
CHECK(g_last, G(last_used_bindings) == 0x007127d4);
CHECK(g_end, 0x00710328 + sizeof(input_abstraction_globals) == 0x00712918);
// settings tables of controller 0 (the absolute addresses the decompile uses)
CHECK(s_stride, sizeof(player_control_settings) == 0x85c);
CHECK(s_keyboard, 0x00710328 + 0x008 == 0x00710330);
CHECK(s_scale_x, 0x00710328 + OFF(player_control_settings, gamepad_axis_scale_x) == 0x00710b58);
CHECK(s_invert, 0x00710328 + OFF(player_control_settings, look_inverted) == 0x00710b80);
// the scan result words (0x007127c6 device index, 0x007127ca input index)
CHECK(scan_device, G(scan_result) + OFF(control_binding_descriptor, device_index) == 0x007127c6);
CHECK(scan_index, G(scan_result) + OFF(control_binding_descriptor, input_index) == 0x007127ca);
CHECK(scan_dir, G(scan_result) + OFF(control_binding_descriptor, direction) == 0x007127cc);
// last-used table field addresses (0x007127d6 / d8 / da / dc)
CHECK(lu_index, G(last_used_bindings) + OFF(control_binding_descriptor, input_index) == 0x007127da);
CHECK(lu_dir, G(last_used_bindings) + OFF(control_binding_descriptor, direction) == 0x007127dc);
// local_player_input_state axes the accumulator writes (0x007124ac .. 0x007124bc)
CHECK(st_throttle_x, G(states) + OFF(local_player_input_state, throttle_x) == 0x007124ac);
CHECK(st_look_y, G(states) + OFF(local_player_input_state, look_y) == 0x007124b8);
CHECK(st_analog, G(states) + OFF(local_player_input_state, look_is_analog) == 0x007124bc);

// enum values the code tests
CHECK(actions, k_input_action_count == 0x1b && _input_action_look_right == 0x1a);
CHECK(unbound, (int)k_input_unbound == (int)k_control_binding_unbound);
CHECK(virtual_keys, _input_key_any_shift == 0x6e && _input_key_any_alt == 0x71);
CHECK(modes, _input_mode_bind_scan_bit == 8);

int main(void) { return 0; }
