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
void control_binding_table_initialize();
uint8_t control_binding_table_query(int32_t target, int32_t raw_id);
void control_binding_table_update_a();
void control_binding_table_update_b();
uint32_t control_word_extract_field(uint32_t which_word, uint32_t field_index);
uint8_t input_apply_control_binding(control_binding_descriptor *binding, int32_t action_index);
uint32_t input_device_default_profile_tag_find(input_guid device_guid, void *out_profile);
void input_scan_any_bound_input();
void test_input_device_defaults_find(char *device_id_ansi);
void control_binding_table_register_single(int32_t target, int32_t selector, int32_t raw_id, uint32_t raw_value);
void hs_bind_control(const char *device_class_name, const char *input_name, const char *action_name);
void hs_unbind_control(const char *device_class_name, const char *input_name);
uint8_t input_get_key_state(int16_t key_index);
/** Drops the queued key events and clears the key press/hold state (InputDevices::keyboard_flush). */
void keyboard_flush(void);

}
