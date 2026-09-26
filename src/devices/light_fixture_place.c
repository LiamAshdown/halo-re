// light_fixture_place  (not a Ghidra function; the light_fixture type's +0x2c (placed from the scenario) callback)
// address 0x44af30, size 99 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: object_type_definition light_fixture (0x0069be50) field +0x2c (placed from the scenario); the object type dispatch (object_type_definitions_*)
//   calls it cdecl with the object handle and the placement. Only reachable through that table (never a Ghidra function);
//   first-boot track: needed while placing the UI map's objects.
//   objdump 0x44af30..0x44af92: device_new with the placement's device data (+0x28), then the placement's
//   three dwords at +0x30 go to object +0x214 and the three at +0x3c to object +0x220.
// blam-cc: stack -> object_index, placement (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}


extern void device_new(uint32_t object_index, void *placement); // 0x44bf90, blam-cc: EAX -> object_index,
    // EDI -> placement (the device part of the scenario placement, at +0x28)

void light_fixture_place(datum_index object_index, uint8_t *placement)
{
    uint8_t *object = object_get(object_index);
    int32_t i;

    device_new(object_index, placement + 0x28);
    for (i = 0; i < 3; i++) {
        ((uint32_t *)(object + 0x214))[i] = ((uint32_t *)(placement + 0x30))[i];
    }
    for (i = 0; i < 3; i++) {
        ((uint32_t *)(object + 0x220))[i] = ((uint32_t *)(placement + 0x3c))[i];
    }
}
