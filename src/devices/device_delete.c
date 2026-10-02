// device_delete  (not a Ghidra function; the device type's +0x30 (delete) callback)
// address 0x44b6b0, size 104 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: object_type_definition device (0x0069bbf8) field +0x30 (delete); the object type dispatch (object_type_definitions_*)
//   calls it cdecl with the object handle. Only reachable through that table (never a Ghidra function);
//   first-boot track: needed while placing the UI map's objects.
//   objdump 0x44b6b0..0x44b717: each of the object's two device groups (+0x1f8, +0x204) that exists and is
//   flagged for deletion with its object (group element byte +2 bit 2) is deleted from device_groups; the
//   handle passed is the int16 index sign-extended, as the original does (movsx edx,cx).
// blam-cc: stack -> object_index (cdecl)

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


extern data_array *device_groups; // 0x0087abf0
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX -> array, EDX -> handle

void device_delete(datum_index object_index)
{
    uint8_t *obj = object_get(object_index);
    int16_t group = ((device_object *)obj)->device.power_group;

    if (group != -1 && (((uint8_t *)device_groups->data)[(uint16_t)group * 8 + 2] & 4) != 0) {
        datum_delete(device_groups, (datum_index)(int32_t)group);
    }
    group = ((device_object *)obj)->device.position_group;
    if (group != -1 && (((uint8_t *)device_groups->data)[(uint16_t)group * 8 + 2] & 4) != 0) {
        datum_delete(device_groups, (datum_index)(int32_t)group);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
