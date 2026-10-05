/**
 * Keyboard, mouse and joystick lifetime, polling and raw to engine state conversion on halo::platform input
 * (SDL2). The raw samples keep the DirectInput layouts the original read, so the conversion is unchanged.
 */

#include "halo/core/datum.hpp"
#include "tags.h"
#include "halo/text/api.hpp"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <wchar.h>
#include <string.h>
#include "crt.h"
#include <stdarg.h>

#include "halo/input/devices.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/input/state.hpp"
#include "halo/input/directinput_constants.hpp"
#include "halo/core/win32_constants.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/platform/time.hpp"
#include "halo/platform/input.hpp"
#include <stdio.h>

namespace halo::input {

/**
 * Counts how many registered input devices have a product GUID (device_key[0..3]) matching the
 * one pointed to by guid.
 *
 * @address 0x491d30
 */
int32_t InputDevices::device_count_by_guid(const uint32_t *guid)
{
    int32_t count;
    int32_t i;
    int32_t k;
    uint8_t match;

    count = 0;
    for (i = 0; i < input_state().input_device_count; i++) {
        match = 1;
        for (k = 0; k < 4; k++) {
            if (guid[k] != input_state().input_devices[i].record.product_guid.words[k]) {
                match = 0;
                break;
            }
        }
        if (match) {
            count = count + 1;
        }
    }
    return count;
}

}

