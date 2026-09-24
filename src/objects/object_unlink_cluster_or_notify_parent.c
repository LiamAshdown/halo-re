// object_unlink_cluster_or_notify_parent
// address 0x4f5de0, size 172 bytes
// name confidence: 0.5 (still FUN_004f5de0 in Ghidra; functions.md's summary matches: "Removes
//   an object from its current cluster/placement bookkeeping, or notifies its parent object
//   when it is owned by one")
// rewrite confidence: 0.45
// evidence: types/objects.h object_header (flags at 0x02, in_pvs_pass bit); object
//   (parent_object 0x11c, flags 0x10 with _object_needs_cluster_update_bit,
//   _object_at_rest_bit); global 0x008603b0 object_data; callee object_try_and_get 0x4f6ec0.
// register convention: object index in EAX (in_EAX).
// UNSURE: cluster_reference_remove_all and object_remove_from_sibling_list (the latter inside this module but outside this batch's
//   address range) are called with no visible arguments; by the pattern used everywhere else in
//   this batch they most likely receive the object index, but that is not proven here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

extern datum_index *noncollideable_cluster_first; // 0x008603c0
extern void cluster_reference_remove_all(uint32_t handle, datum_index *link, void *cluster_list); // 0x552020, UNSURE: argument inferred
extern void object_remove_from_sibling_list(uint32_t object_index); // 0x4f8fe0, UNSURE: argument inferred
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
    // 0x4f6ec0; object handle in ECX, type mask on the stack. Verified against the body at
    // 0x4f6ec0 (cmp ecx,-1 / test cx,cx / and param_1 & 1 << header->type) and against the
    // call site in this file.

void object_unlink_cluster_or_notify_parent(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;

    if (obj->parent_object == k_datum_index_none) {
        cluster_reference_remove_all(object_index, (datum_index *)((uint8_t *)obj + 0x10c),
                     &noncollideable_cluster_first);
        // 0x4f5e3c mov eax,0x8603c0 / add ebx,0x10c / push ebx / push edi // UNSURE: see file header
        if ((header->flags & _object_header_in_pvs_pass_bit) != 0) {
            header = (object_header *)object_data->data + (object_index & 0xffff);
            if ((header->flags & _object_header_active_bit) != 0) {
                header->flags &= (uint8_t)~_object_header_active_bit;
            }
        }
    } else {
        if (object_try_and_get(obj->parent_object, _object_mask_all) != 0) {
            // 0x4f5e01 mov ecx,[ebx+0x11c] -- the parent handle just tested above
            object_remove_from_sibling_list(object_index); // UNSURE: see file header
        }
    }

    obj->flags &= ~(uint32_t)_object_needs_cluster_update_bit;
    header->flags &= (uint8_t)~_object_header_connected_bit;
}

#if 0
Original Ghidra decompilation (0x4f5de0):

void FUN_004f5de0(void)

{
  uint *puVar1;
  int iVar2;
  byte bVar3;
  int iVar4;
  uint in_EAX;
  int iVar5;

  iVar5 = (in_EAX & 0xffff) * 0xc;
  iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar5);
  iVar2 = *(int *)(DAT_008603b0 + 0x34) + iVar5;
  if (*(int *)(iVar4 + 0x11c) == -1) {
    FUN_00552020();
    if ((*(byte *)(iVar2 + 2) & 0x40) != 0) {
      bVar3 = *(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + iVar5);
      if ((bVar3 & 1) != 0) {
        *(byte *)(*(int *)(DAT_008603b0 + 0x34) + iVar5 + 2) = bVar3 & 0xfe;
      }
    }
  }
  else {
    iVar5 = object_try_and_get(0xffffffff);
    if (iVar5 != 0) {
      FUN_004f8fe0();
    }
  }
  puVar1 = (uint *)(iVar4 + 0x10);
  *puVar1 = *puVar1 & 0xfffff7ff;
  *(byte *)(iVar2 + 2) = *(byte *)(iVar2 + 2) & 0xdf;
  return;
}
#endif
