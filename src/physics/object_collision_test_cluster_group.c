// object_collision_test_cluster_group  (Ghidra: FUN_00505490, still unnamed there; phase-2
// guessed object_test_collision_group)
// address 0x505490, size 172 bytes
// name confidence: 0.3   rewrite confidence: 0.2 -- among the lowest-confidence files in this
//   batch; see the UNSURE paragraphs below.
// evidence: out/phase4/physics_functions.md ("Walks the group of objects associated with a
//   cluster/parent to test whether any of them collides with a given point."); src/objects/
//   object_resolve_collideable_reference.c's own object_cluster_reference struct (identifier
//   0x00, object_index 0x04, next_reference 0x08, stride 0x0c, backed by
//   collideable_object_references at 0x008603d4) matches this function's own raw offsets (+4,
//   +8) exactly for the manual chain-walk after the first resolve call; ScenarioStructureBSP
//   .leaves (+0xe4, stride 0x10, cluster at +0x08, per types/physics.h) matches the leaf lookup
//   feeding that first call; object_collision_test_nearby_chain (0x505350, this batch) is the
//   callee the resolved objects are handed to.
// register convention: param_1 is Ghidra's own recognized stack parameter (flags), doing double
//   duty as both the bit-5..7 gate tested here AND the type_mask forwarded to
//   object_collision_test_nearby_chain unchanged. param_2 is Ghidra-recognized but never read
//   anywhere in this function's own body -- UNSURE whether it is genuinely unused or whether
//   Ghidra simply lost every read of it.
//   // blam-cc: EDI -> position, stack -> flags, exclude_object_index
// FIXED (register inputs, objdump): EDI carries position (read at 0x5054aa, mov edx,edi, then
// forwarded to bsp3d_node_find_leaf and pushed for object_collision_test_nearby_chain). The old
// notes guessed it was an untraceable hidden register; it is a plain EDI live-in. The other
// former "hidden register", exclude_object_index, really is a stack argument -- the second one
// ([esp+0x10] at entry, loaded into ebp) -- so it moves to stack -> alongside flags.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "physics.h"

extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98
extern ScenarioStructureBSP *global_structure_bsp;        // 0x00746f9c
extern data_array *collideable_object_references;           // 0x008603d4, objects module

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
                                      real_point3d *point); // 0x5013a0, this module
extern datum_index object_resolve_collideable_reference(datum_index *next_reference,
                                                          int16_t cluster_index); // 0x4f5f00, objects module
extern uint8_t object_collision_test_nearby_chain(uint32_t start_object_index, uint32_t type_mask,
    real_point3d *position, uint32_t exclude_object_index); // 0x505350, this batch

// When flags selects one of the cluster-relative contact tests (bits 5-7), resolves position's
// containing leaf and, for the "test the leaf's cluster object group" variant (bit 7), walks
// every object referenced by that cluster via object_collision_test_nearby_chain. Returns 1
// immediately if position falls outside the BSP entirely (nothing to test against) or if any
// referenced object reports a hit.
// blam-cc: EDI -> position, stack -> flags, exclude_object_index
uint8_t object_collision_test_cluster_group(uint32_t flags, real_point3d *position,
                                             uint32_t exclude_object_index)
{
    if ((flags & 0xe0) != 0) {
        int32_t leaf_index = bsp3d_node_find_leaf(0, global_structure_collision_bsp, position);

        if (leaf_index == -1) {
            return 1;
        }
        if ((flags >> 7 & 1) != 0) {
            ScenarioStructureBSPLeaf *leaves =
                (ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer;
            datum_index next_reference;
            datum_index object_index =
                object_resolve_collideable_reference(&next_reference, leaves[leaf_index].cluster);

            while (object_index != k_datum_index_none) {
                if (object_collision_test_nearby_chain(object_index, flags, position,
                                                         exclude_object_index)) {
                    return 1;
                }
                if (next_reference == k_datum_index_none) {
                    object_index = k_datum_index_none;
                } else {
                    object_cluster_reference *ref =
                        (object_cluster_reference *)collideable_object_references->data +
                        (next_reference & 0xffff);
                    object_index = ref->object_index;
                    next_reference = ref->next_reference;
                }
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x505490):

undefined4 FUN_00505490(uint param_1,undefined4 param_2)

{
  uint uVar1;
  uint uVar2;
  char cVar3;
  int iVar4;
  uint uVar5;

  uVar2 = param_1;
  if ((param_1 & 0xe0) != 0) {
    iVar4 = FUN_005013a0();
    if (iVar4 == -1) {
      return 1;
    }
    if ((uVar2 >> 7 & 1) != 0) {
      iVar4 = object_resolve_collideable_reference
                        (&param_1,*(undefined2 *)(iVar4 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4)))
      ;
      uVar1 = param_1;
      while (iVar4 != -1) {
        cVar3 = FUN_00505350(iVar4,uVar2);
        if (cVar3 != '\0') {
          return 1;
        }
        if (uVar1 == 0xffffffff) {
          iVar4 = -1;
          uVar1 = 0xffffffff;
        }
        else {
          uVar5 = uVar1 & 0xffff;
          uVar1 = *(uint *)(*(int *)(DAT_008603d4 + 0x34) + 8 + uVar5 * 0xc);
          iVar4 = *(int *)(*(int *)(DAT_008603d4 + 0x34) + uVar5 * 0xc + 4);
        }
      }
    }
  }
  return 0;
}
#endif
