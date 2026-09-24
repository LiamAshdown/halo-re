// object_set_position_and_recalculate
// address 0x4f52c0, size 139 bytes
// name confidence: 0.3 (still FUN_004f52c0 in Ghidra; functions.md calls this a "velocity"
//   setter, but the offsets written -- object+0x5c/0x60/0x64 -- are the documented position
//   real_point3d, not velocity at 0x068; renamed accordingly)
// rewrite confidence: 0.45
// evidence: types/objects.h object (position at 0x05c); callees FUN_004f5c30 (0x4f5c30, this
//   batch, cluster relink), FUN_004f5de0 (0x4f5de0, this batch, cluster unlink),
//   object_recalculate_bounding_radius (0x4f8310).
// register convention: position vector pointer in ESI (unaff_ESI), object index in EDI
//   (unaff_EDI).
// UNSURE: bsp3d_node_find_leaf (also seen in objects_update_player_visibility_masks) is called first
//   with no visible arguments and its result is discarded; preserved for its side effect only,
//   whatever that is.
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

extern int32_t bsp3d_node_find_leaf(void *globals, real_point3d *point, int32_t index); // 0x5013a0, UNSURE: unexamined; result discarded here
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0, this batch
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30, this batch; NULL location probes it
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern void object_recalculate_bounding_radius(uint32_t object_index); // 0x4f8310

void object_set_position_and_recalculate(real_point3d *position, uint32_t object_index)
    // blam-cc: ESI -> position, EDI -> object_index
{
    object *obj;

    bsp3d_node_find_leaf(global_collision_bsp, position, 0); // 0x4f52c3 mov ecx,ds:0x746f90 / mov edx,esi / xor eax,eax
    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_unlink_cluster_or_notify_parent(object_index);
    *(real_point3d *)((uint8_t *)obj + 0x5c) = *position;
    object_set_cluster_and_parent(object_index, 0); // UNSURE: flag argument not visible here
    object_recalculate_bounding_radius(object_index);
}

#if 0
Original Ghidra decompilation (0x4f52c0):

void FUN_004f52c0(void)

{
  int iVar1;
  undefined4 *unaff_ESI;
  uint unaff_EDI;

  FUN_005013a0();
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
  FUN_004f5de0();
  *(undefined4 *)(iVar1 + 0x5c) = *unaff_ESI;
  *(undefined4 *)(iVar1 + 0x60) = unaff_ESI[1];
  *(undefined4 *)(iVar1 + 100) = unaff_ESI[2];
  FUN_004f5c30();
  object_recalculate_bounding_radius();
  return;
}
#endif
