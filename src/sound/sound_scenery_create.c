// sound_scenery_create  (not a Ghidra function; the sound_scenery type's +0x28 (create) callback)
// address 0x54e800, size 35 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: object_type_definition sound_scenery (0x0069bb30) field +0x28 (create); the object type dispatch (object_type_definitions_*)
//   calls it cdecl with the object handle. Only reachable through that table (never a Ghidra function);
//   first-boot track: needed while placing the UI map's objects.
//   objdump 0x54e800..0x54e822: sets flag 0x40000; the result is 1.
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


uint8_t sound_scenery_create(datum_index object_index)
{
    *(uint32_t *)(object_get(object_index) + 0x10) |= 0x40000;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
