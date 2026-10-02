// light_cluster_iterate_begin  (not a Ghidra function; a light callback for the visible-object collection)
// address 0x4f34c0, size 57 bytes
// name confidence: 0.7  rewrite confidence: 0.9
// evidence: object_lights_update_all 0x4f0bd0 hands 0x4f34c0 to structure_bsp_collect_visible_objects 0x554420 as
//   its iterate-begin callback (0x4f0e3c..0x4f0e50 push the five). Only reachable as that pointer; first-boot track:
//   the first rendered frame needs it.
// objdump 0x4f34c0..0x4f34f8: the cluster's first reference (light_cluster_first[cluster]); -1 there gives -1
//   (the cursor holds -1). Otherwise the reference element (0xc bytes) gives the light (+4) and the cursor becomes
//   the next reference (+8).
// blam-cc: stack -> cursor, cluster_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index *light_cluster_first; // 0x00860b20
extern data_array *light_cluster_references; // 0x00860b24

datum_index light_cluster_iterate_begin(datum_index *cursor, int16_t cluster_index)
{
    datum_index reference = light_cluster_first[cluster_index];
    uint8_t *element;

    *cursor = reference;
    if (reference == k_datum_index_none) {
        return k_datum_index_none;
    }
    element = (uint8_t *)light_cluster_references->data + (reference & 0xffff) * 0xc;
    *cursor = *(datum_index *)(element + 8);
    return *(datum_index *)(element + 4);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
