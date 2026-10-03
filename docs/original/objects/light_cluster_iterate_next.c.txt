// light_cluster_iterate_next  (not a Ghidra function; a light callback for the visible-object collection)
// address 0x4f3500, size 44 bytes
// name confidence: 0.7  rewrite confidence: 0.9
// evidence: object_lights_update_all 0x4f0bd0 hands 0x4f3500 to structure_bsp_collect_visible_objects 0x554420 as
//   its iterate-next callback (0x4f0e3c..0x4f0e50 push the five). Only reachable as that pointer; first-boot track:
//   the first rendered frame needs it.
// objdump 0x4f3500..0x4f352b: -1 once the cursor is -1; otherwise the light of the cursor's reference (+4), and
//   the cursor moves to the next reference (+8).
// blam-cc: stack -> cursor (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index *light_cluster_first; // 0x00860b20
extern data_array *light_cluster_references; // 0x00860b24

datum_index light_cluster_iterate_next(datum_index *cursor)
{
    uint8_t *element;

    if (*cursor == k_datum_index_none) {
        return k_datum_index_none;
    }
    element = (uint8_t *)light_cluster_references->data + (*cursor & 0xffff) * 0xc;
    *cursor = *(datum_index *)(element + 8);
    return *(datum_index *)(element + 4);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
