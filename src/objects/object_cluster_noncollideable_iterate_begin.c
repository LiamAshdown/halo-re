// object_cluster_noncollideable_iterate_begin  (not a Ghidra function; an object cluster iterator)
// address 0x4f5e90, size 57 bytes
// name confidence: 0.75  rewrite confidence: 0.95
// evidence: render_objects_collect 0x50eac0 hands 0x4f5e90 to structure_bsp_collect_visible_objects 0x554420 as its iterate-begin
//   callback. Only reachable as that pointer (never a Ghidra function); first-boot track: the first frame needs it.
// objdump 0x4f5e90: the cluster's first noncollideable reference (noncollideable_cluster_first[cluster]) -- -1 gives -1 -- then the reference element
//   (0xc bytes) gives the object (+4) and the cursor becomes the next reference (+8).
// blam-cc: stack -> cursor, cluster_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern datum_index *noncollideable_cluster_first; // 0x008603c0
extern data_array *noncollideable_object_references; // 0x008603c4

datum_index object_cluster_noncollideable_iterate_begin(datum_index *cursor, int16_t cluster_index)
{
    datum_index reference = noncollideable_cluster_first[cluster_index];
    uint8_t *element;

    *cursor = reference;
    if (reference == k_datum_index_none) {
        return k_datum_index_none;
    }
    element = (uint8_t *)noncollideable_object_references->data + (reference & 0xffff) * 0xc;
    *cursor = *(datum_index *)(element + 8);
    return *(datum_index *)(element + 4);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
