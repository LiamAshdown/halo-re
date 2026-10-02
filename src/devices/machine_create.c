// machine_create  (not a Ghidra function; the machine type's +0x28 (create) callback)
// address 0x44b020, size 120 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: object_type_definition machine (0x0069bcc0) field +0x28 (create); the object type dispatch (object_type_definitions_*)
//   calls it cdecl with the object handle. Only reachable through that table (never a Ghidra function);
//   first-boot track: needed while placing the UI map's objects.
//   objdump 0x44b020..0x44b097: flag 0x2000 is set; flags 0x4000 and 0x8000 follow the machine tag's flag
//   byte +0x292 bit 2 (set when it is set, cleared otherwise). The result is 1.
// blam-cc: stack -> object_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}

static uint8_t *object_definition(uint8_t *object)
{
    return (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
}

uint8_t machine_create(datum_index object_index)
{
    uint8_t *object = object_get(object_index);
    uint8_t *definition = object_definition(object);
    uint32_t *flags = (uint32_t *)(object + 0x10);

    *flags |= 0x2000;
    if ((definition[0x292] & 4) != 0) {
        *flags |= 0x4000 | 0x8000;
    } else {
        *flags &= ~(uint32_t)(0x4000 | 0x8000);
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