namespace halo::input {

/**
 * Finds the index of the registered input device whose device_key matches record's (instance
 * number first, then the 4-dword product GUID), or 0xffffffff if none match.
 *
 * @address 0x4916e0
 */
uint32_t InputDevices::device_find_index_by_guid(controls_gamepad_record *record)
{
    int32_t i;
    int32_t k;
    uint8_t match;

    for (i = 0; i < input_state().input_device_count; i++) {
        if (record->product_instance == input_state().input_devices[i].record.product_instance) {
            match = 1;
            for (k = 0; k < 4; k++) {
                if (record->product_guid.words[k] != input_state().input_devices[i].record.product_guid.words[k]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                return (uint32_t)(uint16_t)i;
            }
        }
    }
    return 0xffffffff;
}

}

namespace halo::input {

/**
 * Returns the axis count of the input device mapped to joystick slot slot_index, or 0 if the
 * slot has no device mapped.
 *
 * Original register convention: slot in ECX.
 *
 * @address 0x491610
 */
int32_t InputDevices::device_get_axis_count(int16_t slot_index)
{
    int32_t device_index;
    int32_t result;

    result = 0;
    device_index = globals().joystick_slot_devices[slot_index];
    if (device_index != -1) {
        result = input_state().input_devices[device_index].axis_count;
    }
    return result;
}

}

namespace halo::input {

/**
 * Returns the button count of the input device mapped to joystick slot slot_index, or 0 if the
 * slot has no device mapped.
 *
 * Original register convention: slot in ECX.
 *
 * @address 0x491630
 */
int32_t InputDevices::device_get_button_count(int16_t slot_index)
{
    int32_t device_index;
    int32_t result;

    result = 0;
    device_index = globals().joystick_slot_devices[slot_index];
    if (device_index != -1) {
        result = input_state().input_devices[device_index].button_count;
    }
    return result;
}

}

namespace halo::input {

/**
 * Returns the POV-hat count of the input device mapped to joystick slot slot_index, or 0 if
 * the slot has no device mapped.
 *
 * Original register convention: slot in ECX.
 *
 * @address 0x491650
 */
int32_t InputDevices::device_get_pov_count(int16_t slot_index)
{
    int32_t device_index;
    int32_t result;

    result = 0;
    device_index = globals().joystick_slot_devices[slot_index];
    if (device_index != -1) {
        result = input_state().input_devices[device_index].pov_count;
    }
    return result;
}

}

namespace halo::input {

/**
 * Debug/test routine: logs "<index>) deviceid <guid> - <name>" for every registered input
 * device.
 *
 * @address 0x491750
 */
void InputDevices::device_list_print(void)
{
    int32_t index;
    char name_ascii[k_device_name_ascii_capacity];
    uint32_t i;

    for (index = 0; index < input_state().input_device_count; index++) {
        const controls_gamepad_record &record = input_state().input_devices[index].record;
        const uint32_t *guid = record.product_guid.words;
        char guid_ascii[0x27];
        uint32_t length = (uint32_t)wcslen((const wchar_t *)record.name);

        snprintf(guid_ascii, sizeof(guid_ascii), "{%08X-%04X-%04X-%04X-%04X%08X}", guid[0], guid[1] & 0xffff, guid[1] >> 16,
            ((guid[2] & 0xff) << 8) | ((guid[2] >> 8) & 0xff), ((guid[2] >> 16 & 0xff) << 8) | (guid[2] >> 24),
            ((guid[3] & 0xff) << 24) | ((guid[3] >> 8 & 0xff) << 16) | ((guid[3] >> 16 & 0xff) << 8) | (guid[3] >> 24));
        if (length >= k_device_name_ascii_capacity) {
            continue;
        }
        for (i = 0; i < length; i++) {
            name_ascii[i] = ((uint8_t *)record.name)[i * 2 + 1] == 0 ? ((char *)record.name)[i * 2] : ' ';
        }
        name_ascii[i] = '\0';

        halo::interface::console_printf_verbose((ColorARGB *)0, "%d) deviceid %s - %s", index, guid_ascii, name_ascii);
    }
}

}

namespace halo::input {

/**
 * Closes the joystick in device slot slot_index (if any), clears its handle, and zeroes its cached
 * input_devices entry.
 *
 * @address 0x491f80
 */
void InputDevices::device_release(int16_t slot_index)
{
    void *device;

    device = input_state().joystick_devices[slot_index];
    if (device != 0) {
        halo::platform::joystick_close(device);
        input_state().joystick_devices[slot_index] = 0;

        memset(&input_state().input_devices[slot_index], 0, 0x90 * sizeof(uint32_t));
    }
}

}

namespace halo::input {

/**
 * Marks input as active and captures the keyboard and mouse for the game (relative mouse motion,
 * no cursor). The wheel arrives in 120ths of a notch.
 *
 * @address 0x490620
 */
void InputDevices::acquire(void)
{
    input_state().input_acquired = 1;
    input_state().mouse_wheel_granularity = 120;
    halo::platform::input_set_captured(true);
}

}

namespace halo::input {

/**
 * Starts platform input and, on success, sets up the keyboard, mouse, and joysticks, then
 * captures everything. Returns nonzero on success.
 *
 * @address 0x490520
 */
uint8_t InputDevices::initialize(void)
{
    if (!halo::platform::input_initialize()) {
        input_error_log_once(1, "input initialize");
        halo::input::InputDevices::release();
        return 0;
    }
    halo::input::InputDevices::keyboard_device_create();
    halo::input::InputDevices::mouse_device_create();
    halo::input::InputSystem::system_initialize();
    halo::input::InputDevices::acquire();
    return 1;
}

}

namespace halo::input {

/**
 * Per-frame poll: ages the keyboard hold counters and drains the queued key events, reads the
 * mouse and every mapped joystick's raw state into the engine's mouse_state/joystick_states
 * (reseeding a joystick to neutral when it is gone), while clearing input_suppressed and the key
 * event ring.
 *
 * @address 0x490760
 */
void InputDevices::poll(void)
{
    halo::platform::key_event event;
    di_mouse_state2 mouse_raw;
    joystick_raw_state joystick_raw;
    int32_t i;
    int32_t key_index;
    int32_t slot;

    static_assert(sizeof(halo::platform::mouse_sample) == sizeof(di_mouse_state2), "mouse sample layout");
    static_assert(sizeof(halo::platform::joystick_sample) == sizeof(joystick_raw_state), "joystick sample layout");

    if (input_state().input_acquired == 0) {
        return;
    }

    globals().suppressed = 0;
    input_state().key_event_read_index = 0;
    input_state().key_event_count = 0;

    if (input_state().keyboard_device != 0) {
        for (i = 0; i < 0x6d; i++) {
            if (input_state().key_release_pending[i] == 1) {
                input_state().key_frames[i] = 0;
            } else if (input_state().key_frames[i] != 0) {
                input_state().key_frames[i] = (input_state().key_frames[i] < 0xff) ? (uint8_t)(input_state().key_frames[i] + 1) : 0xff;
            }
        }
        for (i = 0; i < 0x6d; i++) {
            input_state().key_release_pending[i] = 0;
        }

        while (halo::platform::keyboard_next_event(&event)) {
            key_index = input_state().scan_code_to_key[event.scan_code];
            if (key_index == -1) {
                continue;
            }
            if (key_index == _input_key_tab && (halo::platform::input_modifiers() & halo::platform::k_modifier_alt) != 0) {
                input_state().key_frames[_input_key_tab] = 0;
            } else if (event.down) {
                input_state().key_frames[key_index] = 1;
            } else if (input_state().key_frames[key_index] == 1 && (input_state().input_globals.mode_flags & _input_mode_menu_bit) == 0) {
                input_state().key_release_pending[key_index] = 1;
            } else {
                input_state().key_frames[key_index] = 0;
            }
        }
    }

    if (input_state().mouse_device != 0 && input_state().game_time_force_single_tick == 0) {
        halo::platform::mouse_read(reinterpret_cast<halo::platform::mouse_sample *>(&mouse_raw));
        halo::input::InputDevices::mouse_state_process(&input_state().live_mouse_state, &mouse_raw);
    }

    if (input_state().game_time_force_single_tick != 0) {
        return;
    }
    for (i = 0; i < 8; i++) {
        if (i < input_state().input_device_count && input_state().input_devices[i].slot != -1 && input_state().joystick_devices[i] != 0) {
            slot = input_state().input_devices[i].slot;
            if (halo::platform::joystick_read(input_state().joystick_devices[i], reinterpret_cast<halo::platform::joystick_sample *>(&joystick_raw))) {
                halo::input::InputDevices::joystick_state_process(&joystick_raw, &input_state().joystick_states[slot], &input_state().input_devices[i]);
            } else {
                input_state().joystick_states[slot] = input_state().joystick_neutral_state;
            }
        }
    }
}

}


namespace halo::input {

/**
 * Closes every joystick (clearing their cached input_devices entries), lets go of the mouse and
 * keyboard, and stops platform input.
 *
 * @address 0x490580
 */
void InputDevices::release(void)
{
    int32_t i;

    for (i = 0; i < 8; i++) {
        halo::input::InputDevices::device_release((int16_t)i);
    }
    input_state().mouse_device = 0;
    input_state().keyboard_device = 0;
    halo::platform::input_shutdown();
}

}

namespace halo::input {

/**
 * Releases the game's capture of the keyboard and mouse and marks input as inactive.
 *
 * @address 0x4906e0
 */
void InputDevices::unacquire(void)
{
    input_state().input_acquired = 0;
    halo::platform::input_set_captured(false);
}

}

namespace halo::input {

/**
 * Registers up to 8 attached joysticks in input_devices: product GUID and name (with a " (N)"
 * suffix for a repeated product), its axis/button/hat counts clamped to the engine's limits, and
 * no slot yet. Replaces the DirectInput EnumDevices callback at 0x491d70.
 */
void InputDevices::enumerate_joysticks(void)
{
    int32_t count = halo::platform::joystick_count();
    int32_t i;

    for (i = 0; i < count && input_state().input_device_count < 8; i++) {
        int32_t index = input_state().input_device_count;
        input_device *device = &input_state().input_devices[index];
        halo::platform::joystick_info info;
        void *handle = halo::platform::joystick_open(i, &info);

        if (handle == nullptr) {
            input_error_log_once(i + 0x100, "open joystick %d", i);
            continue;
        }
        device->record.product_instance = (uint8_t)halo::input::InputDevices::device_count_by_guid(info.guid);
        memcpy(&device->instance_guid, info.guid, sizeof(device->instance_guid));
        memcpy(&device->record.product_guid, info.guid, sizeof(device->record.product_guid));
        halo::text::string_convert_ascii_to_unicode(device->record.name, 0x20a, info.name);
        if (device->record.product_instance != 0 && wcslen((const wchar_t *)device->record.name) < 0xfc) {
            uint16_t suffix[8];

            halo::text::string_format_wide_va(suffix, (const uint16_t *)L" (%d)", device->record.product_instance + 1);
            wcscat((wchar_t *)device->record.name, (const wchar_t *)suffix);
        }
        device->slot = -1;
        device->axis_count = info.axis_count > 0x20 ? 0x20 : info.axis_count;
        device->button_count = info.button_count > 0x20 ? 0x20 : info.button_count;
        device->pov_count = info.pov_count > 0x10 ? 0x10 : info.pov_count;
        input_state().joystick_devices[index] = handle;
        input_state().input_device_count = index + 1;
    }
}

}

namespace halo::input {


}

/**
 * Formats description (printf-style, with any varargs) into a scratch buffer, but only the
 * first time error_code is seen; repeats of the same HRESULT-like code are suppressed. Every
 * call site in this build passes a literal description with no varargs, so the formatted text
 * is only ever the description itself, but the varargs plumbing is preserved.
 *
 * @address 0x492150
 */
void halo::input::input_error_log_once(int32_t error_code, const char *description, ...)
{
    char message[4092];
    va_list args;

    if (error_code != input_state().input_last_error) {
        input_state().input_last_error = error_code;
        va_start(args, description);
        vsprintf(message, description, args);
        va_end(args);
    }
}

namespace halo::input {

/**
 * Returns the current hold-frame count for key_index, or 0 while input is suppressed. The four
 * virtual either-side modifier keys resolve to the larger (or, for windows/alt, the
 * tie-broken-toward-the-second) of their two physical keys. Every other key is blocked (reads as
 * 0) while a key_block_timer names it, otherwise returns its raw key_frames byte.
 *
 * Original register convention: key_index in ECX.
 *
 * @address 0x490b50
 */
uint8_t InputDevices::get_key_state(int16_t key_index)
{
    int32_t i;

    if (globals().suppressed != 0) {
        return 0;
    }

    switch (key_index - _input_key_any_shift) {
    case 0:
        if (input_state().key_frames[_input_key_right_shift] < input_state().key_frames[_input_key_left_shift]) {
            return input_state().key_frames[_input_key_left_shift];
        }
        return input_state().key_frames[_input_key_right_shift];

    case 1:
        if (input_state().key_frames[_input_key_right_control] < input_state().key_frames[_input_key_left_control]) {
            return input_state().key_frames[_input_key_left_control];
        }
        return input_state().key_frames[_input_key_right_control];

    case 2:
        if (input_state().key_frames[_input_key_left_windows] <= input_state().key_frames[_input_key_right_windows]) {
            return input_state().key_frames[_input_key_right_windows];
        }
        return input_state().key_frames[_input_key_left_windows];

    case 3:
        if (input_state().key_frames[_input_key_left_alt] <= input_state().key_frames[_input_key_right_alt]) {
            return input_state().key_frames[_input_key_right_alt];
        }
        return input_state().key_frames[_input_key_left_alt];

    default:
        if (key_index != -1) {
            for (i = 0; i < k_input_key_block_timer_count; i++) {
                if (input_state().key_block_timers[i].key == key_index) {
                    return 0;
                }
            }
        }
        return input_state().key_frames[key_index];
    }
}

}

namespace halo::input {

/**
 * Returns the current hold-frame count for mouse button button_index, or 0 while the mouse
 * device is absent or input is suppressed.
 *
 * @address 0x490e00
 */
uint8_t InputDevices::get_mouse_button_state(int16_t button_index)
{
    uint8_t result;

    result = 0;
    if (input_state().mouse_device != 0 && globals().suppressed == 0) {
        result = input_state().live_mouse_state.button_frames[button_index];
    }
    return result;
}

}

namespace halo::input {

/**
 * Parses a "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" device id (as CLSIDFromString did) into
 * out_guid. Returns nonzero on success.
 *
 * @address 0x491670
 */
uint8_t InputDevices::guid_parse_ansi(input_guid *out_guid, char *ansi)
{
    unsigned int data1;
    unsigned int data2;
    unsigned int data3;
    unsigned int bytes[8];
    uint8_t *out = (uint8_t *)out_guid;
    int i;

    if (sscanf(ansi, "{%8x-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x}", &data1, &data2, &data3, &bytes[0], &bytes[1], &bytes[2], &bytes[3],
            &bytes[4], &bytes[5], &bytes[6], &bytes[7]) != 11) {
        return 0;
    }
    memcpy(out, &data1, 4);
    out[4] = (uint8_t)data2;
    out[5] = (uint8_t)(data2 >> 8);
    out[6] = (uint8_t)data3;
    out[7] = (uint8_t)(data3 >> 8);
    for (i = 0; i < 8; i++) {
        out[8 + i] = (uint8_t)bytes[i];
    }
    return 1;
}

}

namespace halo::input {

/**
 * VERIFIED against disassembly 0x491fd0..0x49213f (2026-09-30): button saturation, the pov threshold cascade (negative or
 * 0xffff low word -> none, >= 0x83d6 -> north), axis copy and all three loop counts (+0x238/+0x23c/+0x234) match.
 * Normalizes a raw joystick sample (raw) into the engine's joystick_state (dest), for the axis
 * count, button count, and POV count that device reports: button hold-frame counters (saturating
 * at 255), POV hats quantized into 8 compass octants (or k_input_joystick_pov_none when
 * centered), and axis values passed through as int16.
 *
 * Original register convention: input_device in EBX.
 *
 * @address 0x491fd0
 */
void InputDevices::joystick_state_process(joystick_raw_state *raw, joystick_state *dest, input_device *device)
{
    int32_t i;
    int32_t angle;
    int32_t octant;

    for (i = 0; i < device->button_count; i++) {
        if ((raw->buttons[i] & 0x80) == 0) {
            dest->button_frames[i] = 0;
        } else if (dest->button_frames[i] < 0xff) {
            dest->button_frames[i] = dest->button_frames[i] + 1;
        } else {
            dest->button_frames[i] = 0xff;
        }
    }

    for (i = 0; i < device->pov_count; i++) {
        angle = (int32_t)raw->povs[i];
        if ((int16_t)angle == -1) {
            angle = -1;
        }

        if (angle < 0) {
            octant = k_input_joystick_pov_none;
        } else {
            octant = 0;
            for (int32_t candidate = 0; candidate < k_pov_octant_count; candidate++) {
                if (angle < k_pov_octant_half_width + candidate * k_pov_octant_width) {
                    octant = candidate;
                    break;
                }
            }
        }
        dest->povs[i] = octant;
    }

    for (i = 0; i < device->axis_count; i++) {
        dest->axes[i] = (int16_t)raw->axes[i];
    }
}

}

namespace halo::input {

/**
 * Schedules key to read as up (via input_get_key_state) for duration_ms milliseconds: reuses a
 * free key_block_timer entry, or the one with the earliest deadline if none is free. If key is
 * one of the three system keys (grave/escape/print screen), also clears its cached
 * system_key_states hold count.
 *
 * Original register convention: key in EDI, duration_ms on the stack.
 *
 * @address 0x490bf0
 */
void InputDevices::key_block_timer_set(int16_t key, int32_t duration_ms)
{
    key_block_timer *chosen;
    int32_t i;
    large_integer counter;
    uint32_t now;

    chosen = (key_block_timer *)0;
    for (i = 0; i < k_input_key_block_timer_count; i++) {
        if (input_state().key_block_timers[i].key == -1) {
            chosen = &input_state().key_block_timers[i];
            break;
        }
        if (chosen == (key_block_timer *)0 || input_state().key_block_timers[i].deadline < chosen->deadline) {
            chosen = &input_state().key_block_timers[i];
        }
    }

    if (chosen != (key_block_timer *)0) {
        halo::platform::read_performance_counter(&counter);
        now = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
        chosen->deadline = now + duration_ms;
        chosen->key = key;

        for (i = 0; i < k_input_system_key_count; i++) {
            if (input_state().system_keys[i] == key) {
                input_state().input_globals.system_key_states[i] = 0;
                return;
            }
        }
    }
}

}

namespace halo::input {

/**
 * Frees any key_block_timer entry whose deadline has passed.
 *
 * @address 0x490ca0
 */
void InputDevices::key_block_timers_expire(void)
{
    int32_t i;
    large_integer counter;
    uint32_t now;

    for (i = 0; i < k_input_key_block_timer_count; i++) {
        if (input_state().key_block_timers[i].deadline != halo::k_dword_none) {
            halo::platform::read_performance_counter(&counter);
            now = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
            if (input_state().key_block_timers[i].deadline <= now) {
                input_state().key_block_timers[i].deadline = halo::k_dword_none;
                input_state().key_block_timers[i].key = -1;
            }
        }
    }
}

}

namespace halo::input {

/**
 * Resets the keyboard runtime state and key block timers and marks the keyboard present. Always
 * returns 1.
 *
 * @address 0x4918a0
 */
uint8_t InputDevices::keyboard_device_create(void)
{
    static uint8_t keyboard_present;
    int32_t i;

    memset(input_state().key_frames, 0, sizeof(input_state().key_frames));
    input_state().key_event_read_index = 0;
    input_state().key_event_count = 0;
    memset(input_state().key_events, 0, sizeof(ui_key_event) * 0x10);
    memset(input_state().key_release_pending, 0, sizeof(input_state().key_release_pending));

    for (i = 0; i < k_input_key_block_timer_count; i++) {
        input_state().key_block_timers[i].deadline = halo::k_dword_none;
        input_state().key_block_timers[i].key = -1;
    }
    input_state().keyboard_device = &keyboard_present;
    return 1;
}

}

namespace halo::input {

/**
 * Switches the keyboard between normal input mode and rebind-capture mode, and flushes the
 * keyboard's queued events and press/hold state.
 *
 * @address 0x48b650
 */
void InputDevices::keyboard_set_capture_mode(uint8_t enable_capture)
{
    if (enable_capture == 0) {
        input_state().input_globals.mode_flags = input_state().input_globals.mode_flags & ~_input_mode_keyboard_capture_bit;
    } else {
        input_state().input_globals.mode_flags = input_state().input_globals.mode_flags | _input_mode_keyboard_capture_bit;
    }
    halo::input::InputDevices::keyboard_flush();
}

/**
 * Drops the keyboard's queued events and clears the per-key press/hold state, when the keyboard
 * is present (the flush the console, chat and on-screen keyboard do when they take or give back
 * the keyboard).
 */
void InputDevices::keyboard_flush(void)
{
    if (input_state().keyboard_device != 0) {
        halo::platform::keyboard_flush();
        memset(input_state().key_release_pending, 0, sizeof(input_state().key_release_pending));
        memset(input_state().key_frames, 0, sizeof(input_state().key_frames));
    }
}

}

namespace halo::input {

/**
 * Seeds mouse_button_map[0..1] (the platform reports the logical buttons, the system's left/right
 * swap already applied) and marks the mouse present. Always returns 1.
 *
 * @address 0x4919c0
 */
uint8_t InputDevices::mouse_device_create(void)
{
    static uint8_t mouse_present;

    input_state().mouse_button_map[0] = 0;
    input_state().mouse_button_map[1] = 2;
    input_state().mouse_device = &mouse_present;
    return 1;
}

}

namespace halo::input {

/**
 * Converts one raw DirectInput mouse sample into the engine's mouse_state: x copied, y negated,
 * wheel divided by mouse_wheel_granularity and negated (left unchanged if the granularity is
 * still 0), and each physical button's hold-frame count and (release-transition) pressed flag
 * updated through the left/right swap map.
 *
 * Original register convention: dest on the stack, raw in ECX.
 *
 * @address 0x491bc0
 */
void InputDevices::mouse_state_process(mouse_state *dest, di_mouse_state2 *raw)
{
    int32_t i;
    int32_t mapped_slot;
    uint8_t pressed_now;
    uint8_t old_frames;

    dest->x = raw->x;
    dest->y = -raw->y;
    if (input_state().mouse_wheel_granularity != 0) {
        dest->wheel = -(raw->z / input_state().mouse_wheel_granularity);
    }

    for (i = 0; i < k_input_mouse_button_count; i++) {
        mapped_slot = input_state().mouse_button_map[i];
        pressed_now = (raw->buttons[i] & 0x80) != 0;
        old_frames = dest->button_frames[mapped_slot];

        dest->button_pressed[mapped_slot] = (pressed_now == 0 && old_frames != 0) ? 1 : 0;

        if (pressed_now) {
            dest->button_frames[mapped_slot] = (old_frames < 0xff) ? (uint8_t)(old_frames + 1) : 0xff;
        } else {
            dest->button_frames[mapped_slot] = 0;
        }
    }
}

}

namespace halo::input {

/**
 * Records one WM_KEYDOWN/WM_SYSKEYDOWN or WM_CHAR/WM_SYSCHAR message, with the current
 * shift/control/alt state, into the key event ring, while input is acquired. A key-down message
 * with no mapped key index, or a char message whose raw character byte is 0xff, is dropped.
 * Ignores every other message.
 *
 * Original register convention: wparam in EAX, message in ECX.
 *
 * @address 0x490d10
 */
void InputDevices::record_windows_key_message(uint32_t wparam, int32_t message)
{
    ui_key_event event;
    uint32_t modifiers;

    if (input_state().input_acquired == 0) {
        return;
    }

    if (message == halo::win32::k_wm_keydown || message == halo::win32::k_wm_syskeydown) {
        event.key_code = input_state().virtual_key_to_key[wparam];
        if (event.key_code == -1) {
            return;
        }
        event.character = 0xff;
    } else if (message == halo::win32::k_wm_char || message == halo::win32::k_wm_syschar) {
        event.character = (uint8_t)wparam;
        event.key_code = -1;
        if (wparam < 0x80) {
            event.key_code = input_state().character_to_key[wparam];
        }
        if (event.character == 0xff) {
            return;
        }
    } else {
        return;
    }

    modifiers = halo::platform::input_modifiers();
    event.modifiers = 0;
    if ((modifiers & halo::platform::k_modifier_shift) != 0) {
        event.modifiers = event.modifiers | _input_modifier_shift_bit;
    }
    if ((modifiers & halo::platform::k_modifier_control) != 0) {
        event.modifiers = event.modifiers | _input_modifier_control_bit;
    }
    if ((modifiers & halo::platform::k_modifier_alt) != 0) {
        event.modifiers = event.modifiers | _input_modifier_alt_bit;
    }

    if (input_state().key_event_count < k_input_key_event_capacity) {
        input_state().key_events[input_state().key_event_count] = event;
        input_state().key_event_count = input_state().key_event_count + 1;
    }
}

}
