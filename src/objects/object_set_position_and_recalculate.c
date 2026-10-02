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
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0, this batch
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30, this batch; NULL location probes it
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern void object_recalculate_bounding_radius(uint32_t object_index); // 0x4f8310

void object_set_position_and_recalculate(real_point3d *position, uint32_t object_index)
    // blam-cc: ESI -> position, EDI -> object_index
{
    object *obj;
    bsp_leaf_reference location;
    int32_t leaf;

    // VERIFIED against disassembly 0x4f52c0..0x4f534a (2026-09-30): the leaf found for the new position and its
    // cluster (leaf masked with 0x7fffffff, cluster -1 for a -1 leaf) are built in a stack {leaf, cluster} pair
    // that is passed to object_set_cluster_and_parent (0x4f5c30) as its location; the old C discarded it and passed 0.
    leaf = (int32_t)bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)global_collision_bsp, position);
    location.leaf_index = leaf;
    if (leaf == -1) {
        location.cluster_index = -1;
    } else {
        location.cluster_index =
            (int16_t)((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf & 0x7fffffff].cluster;
    }
    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_unlink_cluster_or_notify_parent(object_index);
    *(real_point3d *)&((object *)obj)->position.x = *position;
    object_set_cluster_and_parent(object_index, &location);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
