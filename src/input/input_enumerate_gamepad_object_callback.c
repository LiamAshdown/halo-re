// input_enumerate_gamepad_object_callback  (not a Ghidra function; the DirectInput EnumObjects callback)
// address 0x491c50, size 213 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 (retail-independence loop) from the disassembly 0x491c50..0x491d25. Reached only as an
// immediate: input_enumerate_gamepad_callback (0x491d70) passes it to IDirectInputDevice8::EnumObjects with the
// device index as the reference. For every axis object (type bits 0..1 set) whose instance number (type >> 8, as a
// word) is below 0x20, the device joystick_devices[index] gets DIPROP_RANGE -0x1000..0x1000 and then
// DIPROP_DEADZONE 1000, both by object id (DIPH_BYID); a failure is logged with the object's type and name and
// stops that object. Always continues the enumeration (1).
// blam-cc: __stdcall (a DirectInput callback: object instance, reference)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "fn_input.h"

extern void *joystick_devices[8];             // 0x006b1848, IDirectInputDevice8A*


int32_t __stdcall input_enumerate_gamepad_object_callback(const di_device_object_instance *object, void *reference)
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
        input_error_log_once(hr, "InitializeObject range %d - %s", object->type, object->name);
        return 1;
    }
    deadzone.header.size = sizeof(deadzone);
    deadzone.header.header_size = sizeof(di_property_header);
    deadzone.header.object = object->type;
    deadzone.header.how = 2;
    deadzone.data = 1000;
    hr = set_property(device, 5, &deadzone);
    if (hr < 0) {
        input_error_log_once(hr, "InitializeObject deadzone %d - %s", object->type, object->name);
    }
    return 1;
}
