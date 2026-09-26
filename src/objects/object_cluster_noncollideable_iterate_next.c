// object_cluster_noncollideable_iterate_next  (not a Ghidra function; an object cluster iterator)
// address 0x4f5ed0, size 44 bytes
// name confidence: 0.75  rewrite confidence: 0.95
// evidence: render_objects_collect 0x50eac0 hands 0x4f5ed0 to structure_bsp_collect_visible_objects 0x554420 as its iterate-next
//   callback. Only reachable as that pointer (never a Ghidra function); first-boot track: the first frame needs it.
// objdump 0x4f5ed0: -1 once the cursor is -1; otherwise the object of the cursor's reference (+4), and the cursor
//   moves to the next reference (+8).
// blam-cc: stack -> cursor (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"

extern datum_index *noncollideable_cluster_first; // 0x008603c0
extern data_array *noncollideable_object_references; // 0x008603c4

datum_index object_cluster_noncollideable_iterate_next(datum_index *cursor)
{
    uint8_t *element;

    if (*cursor == k_datum_index_none) {
        return k_datum_index_none;
    }
    element = (uint8_t *)noncollideable_object_references->data + (*cursor & 0xffff) * 0xc;
    *cursor = *(datum_index *)(element + 8);
    return *(datum_index *)(element + 4);
}
