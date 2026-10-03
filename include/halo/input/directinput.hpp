#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::input {

/**
 * DirectInput 8 device lifetime, polling and raw to engine state conversion.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct DirectInput {
    static int32_t device_count_by_guid(const uint32_t *guid);
    static uint32_t device_find_index_by_guid(controls_gamepad_record *record);
    static int32_t device_get_axis_count(int16_t slot_index);
    static int32_t device_get_button_count(int16_t slot_index);
    static int32_t device_get_pov_count(int16_t slot_index);
    static void device_list_print(void);
    static void device_release(int16_t slot_index);
    static void directinput_acquire_devices(void);
    static uint8_t directinput_initialize(void);
    static void directinput_poll_devices(void);
    static void directinput_release_devices(void);
    static void directinput_unacquire_devices(void);
    static int32_t enumerate_gamepad_callback(const di_device_instance *instance, void *reference);
    static int32_t enumerate_gamepad_object_callback(const di_device_object_instance *object, void *reference);
    static uint8_t get_key_state(int16_t key_index);
    static uint8_t get_mouse_button_state(int16_t button_index);
    static uint8_t guid_parse_ansi(input_guid *out_guid, char *ansi);
    static void key_block_timer_set(int16_t key, int32_t duration_ms);
    static void key_block_timers_expire(void);
    static uint8_t keyboard_device_create(void);
    static void keyboard_set_capture_mode(uint8_t enable_capture);
    static uint8_t mouse_device_create(void);
    static void record_windows_key_message(uint32_t wparam, int32_t message);
};

}
