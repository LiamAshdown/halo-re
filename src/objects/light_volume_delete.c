// light_volume_delete  (not a Ghidra function; an object widget type callback)
// address 0x4fe720, size 20 bytes
// name confidence: 0.6  rewrite confidence: 0.85
// evidence: object widget type table (records of 0x28 from 0x0069c010: fourcc, flag, initialize, dispose,
//   clear_disposing_flag, reset, new, delete, update, render) slot 0x69c0a4 = delete entry of 'mgs2'. Only reachable
//   through that table. First-boot track: placing a campaign level's objects (weapons carry widgets).
// objdump 0x4fe720..0x4fe733: datum_delete(light_volume_instances, index) unless the index is -1.
// blam-cc: stack -> light_volume_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void datum_delete(data_array *array, datum_index index); // 0x4d0510, blam-cc: EAX -> array, EDX -> index
extern data_array *light_volume_instances; // 0x006b8d70

void light_volume_delete(datum_index light_volume_index)
{
    if (light_volume_index != k_datum_index_none) {
        datum_delete(light_volume_instances, light_volume_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
