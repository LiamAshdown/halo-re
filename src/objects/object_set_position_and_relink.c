// object_set_position_and_relink
// address 0x4f5350, size 66 bytes
// name confidence: 0.3 (still FUN_004f5350 in Ghidra; functions.md calls this a lighter
//   "velocity" setter, but the offset written -- object+0x5c/0x60/0x64 -- is the documented
//   position real_point3d, not velocity at 0x068; renamed accordingly)
// rewrite confidence: 0.5
// evidence: types/objects.h object (position at 0x05c); callees FUN_004f5de0 (0x4f5de0, this
//   batch, cluster unlink) and FUN_004f5c30 (0x4f5c30, this batch, cluster relink) -- the
//   minimal variant of object_set_position_and_recalculate, with no leaf probe and no bounding
//   radius recalculation.
// register convention: position vector pointer in ESI (unaff_ESI), object index in EDI
//   (unaff_EDI).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0, this batch
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30, this batch; NULL location probes it

void object_set_position_and_relink(real_point3d *position, uint32_t object_index)
    // blam-cc: ESI -> position, EDI -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    object_unlink_cluster_or_notify_parent(object_index);
    *(real_point3d *)((uint8_t *)obj + 0x5c) = *position;
    object_set_cluster_and_parent(object_index, 0); // UNSURE: flag argument not visible here
}

#if 0
Original Ghidra decompilation (0x4f5350):

void FUN_004f5350(void)

{
  int iVar1;
  undefined4 *unaff_ESI;
  uint unaff_EDI;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
  FUN_004f5de0();
  *(undefined4 *)(iVar1 + 0x5c) = *unaff_ESI;
  *(undefined4 *)(iVar1 + 0x60) = unaff_ESI[1];
  *(undefined4 *)(iVar1 + 100) = unaff_ESI[2];
  FUN_004f5c30();
  return;
}
#endif
