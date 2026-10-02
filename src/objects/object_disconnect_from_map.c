// object_disconnect_from_map  (Ghidra: object_disconnect_from_map, already named)
// address 0x4f96f0, size 39 bytes
// name confidence: 0.6 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Tests whether an object's attachment was not visited during the
//   current map traversal (i.e. is effectively disconnected)")
// rewrite confidence: 0.95 (objdump-verified; also known as object_cluster_stamp_not_visited, the visit-stamp predicate render_objects_collect hands to structure_bsp_collect_visible_objects)
// evidence: types/objects.h object (cluster_stamp 0x014); global 0x008603b0 object_data,
//   global 0x008603cc object_cluster_stamp.
// register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
//   "object_disconnect_from_map(uint param_1)").
// UNSURE: the return value packs the bool in the low byte with object_cluster_stamp's upper 3
//   bytes as garbage (Ghidra's CONCAT31); narrowed to a plain bool here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern int32_t object_cluster_stamp; // 0x008603cc

uint8_t object_disconnect_from_map(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    return obj->cluster_stamp != object_cluster_stamp;
}

#if 0
Original Ghidra decompilation (0x4f96f0):

undefined4 object_disconnect_from_map(uint param_1)

{
  return CONCAT31((int3)((uint)DAT_008603cc >> 8),
                  *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) +
                          0x14) != DAT_008603cc);
}
#endif
