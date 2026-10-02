// device_create  (not a Ghidra function; the device type's +0x28 (create) callback)
// address 0x44b670, size 52 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: object_type_definition device (0x0069bbf8) field +0x28 (create); the object type dispatch (object_type_definitions_*)
//   calls it cdecl with the object handle. Only reachable through that table (never a Ghidra function);
//   first-boot track: needed while placing the UI map's objects.
//   objdump 0x44b670..0x44b6a3: both device group handles (+0x1f8 power, +0x204 position) start at -1 and flag
//   0x40000 is set; the result is 1.
// blam-cc: stack -> object_index (cdecl); returns AL

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


uint8_t device_create(datum_index object_index)
{
    uint8_t *obj = object_get(object_index);

    ((device_object *)obj)->device.position_group = -1;
    ((device_object *)obj)->device.power_group = -1;
    ((device_object *)obj)->base.flags |= 0x40000;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
