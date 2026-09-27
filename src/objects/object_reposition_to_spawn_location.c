// object_reposition_to_spawn_location  (Ghidra: FUN_004f7b70; renamed, Blam-style, not
// previously named)
// address 0x4f7b70, size 195 bytes
// name confidence: 0.3 (matches functions.md's summary: "Optionally repositions an object to a
//   stored/spawn location and refreshes its cluster and bounding radius")
// rewrite confidence: 0.3 (collision_test_movement_segment's exact argument semantics could not be pinned down by
//   hand-tracing the disassembly in the time available; its outputs are preserved as a raw
//   scratch buffer rather than named fields -- see UNSURE below)
// evidence: types/objects.h object (position 0x05c, location_cluster_index 0x09c); global
//   0x008603b0 object_data; callees object_unlink_cluster_or_notify_parent (0x4f5de0,
//   established: EAX -> object_index), object_set_cluster_and_parent (0x4f5c30, established:
//   EAX -> object_index, ECX -> location), object_recalculate_bounding_radius (0x4f8310,
//   established).
// register convention: object index is a stack parameter, a target-position pointer is in ECX.
//   Confirmed against objdump -d -M intel bin/halo.exe: 0x4f7b9c fsub [ecx] uses ECX before any
//   load of it in the visible window, and 0x4f7b7b mov ebp,[esp+0x70] reads the object index
//   from the stack.
//   // blam-cc: ECX -> target_position, stack -> object_index
// UNSURE: collision_test_movement_segment (0x505880, foreign module) is called with a literal selector
//   (0x1000e9), the target_position pointer, the position delta, and a fifth pointer this
//   rewrite treats as a 24-byte scratch/output buffer whose first three dwords become the
//   {leaf, cluster, pad} location object_set_cluster_and_parent is later handed; the exact
//   field boundaries inside that buffer are not resolved.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data; // 0x008603b0

extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30
extern void object_recalculate_bounding_radius(uint32_t object_index); // 0x4f8310
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, collision_result *result); // 0x505880
    // No prototype is asserted: Ghidra models fewer or differently-typed arguments here than
    // the other call site(s) of the same address, because the missing operands travel in
    // registers it could not source. The empty parameter list is the convention this module
    // already uses for FUN_00450870 -- one declaration per symbol, no invented signature.

// REWRITTEN from objdump 0x4f7b70..0x4f7c32: the draft dropped the stack object to ignore, so the collision
//   sweep got the result buffer as its exclude object and wrote through garbage; the result fields were misread.
//   Sweeps from target_position back to the object (mask 0x1000e9, ignoring `ignore`); an unobstructed object
//   already in a cluster stays; otherwise it moves to the hit point in the hit leaf (0 when that leaf has no
//   cluster).
// blam-cc: ECX -> target_position, stack -> object_index, ignore_object_index
uint8_t object_reposition_to_spawn_location(uint32_t object_index, real_point3d *target_position,
                                            uint32_t ignore_object_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    real_vector3d delta;
    collision_result hit;

    delta.i = *(float *)(obj + 0x5c) - target_position->x;
    delta.j = *(float *)(obj + 0x60) - target_position->y;
    delta.k = *(float *)(obj + 0x64) - target_position->z;
    if (!collision_test_movement_segment(0x1000e9, target_position, &delta, ignore_object_index, &hit) &&
        *(int16_t *)(obj + 0x9c) != -1) {
        return 1;
    }
    if (hit.leaf.cluster_index == -1) {
        return 0;
    }
    object_unlink_cluster_or_notify_parent(object_index);
    obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    *(real_point3d *)(obj + 0x5c) = hit.point;
    object_set_cluster_and_parent(object_index, &hit.leaf);
    object_recalculate_bounding_radius(object_index);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4f7b70):

undefined4 FUN_004f7b70(uint param_1)

{
  int iVar1;
  int iVar2;
  char cVar3;
  int iVar4;
  undefined1 local_48 [4];
  short local_44;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;

  iVar2 = DAT_008603b0;
  iVar4 = (param_1 & 0xffff) * 0xc;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar4);
  cVar3 = FUN_00505880(0x1000e9);
  if ((cVar3 != '\0') || (*(short *)(iVar1 + 0x9c) == -1)) {
    if (local_44 == -1) {
      return 0;
    }
    iVar1 = *(int *)(*(int *)(iVar2 + 0x34) + 8 + iVar4);
    FUN_004f5de0();
    *(undefined4 *)(iVar1 + 0x5c) = local_3c;
    *(undefined4 *)(iVar1 + 0x60) = local_38;
    *(undefined4 *)(iVar1 + 100) = local_34;
    FUN_004f5c30(param_1,local_48);
    object_recalculate_bounding_radius(param_1);
  }
  return 1;
}
#endif
