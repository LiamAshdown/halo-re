// machine_place  (not a Ghidra function; the machine type's +0x2c (placed from the scenario) callback)
// address 0x44afa0, size 119 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: object_type_definition machine (0x0069bcc0) field +0x2c (placed from the scenario); the object type dispatch (object_type_definitions_*)
//   calls it cdecl with the object handle and the placement. Only reachable through that table (never a Ghidra function);
//   first-boot track: needed while placing the UI map's objects.
//   objdump 0x44afa0..0x44b016: device_new with the placement's device data (+0x28); placement flags (+0x30)
//   bits 0..3 are or-ed into object +0x214.
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

void machine_place(datum_index object_index, uint8_t *placement)
{
    uint8_t *obj = object_get(object_index);

    device_new(object_index, placement + 0x28);
    ((device_object *)obj)->device.type_flags |= placement[0x30] & 0xf;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
