// object_collision_context_test_point  (Ghidra: FUN_00504e90, still unnamed there; phase-2
// guessed object_nodes_test_bsp_leaf)
// address 0x504e90, size 202 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/physics_functions.md ("Checks whether all of an object's collision nodes
//   resolve to a valid BSP leaf, i.e. are not embedded outside the world's collision geometry.");
//   types/physics.h object_collision_context (object_index, definition, region_permutations,
//   nodes -- every field this function reads through unaff_EBX matches exactly); types/tags.h
//   ModelCollisionGeometry.nodes (+0x28c count / +0x290 pointer), ModelCollisionGeometryNode
//   (region +0x20, bsps +0x34 count / +0x38 pointer, stride 0x40), ModelCollisionGeometryBSP
//   (bsp3d_nodes.count at +0x00, stride 0x60); the sibling call chain FUN_00505350 (this batch)
//   which treats a nonzero return here as "this object occupies the query point" -- consistent
//   with -1 being the bsp3d_node_find_leaf "solid, no leaf" sentinel rather than the ordinary
//   negative-encoded-leaf terminal value.
// register convention: unaff_EBX -> context (object_collision_context *), unaff_ESI -> point
//   (real_point3d *, the query point; never read directly in this function's own body -- it
//   only survives, untouched, into the matrix4x3_inverse_transform_point call at the bottom of
//   the loop, which is why Ghidra shows no "in_ESI" here at all). No stack parameters.
//   // blam-cc: EBX -> context, stack -> point (0x504f1b `mov esi,[esp+0x24]`)
// UNSURE: the local_point scratch matrix4x3_inverse_transform_point/bsp3d_node_find_leaf share
//   is likewise invisible in Ghidra's own decompile (no local ever named for it); reconstructed
//   from the identical local-buffer pattern the three sibling per-node-loop functions in this
//   batch (0x504f60, 0x5050b0, 0x505200) declare explicitly for the same matrix4x3_inverse_
//   transform_point call.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#include "fn_math.h"
#include "fn_physics.h"


extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
                                      real_point3d *point); // 0x5013a0, this module

// Walks every collision node of context's object, transforms point into each node's currently
// active BSP permutation's local space, and reports whether ANY node resolves to no leaf at
// all (the bsp3d_node_find_leaf "solid" sentinel, as opposed to an ordinary leaf) -- meaning
// the point is embedded in that node's collision geometry. Nodes whose region has no active
// permutation, or whose active BSP is empty, are skipped.
// blam-cc: EBX -> context, stack -> point (0x504f1b `mov esi,[esp+0x24]`)
uint32_t object_collision_context_test_point(object_collision_context *context, real_point3d *point)
{
    ModelCollisionGeometry *definition = context->definition;
    ModelCollisionGeometryNode *nodes = (ModelCollisionGeometryNode *)definition->nodes.pointer;
    int32_t node_index;

    for (node_index = 0; node_index < (int32_t)definition->nodes.count; node_index++) {
        ModelCollisionGeometryNode *node = &nodes[node_index];

        if (node->region != 0xffff) {
            uint8_t permutation = context->region_permutations[node->region];

            // 0x504edb..0x504ee4: `movzx dx,byte / cmp dx,0xffff` can never match, so a 0xff permutation is NOT
            // skipped -- it is clamped to the last bsp like any other out-of-range index. (The draft skipped it.)
            if (node->bsps.count > 0) {
                int32_t bsp_index = permutation;
                ModelCollisionGeometryBSP *bsps = (ModelCollisionGeometryBSP *)node->bsps.pointer;
                ModelCollisionGeometryBSP *bsp;

                if (bsp_index > (int32_t)node->bsps.count - 1) {
                    bsp_index = (int32_t)node->bsps.count - 1;
                }
                bsp = &bsps[bsp_index];

                if ((int32_t)bsp->bsp3d_nodes.count > 0) {
                    real_point3d local_point;

                    matrix4x3_inverse_transform_point(&((real_matrix4x3 *)context->nodes)[node_index],
                                                       &local_point, point);
                    if (bsp3d_node_find_leaf(0, bsp, &local_point) == 0xffffffff) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x504e90):

/* WARNING: Removing unreachable block (ram,0x00504ef2) */

uint FUN_00504e90(void)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  ushort uVar6;
  int unaff_EBX;
  short sVar7;

  uVar4 = *(uint *)(unaff_EBX + 4);
  iVar2 = *(int *)(uVar4 + 0x28c);
  sVar7 = 0;
  if (0 < iVar2) {
    iVar3 = *(int *)(uVar4 + 0x290);
    iVar5 = 0;
    do {
      uVar4 = iVar5 * 0x40 + iVar3;
      if (((*(short *)(uVar4 + 0x20) != -1) &&
          (bVar1 = *(byte *)(*(int *)(unaff_EBX + 8) + (int)*(short *)(uVar4 + 0x20)),
          bVar1 != 0xffff)) && (0 < *(int *)(uVar4 + 0x34))) {
        uVar6 = (ushort)bVar1;
        iVar5 = *(int *)(uVar4 + 0x34) + -1;
        if (iVar5 < (short)uVar6) {
          uVar6 = (ushort)iVar5;
        }
        uVar4 = *(uint *)((short)uVar6 * 0x60 + *(int *)(uVar4 + 0x38));
        if (0 < (int)uVar4) {
          matrix4x3_inverse_transform_point();
          uVar4 = FUN_005013a0();
          if (uVar4 == 0xffffffff) {
            return 0xffffff01;
          }
        }
      }
      sVar7 = sVar7 + 1;
      iVar5 = (int)sVar7;
    } while (iVar5 < iVar2);
  }
  return uVar4 & 0xffffff00;
}
#endif
