// object_recalculate_bounding_radius_recursive  (Ghidra: object_recalculate_bounding_radius_recursive, already named)
// address 0x4f82b0, size 96 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Recomputes the bounding radius of an object and all of its
//   attached children")
// rewrite confidence: 0.7
// evidence: types/objects.h object (first_child_object 0x118, next_object 0x114); global
//   0x008603b0 object_data; callee object_recalculate_bounding_radius (0x4f8310, this batch).
// register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
//   "object_recalculate_bounding_radius_recursive(uint param_1)").

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void object_recalculate_bounding_radius(uint32_t object_index); // 0x4f8310, this batch

void object_recalculate_bounding_radius_recursive(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    datum_index child;

    object_recalculate_bounding_radius(object_index);

    child = obj->first_child_object;
    while (child != k_datum_index_none) {
        object *child_obj = ((object_header *)object_data->data)[child & 0xffff].data;
        object_recalculate_bounding_radius_recursive(child);
        child = child_obj->next_object;
    }
}

#if 0
Original Ghidra decompilation (0x4f82b0):

void object_recalculate_bounding_radius_recursive(uint param_1)

{
  int iVar1;
  uint uVar2;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  object_recalculate_bounding_radius(param_1);
  uVar2 = *(uint *)(iVar1 + 0x118);
  while (uVar2 != 0xffffffff) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
    object_recalculate_bounding_radius_recursive(uVar2);
    uVar2 = *(uint *)(iVar1 + 0x114);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
