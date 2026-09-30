// input_enumerate_gamepad_callback  (not a Ghidra function; the DirectInput EnumDevices callback)
// address 0x491d70, size 514 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 (retail-independence loop) from the disassembly 0x491d70..0x491f72. It had no C because it is
// reached only as an immediate: input_system_initialize passes it to IDirectInput8::EnumDevices
// (DI8DEVCLASS_GAMECTRL, DIEDFL_ATTACHEDONLY), which the C did as the literal retail address 0x491d70 -- a jump into
// original code the standalone cannot run. For each attached game controller (up to 8, the count at 0x6b1844):
// CreateDevice into the device slot joystick_devices[n] (0x6b1848), SetCooperativeLevel(GetActiveWindow(), 5),
// SetDataFormat(the joystick format 0x68e51c), GetCapabilities; the input_device record (0x6b1868 + n * 0x240)
// takes the product instance number (input_device_count_by_guid of the product guid), both guids, the instance name
// widened (0x20a bytes) with a " (N)" suffix for the 2nd.. device of a product, slot -1 and the axis / button / POV
// counts (clamped 0x20 / 0x20 / 0x10); then EnumObjects(input_enumerate_gamepad_object_callback, n, DIDFT_ALL) sets
// each axis' range and deadzone. Success counts the device and continues while fewer than 8 are known; a failing
// step logs (input_error_log_once), releases the half-made device and continues.
// blam-cc: __stdcall (a DirectInput callback: instance, reference)

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "fn_input.h"
#include <string.h>
#include <wchar.h>

extern void *direct_input;                    // 0x006b15fc, IDirectInput8A*
extern int32_t input_device_count;            // 0x006b1844
extern void *joystick_devices[8];             // 0x006b1848, IDirectInputDevice8A*
extern input_device input_devices[8];         // 0x006b1868
extern di_data_format joystick_data_format;   // 0x0068e51c


extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source); // 0x557990, EAX, EDI, EBX
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, EDX, stack


typedef int32_t (__stdcall *idirectinputdevice8_getcapabilities_proc)(void *self, di_device_caps *caps);
typedef int32_t (__stdcall *idirectinputdevice8_enumobjects_proc)(void *self, void *callback, void *reference,
    uint32_t flags);

int32_t __stdcall input_enumerate_gamepad_callback(const di_device_instance *instance, void *reference)
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
        failed = "CreateDevice (gamepad)";
        goto fail;
    }
    vtable = *(void ***)handle;
    hr = ((idirectinputdevice8_setcooplevel_proc)vtable[13])(handle, GetActiveWindow(), 5);
    if (hr < 0) {
        failed = "SetCooperativeLevel (gamepad)";
        goto fail;
    }
    hr = ((idirectinputdevice8_setdataformat_proc)(*(void ***)handle)[11])(handle, &joystick_data_format);
    if (hr < 0) {
        failed = "SetDataFormat (gamepad)";
        goto fail;
    }
    memset(&caps, 0, sizeof(caps));
    caps.size = sizeof(caps);
    hr = ((idirectinputdevice8_getcapabilities_proc)(*(void ***)handle)[3])(handle, &caps);
    if (hr < 0) {
        failed = "GetCapabilities (gamepad)";
        goto fail;
    }
    device->record.product_instance = (uint8_t)input_device_count_by_guid((const uint32_t *)&instance->product_guid);
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
        (void *)input_enumerate_gamepad_object_callback, (void *)(intptr_t)index, 0);
    if (hr < 0) {
        failed = "EnumObjects (gamepad)";
        goto fail;
    }
    input_device_count = index + 1;
    return input_device_count != 8;

fail:
    input_error_log_once(hr, failed);
    input_device_release((int16_t)index);
    return 1;
}
