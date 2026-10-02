// control_place  (not a Ghidra function; the control type's +0x2c (placed from the scenario) callback)
// address 0x44ad90, size 91 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: object_type_definition control (0x0069bd88) field +0x2c (placed from the scenario); the object type dispatch (object_type_definitions_*)
//   calls it cdecl with the object handle and the placement. Only reachable through that table (never a Ghidra function);
//   first-boot track: needed while placing the UI map's objects.
//   objdump 0x44ad90..0x44adea: device_new with the placement's device data (+0x28); placement flags (+0x30)
//   bit 0 -> object +0x214 bit 0, bit 4 -> +0x214 bit 1; object +0x218 = placement word +0x34 minus 1.
// blam-cc: stack -> object_index, placement (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "devices.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}


extern void device_new(uint32_t object_index, void *placement); // 0x44bf90, blam-cc: EAX -> object_index,
    // EDI -> placement (the device part of the scenario placement, at +0x28)

void control_place(datum_index object_index, uint8_t *placement)
{
    uint8_t *obj = object_get(object_index);

    device_new(object_index, placement + 0x28);
    if ((placement[0x30] & 1) != 0) {
        ((device_object *)obj)->device.type_flags |= 1;
    }
    if ((placement[0x30] & 0x10) != 0) {
        ((device_object *)obj)->device.type_flags |= 2;
    }
    ((control_object *)obj)->control.custom_name_index =
        (int16_t)(((ScenarioControl *)placement)->custom_control_name - 1);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
