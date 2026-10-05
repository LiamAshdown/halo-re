#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::input {

/**
 * Keyboard, mouse and joystick lifetime, polling and raw to engine state conversion, on halo::platform input (SDL2).
 * The raw samples keep the DirectInput layouts the original read, so the conversion code is unchanged.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct InputDevices {
    static int32_t device_count_by_guid(const uint32_t *guid);
    static uint32_t device_find_index_by_guid(controls_gamepad_record *record);
    static int32_t device_get_axis_count(int16_t slot_index);
    static int32_t device_get_button_count(int16_t slot_index);
    static int32_t device_get_pov_count(int16_t slot_index);
    static void device_list_print(void);
    static void device_release(int16_t slot_index);
    static void acquire(void);
    static uint8_t initialize(void);
    static void poll(void);
    static void release(void);
    static void unacquire(void);
    static void enumerate_joysticks(void);
    static uint8_t get_key_state(int16_t key_index);
    static uint8_t get_mouse_button_state(int16_t button_index);
    static uint8_t guid_parse_ansi(input_guid *out_guid, char *ansi);
    static void joystick_state_process(joystick_raw_state *raw, joystick_state *dest, input_device *device);
    static void key_block_timer_set(int16_t key, int32_t duration_ms);
    static void key_block_timers_expire(void);
    static uint8_t keyboard_device_create(void);
    static void keyboard_set_capture_mode(uint8_t enable_capture);
    static void keyboard_flush(void);
    static uint8_t mouse_device_create(void);
    static void mouse_state_process(mouse_state *dest, di_mouse_state2 *raw);
    static void record_windows_key_message(uint32_t wparam, int32_t message);
};

/** Logs an input error once per distinct error code (printf-style description). @address 0x492150 */
void input_error_log_once(int32_t error_code, const char *description, ...);

}
