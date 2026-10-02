// lightning_delete  (not a Ghidra function; an object widget type callback)
// address 0x4fef20, size 20 bytes
// name confidence: 0.6  rewrite confidence: 0.85
// evidence: object widget type table (records of 0x28 from 0x0069c010: fourcc, flag, initialize, dispose,
//   clear_disposing_flag, reset, new, delete, update, render) slot 0x69c0cc = delete entry of 'elec'. Only reachable
//   through that table. First-boot track: placing a campaign level's objects (weapons carry widgets).
// objdump 0x4fef20..0x4fef33: datum_delete(lightning_instances, index) unless the index is -1.
// blam-cc: stack -> lightning_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void datum_delete(data_array *array, datum_index index); // 0x4d0510, blam-cc: EAX -> array, EDX -> index
extern data_array *lightning_instances; // 0x006b8d74

void lightning_delete(datum_index lightning_index)
{
    if (lightning_index != k_datum_index_none) {
        datum_delete(lightning_instances, lightning_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
