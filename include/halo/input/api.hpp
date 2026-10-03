/**
 * @file include/halo/input/api.hpp
 * Functions of the input module that other modules and the data tables call (namespace halo::input). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>

struct control_binding_descriptor;
struct controls_gamepad_record;
struct di_device_instance;
struct di_device_object_instance;
struct di_mouse_state2;
struct input_device;
struct input_guid;
struct joystick_raw_state;
struct joystick_state;
struct local_player_input_state;
struct mouse_state;
struct object;
struct player_control_settings;
struct saved_player_profile;
struct ui_input_event;

namespace halo::input {

/**
 * The engine globals the input module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    int32_t (&joystick_slot_devices)[4];
    uint8_t &suppressed;
    uint8_t &binding_state;
    uint8_t &binding_secondary_active;
};

/**
 * The input service singleton. instance() builds the Globals reference table on first use (Meyers singleton); the state it
 * refers to lives in the data image. globals() is the short form every caller uses.
 */
class Service {
public:
    static Globals &instance();
};

inline Globals &globals() { return Service::instance(); }

void chimera__axis_text(int16_t axis_index, uint8_t direction, uint16_t *out_text);
void chimera__button_text(int16_t button_index, uint16_t *out_text);
void chimera__pov_text(int16_t pov_index, int16_t direction_index, uint16_t *out_text);
int16_t input_action_name_to_index(char *name);
void input_get_binding_display_name(control_binding_descriptor *binding, uint16_t *out_text);
void control_binding_table_initialize();
uint8_t control_binding_table_query(int32_t target, int32_t raw_id);
void control_binding_table_update_a();
void control_binding_table_update_b();
uint32_t control_word_extract_field(uint32_t which_word, uint32_t field_index);
uint8_t input_apply_control_binding(control_binding_descriptor *binding, int32_t action_index);
void input_apply_named_device_default_profile(uint16_t *device_name);
void input_bind_capture_reset();
void input_bind_scan_set_active(uint8_t enable_scan);
uint32_t input_device_default_profile_tag_find(input_guid device_guid, void *out_profile);
uint8_t input_get_last_used_binding(int16_t action, control_binding_descriptor *out);
void input_last_used_binding_copy(int16_t action, control_binding_descriptor *source);
void input_scan_any_bound_input();
void test_input_device_defaults_find(char *device_id_ansi);
void control_binding_table_register_single(int32_t target, int32_t selector, int32_t raw_id, uint32_t raw_value);
void hs_bind_control(const char *device_class_name, const char *input_name, const char *action_name);
void hs_unbind_control(const char *device_class_name, const char *input_name);
float input_clamp_unit_float(float value);
void input_joystick_set_axis_scale_x(int16_t slot, float value);
void input_joystick_set_axis_scale_y(int16_t slot, float value);
void input_reset_state_and_axis_configs();
float input_sensitivity_to_turn_rate(float sensitivity);
uint32_t input_device_find_index_by_guid(controls_gamepad_record *record);
int32_t input_device_get_axis_count(int16_t slot_index);
int32_t input_device_get_button_count(int16_t slot_index);
int32_t input_device_get_pov_count(int16_t slot_index);
void input_device_list_print();
void input_directinput_acquire_devices();
uint8_t input_directinput_initialize();
void input_directinput_poll_devices();
void input_directinput_release_devices();
void input_directinput_unacquire_devices();
int32_t __stdcall input_enumerate_gamepad_callback(const di_device_instance *instance, void *reference);
int32_t __stdcall input_enumerate_gamepad_object_callback(const di_device_object_instance *object, void *reference);
uint8_t input_get_key_state(int16_t key_index);
uint8_t input_get_mouse_button_state(int16_t button_index);
void input_key_block_timer_set(int16_t key, int32_t duration_ms);
void input_keyboard_set_capture_mode(uint8_t enable_capture);
void input_record_windows_key_message(uint32_t wparam, int32_t message);
void input_queue_initialize();
uint8_t input_queue_pop_event(ui_input_event *out_event, int16_t queue_index);
void input_queue_push_event(int16_t queue_index, ui_input_event *record);
void input_queue_sample_time_update();
void input_print_bound_controls();
void input_state_initialize();
void input_time_base_resync();
void input_update_tick();

}
