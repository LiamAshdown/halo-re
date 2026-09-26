// object_cluster_stamp_not_visited  (not a Ghidra function; an object visit-stamp callback)
// address 0x4f96f0, size 39 bytes
// name confidence: 0.75  rewrite confidence: 0.95
// evidence: render_objects_collect 0x50eac0 hands 0x4f96f0 to structure_bsp_collect_visible_objects 0x554420 as its predicate
//   callback. Only reachable as that pointer (never a Ghidra function); first-boot track: the first frame needs it.
// objdump 0x4f96f0: true while the object's cluster stamp (+0x14) differs from object_cluster_stamp.
// blam-cc: stack -> object_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"

extern data_array *object_data; // 0x008603b0
extern int32_t object_cluster_stamp; // 0x008603cc

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}

uint8_t object_cluster_stamp_not_visited(datum_index object_index)
{
    return *(int32_t *)(object_get(object_index) + 0x14) != object_cluster_stamp;
}
