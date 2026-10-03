/**
 * DirectInput 8 device lifetime, polling and raw to engine state conversion.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "win32.h"
#include <wchar.h>
#include <string.h>
#include "crt.h"
#include <stdarg.h>

#include "halo/input/directinput.hpp"
#include "halo/input/api.hpp"

extern "C" { extern int32_t input_device_count; }
extern "C" { extern input_device input_devices[8]; }
namespace halo::input {

/**
 * Counts how many registered input devices have a product GUID (device_key[0..3]) matching the
 * one pointed to by guid.
 *
 * @address 0x491d30
 */
int32_t DirectInput::device_count_by_guid(const uint32_t *guid)
{
    int32_t count;
    int32_t i;
    int32_t k;
    uint8_t match;

    count = 0;
    for (i = 0; i < input_device_count; i++) {
        match = 1;
        for (k = 0; k < 4; k++) {
            if (guid[k] != input_devices[i].record.product_guid.words[k]) {
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
uint32_t DirectInput::device_find_index_by_guid(controls_gamepad_record *record)
{
    int32_t i;
    int32_t k;
    uint8_t match;

    for (i = 0; i < input_device_count; i++) {
        if (record->product_instance == input_devices[i].record.product_instance) {
            match = 1;
            for (k = 0; k < 4; k++) {
                if (record->product_guid.words[k] != input_devices[i].record.product_guid.words[k]) {
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

extern "C" { extern int32_t joystick_slot_devices[4]; }
namespace halo::input {

/**
 * Returns the axis count of the input device mapped to joystick slot slot_index, or 0 if the
 * slot has no device mapped.
 *
 * Original register convention: slot in ECX.
 *
 * @address 0x491610
 */
int32_t DirectInput::device_get_axis_count(int16_t slot_index)
{
    int32_t device_index;
    int32_t result;

    result = 0;
    device_index = joystick_slot_devices[slot_index];
    if (device_index != -1) {
        result = input_devices[device_index].axis_count;
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
int32_t DirectInput::device_get_button_count(int16_t slot_index)
{
    int32_t device_index;
    int32_t result;

    result = 0;
    device_index = joystick_slot_devices[slot_index];
    if (device_index != -1) {
        result = input_devices[device_index].button_count;
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
int32_t DirectInput::device_get_pov_count(int16_t slot_index)
{
    int32_t device_index;
    int32_t result;

    result = 0;
    device_index = joystick_slot_devices[slot_index];
    if (device_index != -1) {
        result = input_devices[device_index].pov_count;
    }
    return result;
}

}

extern "C" { extern void console_printf_verbose(ColorARGB *color, char *format, ...); }
namespace halo::input {

/**
 * Debug/test routine: logs "<index>) deviceid <guid> - <name>" for every registered input
 * device. Dead code in this build: nothing calls it.
 *
 * @address 0x491750
 */
void DirectInput::device_list_print(void)
{
    int32_t index;
    controls_gamepad_record record;
    uint16_t guid_wide[0x27];
    char guid_ascii[0x27];
    char guid_ascii_trimmed[0x27];
    char name_ascii[0x105];
    uint32_t length;
    uint32_t i;
    int32_t hr;

    for (index = 0; index < input_device_count; index++) {
        record = input_devices[index].record;

#ifdef __cplusplus
        hr = StringFromGUID2(*(const GUID *)&record.product_guid, (LPOLESTR)guid_wide, 0x27);
#else
        hr = StringFromGUID2((REFGUID)&record.product_guid, guid_wide, 0x27);
#endif
        if (hr < 0) {
            continue;
        }

        length = (uint32_t)wcslen((const wchar_t *)guid_wide);
        if (length >= 0x27) {
            continue;
        }
        for (i = 0; i < length; i++) {
            guid_ascii[i] = ((uint8_t *)guid_wide)[i * 2 + 1] == 0 ? ((char *)guid_wide)[i * 2] : ' ';
        }
        guid_ascii[i] = '\0';
        strncpy(guid_ascii_trimmed, guid_ascii, 0x26);
        guid_ascii_trimmed[0x26] = '\0';

        length = (uint32_t)wcslen((const wchar_t *)record.name);
        if (length >= 0x105) {
            continue;
        }
        for (i = 0; i < length; i++) {
            name_ascii[i] = ((uint8_t *)record.name)[i * 2 + 1] == 0 ? ((char *)record.name)[i * 2] : ' ';
        }
        name_ascii[i] = '\0';

        console_printf_verbose((ColorARGB *)0, (char *)"%d) deviceid %s - %s", index, guid_ascii_trimmed, name_ascii);
    }
}

}

extern "C" { extern void *joystick_devices[8]; }
namespace halo::input {

/**
 * Unacquires and releases the DirectInput device object mapped to joystick slot slot_index (if
 * any), clears the device pointer, and zeroes its cached input_devices entry.
 *
 * Original register convention: slot index in ESI.
 *
 * @address 0x491f80
 */
void DirectInput::device_release(int16_t slot_index)
{
    void *device;
    void **vtable;
    uint32_t *cursor;
    int32_t count;

    device = joystick_devices[slot_index];
    if (device != 0) {
        vtable = *(void ***)device;
        ((idirectinputdevice8_unacquire_proc)vtable[8])(device);
        ((idirectinputdevice8_release_proc)vtable[2])(device);
        joystick_devices[slot_index] = 0;

        cursor = (uint32_t *)&input_devices[slot_index];
        for (count = 0x90; count != 0; count--) {
            *cursor = 0;
            cursor = cursor + 1;
        }
    }
}

}

extern "C" { extern uint8_t input_acquired; }
extern "C" { extern void *keyboard_device; }
extern "C" { extern void *mouse_device; }
extern "C" { extern int32_t mouse_wheel_granularity; }
extern "C" { extern void input_error_log_once(int32_t error_code, char *description, ...); }
namespace halo::input {

/**
 * Marks input as active and acquires the keyboard, mouse (also reading its wheel granularity),
 * and every connected joystick device; logs (but does not treat as fatal) any Acquire/GetProperty
 * failure.
 *
 * @address 0x490620
 */
void DirectInput::directinput_acquire_devices(void)
{
    int32_t i;
    void *device;
    void **vtable;
    int32_t hr;
    di_property_dword granularity;

    input_acquired = 1;

    if (keyboard_device != 0) {
        vtable = *(void ***)keyboard_device;
        hr = ((idirectinputdevice8_acquire_proc)vtable[7])(keyboard_device);
        if (hr < 0) {
            input_error_log_once(hr, (char *)"Acquire (keyboard)");
        }
    }

    if (mouse_device != 0) {
        vtable = *(void ***)mouse_device;
        hr = ((idirectinputdevice8_acquire_proc)vtable[7])(mouse_device);
        if (hr < 0) {
            input_error_log_once(hr, (char *)"Acquire (mouse)");
        } else {
            granularity.header.size = 0x14;
            granularity.header.header_size = 0x10;
            granularity.header.object = 8;
            granularity.header.how = 1;
            granularity.data = 0;
            hr = ((idirectinputdevice8_getproperty_proc)vtable[5])(mouse_device, 3, &granularity);
            if (hr >= 0) {
                mouse_wheel_granularity = granularity.data;
            }
        }
    }

    for (i = 0; i < 8; i++) {
        device = joystick_devices[i];
        if (device != 0) {
            vtable = *(void ***)device;
            hr = ((idirectinputdevice8_acquire_proc)vtable[7])(device);
            if (hr < 0) {
                input_error_log_once(hr, (char *)"Acquire (gamepad)");
            }
        }
    }
}

}

extern "C" { extern void *shell_instance; }
extern "C" { extern void *direct_input8_create; }
extern "C" { extern input_guid iid_directinput8a; }
extern "C" { extern void *direct_input; }
namespace halo::input {

/**
 * Creates the shared IDirectInput8A object and, on success, the keyboard, mouse, and joystick
 * device objects, then acquires everything; logs and releases whatever was created on failure.
 * Returns nonzero on success.
 *
 * @address 0x490520
 */
uint8_t DirectInput::directinput_initialize(void)
{
    int32_t hr;

    hr = ((directinput8create_proc)direct_input8_create)(shell_instance, 0x800,
        &iid_directinput8a, &direct_input, (void *)0);
    if (hr < 0) {
        input_error_log_once(hr, (char *)"DirectInputCreate");
        halo::input::input_directinput_release_devices();
    } else {
        halo::input::input_keyboard_device_create();
        halo::input::input_mouse_device_create();
        halo::input::input_system_initialize();
        halo::input::input_directinput_acquire_devices();
    }
    return hr >= 0;
}

}

extern "C" { extern uint8_t input_suppressed; }
extern "C" { extern int16_t key_event_read_index; }
extern "C" { extern int16_t key_event_count; }
extern "C" { extern uint8_t key_frames[0x6d]; }
extern "C" { extern uint8_t key_release_pending[0x6d]; }
extern "C" { extern int16_t scan_code_to_key[0x100]; }
extern "C" { extern input_abstraction_globals input_globals; }
extern "C" { extern int32_t game_time_force_single_tick; }
extern "C" { extern mouse_state live_mouse_state; }
extern "C" { extern joystick_state joystick_states[4]; }
extern "C" { extern joystick_state joystick_neutral_state; }
#define k_dierr_reacquire_a ((int32_t)0x8007000cu)
#define k_dierr_reacquire_b ((int32_t)0x8007001eu)
namespace halo::input {

/**
 * Per-frame poll: ages the keyboard hold counters and drains the buffered key events (handling
 * overflow and device loss), reads the mouse and every mapped joystick's raw state into the
 * engine's mouse_state/joystick_states (zeroing/reseeding them to neutral on failure), while
 * clearing input_suppressed and the key event ring.
 *
 * @address 0x490760
 */
void DirectInput::directinput_poll_devices(void)
{
    void **vtable;
    int32_t hr;
    uint32_t event_count;
    di_device_object_data event;
    di_mouse_state2 mouse_raw;
    joystick_raw_state joystick_raw;
    int32_t i;
    int32_t key_index;
    int32_t slot;

    if (input_acquired == 0) {
        return;
    }

    input_suppressed = 0;
    key_event_read_index = 0;
    key_event_count = 0;

    if (keyboard_device != 0) {
        for (i = 0; i < 0x6d; i++) {
            if (key_release_pending[i] == 1) {
                key_frames[i] = 0;
            } else if (key_frames[i] != 0) {
                key_frames[i] = (key_frames[i] < 0xff) ? (uint8_t)(key_frames[i] + 1) : 0xff;
            }
        }
        for (i = 0; i < 0x6d; i++) {
            key_release_pending[i] = 0;
        }

        event_count = 1;
        for (;;) {
            vtable = *(void ***)keyboard_device;
            hr = ((idirectinputdevice8_getdevicedata_proc)vtable[10])(keyboard_device, 0x14,
                &event, &event_count, 0);

            if (hr > 0) {
                if (hr == 1) {
                    input_error_log_once(1, (char *)"keyboard_buffer_overflow");
                    event_count = 0xffffffff;
                    ((idirectinputdevice8_getdevicedata_proc)vtable[10])(keyboard_device, 0x14,
                        (di_device_object_data *)0, &event_count, 0);
                } else {
                    input_error_log_once(hr, (char *)"IDirectInputDevice_GetDeviceData (mouse)");
                }
                break;
            }

            if (hr == 0) {
                if (event_count != 1) {
                    break;
                }
                key_index = scan_code_to_key[event.offset];
                if (key_index == -1) {
                    continue;
                }
                if (key_index == _input_key_tab && GetAsyncKeyState(0x12) < 0) {
                    key_frames[_input_key_tab] = 0;
                } else if (((uint8_t)event.data & 0x80) != 0) {
                    key_frames[key_index] = 1;
                } else if (key_frames[key_index] == 1 && (input_globals.mode_flags & _input_mode_menu_bit) == 0) {
                    key_release_pending[key_index] = 1;
                } else {
                    key_frames[key_index] = 0;
                }
                continue;
            }

            if (hr == k_dierr_reacquire_b || hr == k_dierr_reacquire_a) {
                ((idirectinputdevice8_acquire_proc)vtable[7])(keyboard_device);
            } else {
                input_error_log_once(hr, (char *)"IDirectInputDevice_GetDeviceData (mouse)");
            }
            break;
        }
    }

    if (mouse_device != 0 && game_time_force_single_tick == 0) {
        vtable = *(void ***)mouse_device;
        hr = ((idirectinputdevice8_getdevicestate_proc)vtable[9])(mouse_device, 0x14, &mouse_raw);
        if (hr == k_dierr_reacquire_b || hr == k_dierr_reacquire_a) {
            ((idirectinputdevice8_acquire_proc)vtable[7])(mouse_device);
        } else if (hr == 0) {
            halo::input::input_mouse_state_process(&live_mouse_state, &mouse_raw);
            goto joystick_poll;
        } else {
            input_error_log_once(hr, (char *)"GetDeviceState (mouse)");
        }
        if (hr < 0) {
            memset(&live_mouse_state, 0, sizeof(live_mouse_state));
        }
    }

joystick_poll:
    if (game_time_force_single_tick != 0) {
        return;
    }
    for (i = 0; i < 8; i++) {
        if (i < input_device_count && input_devices[i].slot != -1 && joystick_devices[i] != 0) {
            slot = input_devices[i].slot;

            vtable = *(void ***)joystick_devices[i];
            hr = ((idirectinputdevice8_poll_proc)vtable[25])(joystick_devices[i]);
            if (hr >= 0) {
                vtable = *(void ***)joystick_devices[i];
                hr = ((idirectinputdevice8_getdevicestate_proc)vtable[9])(joystick_devices[i], 0xe0, &joystick_raw);
            }

            if (hr == k_dierr_reacquire_b || hr == k_dierr_reacquire_a) {
                vtable = *(void ***)joystick_devices[i];
                ((idirectinputdevice8_acquire_proc)vtable[7])(joystick_devices[i]);
            } else if (hr == 0) {
                halo::input::input_joystick_state_process(&joystick_raw, &joystick_states[slot], &input_devices[i]);
                continue;
            } else {
                input_error_log_once(hr, (char *)"Poll/GetDeviceState (gamepad)");
            }

            if (hr < 0) {
                joystick_states[slot] = joystick_neutral_state;
            }
        }
    }
}

}

#undef k_dierr_reacquire_a
#undef k_dierr_reacquire_b

namespace halo::input {

/**
 * Unacquires and releases every joystick, the mouse, and the keyboard DirectInput device object
 * (clearing their cached input_devices entries), then releases the shared IDirectInput8A object.
 *
 * @address 0x490580
 */
void DirectInput::directinput_release_devices(void)
{
    int32_t i;
    void *device;
    void **vtable;
    uint32_t *cursor;
    int32_t count;

    for (i = 0; i < 8; i++) {
        device = joystick_devices[i];
        if (device != 0) {
            vtable = *(void ***)device;
            ((idirectinputdevice8_unacquire_proc)vtable[8])(device);
            ((idirectinputdevice8_release_proc)vtable[2])(device);
            joystick_devices[i] = 0;

            cursor = (uint32_t *)&input_devices[i];
            for (count = 0x90; count != 0; count--) {
                *cursor = 0;
                cursor = cursor + 1;
            }
        }
    }

    if (mouse_device != 0) {
        vtable = *(void ***)mouse_device;
        ((idirectinputdevice8_unacquire_proc)vtable[8])(mouse_device);
        ((idirectinputdevice8_release_proc)vtable[2])(mouse_device);
        mouse_device = 0;
    }
    if (keyboard_device != 0) {
        vtable = *(void ***)keyboard_device;
        ((idirectinputdevice8_unacquire_proc)vtable[8])(keyboard_device);
        ((idirectinputdevice8_release_proc)vtable[2])(keyboard_device);
        keyboard_device = 0;
    }
    if (direct_input != 0) {
        vtable = *(void ***)direct_input;
        ((idirectinput8_release_proc)vtable[2])(direct_input);
        direct_input = 0;
    }
}

}

namespace halo::input {

/**
 * Unacquires (but does not release) every joystick, the mouse, and the keyboard, then marks
 * input as inactive.
 *
 * @address 0x4906e0
 */
void DirectInput::directinput_unacquire_devices(void)
{
    int32_t i;
    void *device;
    void **vtable;
    int32_t hr;

    for (i = 0; i < 8; i++) {
        device = joystick_devices[i];
        if (device != 0) {
            vtable = *(void ***)device;
            hr = ((idirectinputdevice8_unacquire_proc)vtable[8])(device);
            if (hr < 0) {
                input_error_log_once(hr, (char *)"Unacquire (gamepad)");
            }
        }
    }

    if (mouse_device != 0) {
        vtable = *(void ***)mouse_device;
        hr = ((idirectinputdevice8_unacquire_proc)vtable[8])(mouse_device);
        if (hr < 0) {
            input_error_log_once(hr, (char *)"Unacquire (mouse)");
        }
    }

    input_acquired = 0;

    if (keyboard_device != 0) {
        vtable = *(void ***)keyboard_device;
        hr = ((idirectinputdevice8_unacquire_proc)vtable[8])(keyboard_device);
        if (hr < 0) {
            input_error_log_once(hr, (char *)"Unacquire (keyboard)");
        }
    }
}

}

extern "C" { extern di_data_format joystick_data_format; }
extern "C" { extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source); }
extern "C" { extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); }
typedef int32_t (__stdcall *idirectinputdevice8_getcapabilities_proc)(void *self, di_device_caps *caps);

typedef int32_t (__stdcall *idirectinputdevice8_enumobjects_proc)(void *self, void *callback, void *reference,
    uint32_t flags);

namespace halo::input {

/**
 * Implements input enumerate gamepad callback.
 *
 * @address 0x491d70
 */
int32_t DirectInput::enumerate_gamepad_callback(const di_device_instance *instance, void *reference)
{
    int32_t index = input_device_count;
    input_device *device = &input_devices[index];
    void *handle = 0;
    void **vtable;
    di_device_caps caps;
    int32_t hr;
    char *failed;

    (void)reference;
    hr = ((idirectinput8_createdevice_proc)(*(void ***)direct_input)[3])(direct_input,
        (input_guid *)&instance->instance_guid, &handle, 0);
    if (hr < 0) {
        failed = (char *)"CreateDevice (gamepad)";
        goto fail;
    }
    vtable = *(void ***)handle;
    hr = ((idirectinputdevice8_setcooplevel_proc)vtable[13])(handle, GetActiveWindow(), 5);
    if (hr < 0) {
        failed = (char *)"SetCooperativeLevel (gamepad)";
        goto fail;
    }
    hr = ((idirectinputdevice8_setdataformat_proc)(*(void ***)handle)[11])(handle, &joystick_data_format);
    if (hr < 0) {
        failed = (char *)"SetDataFormat (gamepad)";
        goto fail;
    }
    memset(&caps, 0, sizeof(caps));
    caps.size = sizeof(caps);
    hr = ((idirectinputdevice8_getcapabilities_proc)(*(void ***)handle)[3])(handle, &caps);
    if (hr < 0) {
        failed = (char *)"GetCapabilities (gamepad)";
        goto fail;
    }
    device->record.product_instance = (uint8_t)halo::input::input_device_count_by_guid((const uint32_t *)&instance->product_guid);
    device->instance_guid = instance->instance_guid;
    device->record.product_guid = instance->product_guid;
    string_convert_ascii_to_unicode(device->record.name, 0x20a, instance->instance_name);
    if (device->record.product_instance != 0 && wcslen((const wchar_t *)device->record.name) < 0xfc) {
        uint16_t suffix[8];

        string_format_wide_va(suffix, (const uint16_t *)L" (%d)", device->record.product_instance + 1);
        wcscat((wchar_t *)device->record.name, (const wchar_t *)suffix);
    }
    device->slot = -1;
    device->axis_count = caps.axis_count > 0x20 ? 0x20 : caps.axis_count;
    device->button_count = caps.button_count > 0x20 ? 0x20 : caps.button_count;
    device->pov_count = caps.pov_count > 0x10 ? 0x10 : caps.pov_count;
    joystick_devices[index] = handle;
    hr = ((idirectinputdevice8_enumobjects_proc)(*(void ***)handle)[4])(handle,
        (void *)halo::input::input_enumerate_gamepad_object_callback, (void *)(intptr_t)index, 0);
    if (hr < 0) {
        failed = (char *)"EnumObjects (gamepad)";
        goto fail;
    }
    input_device_count = index + 1;
    return input_device_count != 8;

fail:
    input_error_log_once(hr, failed);
    halo::input::input_device_release((int16_t)index);
    return 1;
}

}

namespace halo::input {

/**
 * Implements input enumerate gamepad object callback.
 *
 * @address 0x491c50
 */
int32_t DirectInput::enumerate_gamepad_object_callback(const di_device_object_instance *object, void *reference)
{
    void *device = joystick_devices[(int32_t)reference];
    uint32_t type = object->type;
    idirectinputdevice8_setproperty_proc set_property;
    di_property_range range;
    di_property_dword deadzone;
    int32_t hr;

    if ((type & 3) == 0 || (uint16_t)(type >> 8) >= 0x20) {
        return 1;
    }
    set_property = (idirectinputdevice8_setproperty_proc)(*(void ***)device)[6];
    range.header.size = sizeof(range);
    range.header.header_size = sizeof(di_property_header);
    range.header.object = type;
    range.header.how = 2;
    range.minimum = -0x1000;
    range.maximum = 0x1000;
    hr = set_property(device, 4, (di_property_dword *)&range);
    if (hr < 0) {
        input_error_log_once(hr, (char *)"InitializeObject range %d - %s", object->type, object->name);
        return 1;
    }
    deadzone.header.size = sizeof(deadzone);
    deadzone.header.header_size = sizeof(di_property_header);
    deadzone.header.object = object->type;
    deadzone.header.how = 2;
    deadzone.data = 1000;
    hr = set_property(device, 5, &deadzone);
    if (hr < 0) {
        input_error_log_once(hr, (char *)"InitializeObject deadzone %d - %s", object->type, object->name);
    }
    return 1;
}

}

extern "C" { extern int32_t input_last_error; }
/**
 * Formats description (printf-style, with any varargs) into a scratch buffer, but only the
 * first time error_code is seen; repeats of the same HRESULT-like code are suppressed. Every
 * call site in this build passes a literal description with no varargs, so the formatted text
 * is only ever the description itself, but the varargs plumbing is preserved.
 *
 * @address 0x492150
 */
extern "C" void input_error_log_once(int32_t error_code, char *description, ...)
{
    char message[4092];
    va_list args;

    if (error_code != input_last_error) {
        input_last_error = error_code;
        va_start(args, description);
        vsprintf(message, description, args);
        va_end(args);
    }
}

extern "C" { extern key_block_timer key_block_timers[k_input_key_block_timer_count]; }
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
uint8_t DirectInput::get_key_state(int16_t key_index)
{
    int32_t i;

    if (input_suppressed != 0) {
        return 0;
    }

    switch (key_index - _input_key_any_shift) {
    case 0:
        if (key_frames[_input_key_right_shift] < key_frames[_input_key_left_shift]) {
            return key_frames[_input_key_left_shift];
        }
        return key_frames[_input_key_right_shift];

    case 1:
        if (key_frames[_input_key_right_control] < key_frames[_input_key_left_control]) {
            return key_frames[_input_key_left_control];
        }
        return key_frames[_input_key_right_control];

    case 2:
        if (key_frames[_input_key_left_windows] <= key_frames[_input_key_right_windows]) {
            return key_frames[_input_key_right_windows];
        }
        return key_frames[_input_key_left_windows];

    case 3:
        if (key_frames[_input_key_left_alt] <= key_frames[_input_key_right_alt]) {
            return key_frames[_input_key_right_alt];
        }
        return key_frames[_input_key_left_alt];

    default:
        if (key_index != -1) {
            for (i = 0; i < k_input_key_block_timer_count; i++) {
                if (key_block_timers[i].key == key_index) {
                    return 0;
                }
            }
        }
        return key_frames[key_index];
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
uint8_t DirectInput::get_mouse_button_state(int16_t button_index)
{
    uint8_t result;

    result = 0;
    if (mouse_device != 0 && input_suppressed == 0) {
        result = live_mouse_state.button_frames[button_index];
    }
    return result;
}

}

namespace halo::input {

/**
 * Widens ansi (up to 0x26 characters; longer strings are truncated to 0x26, matching the
 * original bounds check) into a stack buffer and parses it with CLSIDFromString. Returns
 * nonzero on success.
 *
 * Original register convention: ansi string in ESI.
 *
 * @address 0x491670
 */
uint8_t DirectInput::guid_parse_ansi(input_guid *out_guid, char *ansi)
{
    int32_t length;
    int32_t i;
    uint16_t wide[40];
    int32_t hresult;

    length = 0;
    while (ansi[length] != '\0') {
        length = length + 1;
    }
    if (0x4e < (uint32_t)(length * 2 + 2)) {
        length = 0x26;
    }
    if ((uint32_t)(length * 2 + 2) < 0x4f) {
        wide[length] = 0;
        for (i = length - 1; i >= 0; i--) {
            wide[i] = (uint8_t)ansi[i];
        }
    }
    hresult = CLSIDFromString((LPCOLESTR)wide, (LPCLSID)out_guid);
    return hresult >= 0;
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
void DirectInput::joystick_state_process(joystick_raw_state *raw, joystick_state *dest, input_device *device)
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
        } else if (angle < 0x8ca) {
            octant = 0;
        } else if (angle < 0x1a5e) {
            octant = 1;
        } else if (angle < 0x2bf2) {
            octant = 2;
        } else if (angle < 0x3d86) {
            octant = 3;
        } else if (angle < 0x4f1a) {
            octant = 4;
        } else if (angle < 0x60ae) {
            octant = 5;
        } else if (angle < 0x7242) {
            octant = 6;
        } else if (angle < 0x83d6) {
            octant = 7;
        } else {
            octant = 0;
        }
        dest->povs[i] = octant;
    }

    for (i = 0; i < device->axis_count; i++) {
        dest->axes[i] = (int16_t)raw->axes[i];
    }
}

}

extern "C" { extern int16_t system_keys[k_input_system_key_count]; }
extern "C" { extern int64_t performance_frequency; }
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
void DirectInput::key_block_timer_set(int16_t key, int32_t duration_ms)
{
    key_block_timer *chosen;
    int32_t i;
    large_integer counter;
    uint32_t now;

    chosen = (key_block_timer *)0;
    for (i = 0; i < k_input_key_block_timer_count; i++) {
        if (key_block_timers[i].key == -1) {
            chosen = &key_block_timers[i];
            break;
        }
        if (chosen == (key_block_timer *)0 || key_block_timers[i].deadline < chosen->deadline) {
            chosen = &key_block_timers[i];
        }
    }

    if (chosen != (key_block_timer *)0) {
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
        chosen->deadline = now + duration_ms;
        chosen->key = key;

        for (i = 0; i < k_input_system_key_count; i++) {
            if (system_keys[i] == key) {
                input_globals.system_key_states[i] = 0;
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
void DirectInput::key_block_timers_expire(void)
{
    int32_t i;
    large_integer counter;
    uint32_t now;

    for (i = 0; i < k_input_key_block_timer_count; i++) {
        if (key_block_timers[i].deadline != 0xffffffff) {
            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            now = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
            if (key_block_timers[i].deadline <= now) {
                key_block_timers[i].deadline = 0xffffffff;
                key_block_timers[i].key = -1;
            }
        }
    }
}

}

extern "C" { extern ui_key_event key_events[k_input_key_event_capacity]; }
extern "C" { extern input_guid guid_sys_keyboard; }
extern "C" { extern di_data_format c_dfDIKeyboard; }
extern "C" { extern void *shell_window; }
namespace halo::input {

/**
 * Resets the keyboard runtime state and key block timers, then creates and configures the
 * DirectInput keyboard device (cooperative level, key data format, and a 32-entry buffered
 * input property), releasing it again on any failure. Always returns 1.
 *
 * @address 0x4918a0
 */
uint8_t DirectInput::keyboard_device_create(void)
{
    void **vtable;
    int32_t hr;
    char *description;
    di_property_dword buffer_size;
    int32_t i;

    memset(key_frames, 0, sizeof(key_frames));
    key_event_read_index = 0;
    key_event_count = 0;
    memset(key_events, 0, sizeof(ui_key_event) * 0x10);
    memset(key_release_pending, 0, sizeof(key_release_pending));

    for (i = 0; i < k_input_key_block_timer_count; i++) {
        key_block_timers[i].deadline = 0xffffffff;
        key_block_timers[i].key = -1;
    }

    vtable = *(void ***)direct_input;
    hr = ((idirectinput8_createdevice_proc)vtable[3])(direct_input, &guid_sys_keyboard,
        &keyboard_device, (void *)0);
    if (hr < 0) {
        description = (char *)"CreateDevice (keyboard)";
    } else {
        vtable = *(void ***)keyboard_device;
        hr = ((idirectinputdevice8_setcooplevel_proc)vtable[13])(keyboard_device, shell_window, 0x16);
        if (hr < 0) {
            description = (char *)"SetCooperativeLevel (keyboard)";
        } else {
            vtable = *(void ***)keyboard_device;
            hr = ((idirectinputdevice8_setdataformat_proc)vtable[11])(keyboard_device, &c_dfDIKeyboard);
            if (hr < 0) {
                description = (char *)"SetDataFormat (keyboard)";
            } else {
                buffer_size.header.size = 0x14;
                buffer_size.header.header_size = 0x10;
                buffer_size.header.object = 0;
                buffer_size.header.how = 0;
                buffer_size.data = 0x20;

                vtable = *(void ***)keyboard_device;
                hr = ((idirectinputdevice8_setproperty_proc)vtable[6])(keyboard_device, 1, &buffer_size);
                if (hr >= 0) {
                    return 1;
                }
                description = (char *)"SetProperty (keyboard)";
            }
        }
    }

    input_error_log_once(hr, description);
    if (keyboard_device != (void *)0) {
        vtable = *(void ***)keyboard_device;
        ((idirectinputdevice8_unacquire_proc)vtable[8])(keyboard_device);
        ((idirectinputdevice8_release_proc)vtable[2])(keyboard_device);
        keyboard_device = (void *)0;
    }
    return 1;
}

}

namespace halo::input {

/**
 * Switches the keyboard device between normal input mode and rebind-capture mode, and clears
 * the per-key press/hold state arrays whenever the keyboard device is present.
 *
 * @address 0x48b650
 */
void DirectInput::keyboard_set_capture_mode(uint8_t enable_capture)
{
    if (enable_capture == 0) {
        input_globals.mode_flags = input_globals.mode_flags & ~_input_mode_keyboard_capture_bit;
    } else {
        input_globals.mode_flags = input_globals.mode_flags | _input_mode_keyboard_capture_bit;
    }

    if (keyboard_device != 0) {
        uint32_t flush_all = 0xffffffff;
        void **vtable = *(void ***)keyboard_device;
        ((idirectinputdevice8_getdevicedata_proc)vtable[0x28 / 4])(keyboard_device,
            sizeof(di_device_object_data), (di_device_object_data *)0, &flush_all, 0);
        memset(key_release_pending, 0, sizeof(key_release_pending));
        memset(key_frames, 0, sizeof(key_frames));
    }
}

}

extern "C" { extern int16_t mouse_button_map[k_input_mouse_button_count]; }
extern "C" { extern input_guid guid_sys_mouse; }
extern "C" { extern di_data_format c_dfDIMouse2; }
namespace halo::input {

/**
 * Seeds mouse_button_map[0..1] from the system's left/right swap setting, then creates and
 * configures the DirectInput mouse device (cooperative level, DIMOUSESTATE2 data format),
 * releasing it again on any failure. Always returns 1.
 *
 * @address 0x4919c0
 */
uint8_t DirectInput::mouse_device_create(void)
{
    void **vtable;
    int32_t hr;
    char *description;

    if (GetSystemMetrics(0x17) == 0) {
        mouse_button_map[0] = 0;
        mouse_button_map[1] = 2;
    } else {
        mouse_button_map[0] = 2;
        mouse_button_map[1] = 0;
    }

    vtable = *(void ***)direct_input;
    hr = ((idirectinput8_createdevice_proc)vtable[3])(direct_input, &guid_sys_mouse,
        &mouse_device, (void *)0);
    if (hr < 0) {
        description = (char *)"CreateDevice (mouse)";
    } else {
        vtable = *(void ***)mouse_device;
        hr = ((idirectinputdevice8_setcooplevel_proc)vtable[13])(mouse_device, shell_window, 5);
        if (hr < 0) {
            description = (char *)"SetCooperativeLevel (mouse)";
        } else {
            vtable = *(void ***)mouse_device;
            hr = ((idirectinputdevice8_setdataformat_proc)vtable[11])(mouse_device, &c_dfDIMouse2);
            if (hr >= 0) {
                return 1;
            }
            description = (char *)"SetDataFormat (mouse)";
        }
    }

    input_error_log_once(hr, description);
    if (mouse_device != (void *)0) {
        vtable = *(void ***)mouse_device;
        ((idirectinputdevice8_unacquire_proc)vtable[8])(mouse_device);
        ((idirectinputdevice8_release_proc)vtable[2])(mouse_device);
        mouse_device = (void *)0;
    }
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
void DirectInput::mouse_state_process(mouse_state *dest, di_mouse_state2 *raw)
{
    int32_t i;
    int32_t mapped_slot;
    uint8_t pressed_now;
    uint8_t old_frames;

    dest->x = raw->x;
    dest->y = -raw->y;
    if (mouse_wheel_granularity != 0) {
        dest->wheel = -(raw->z / mouse_wheel_granularity);
    }

    for (i = 0; i < k_input_mouse_button_count; i++) {
        mapped_slot = mouse_button_map[i];
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

extern "C" { extern int16_t virtual_key_to_key[0x100]; }
extern "C" { extern int16_t character_to_key[0x80]; }
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
void DirectInput::record_windows_key_message(uint32_t wparam, int32_t message)
{
    ui_key_event event;

    if (input_acquired == 0) {
        return;
    }

    if (message == 0x100 || message == 0x104) {
        event.key_code = virtual_key_to_key[wparam];
        if (event.key_code == -1) {
            return;
        }
        event.character = 0xff;
    } else if (message == 0x102 || message == 0x106) {
        event.character = (uint8_t)wparam;
        event.key_code = -1;
        if (wparam < 0x80) {
            event.key_code = character_to_key[wparam];
        }
        if (event.character == 0xff) {
            return;
        }
    } else {
        return;
    }

    event.modifiers = 0;
    if (((uint16_t)GetKeyState(0x10) >> 15) != 0) {
        event.modifiers = event.modifiers | _input_modifier_shift_bit;
    }
    if (GetKeyState(0x11) < 0) {
        event.modifiers = event.modifiers | _input_modifier_control_bit;
    }
    if (GetKeyState(0x12) < 0) {
        event.modifiers = event.modifiers | _input_modifier_alt_bit;
    }

    if (key_event_count < k_input_key_event_capacity) {
        key_events[key_event_count] = event;
        key_event_count = key_event_count + 1;
    }
}

}
