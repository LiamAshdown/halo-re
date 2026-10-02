// object_collision_context_test_segment  (Ghidra: FUN_00504f60, still unnamed there; phase-2
// guessed object_nodes_test_bsp_pill, and out/phase4/physics_functions.md's own summary calls
// it a "capsule/pill query" -- both wrong: its only BSP-query callee is 0x00502060
// (collision_bsp_query_segment_init, this module), not 0x00502730 (the pill init at 0x5050b0,
// which passes a radius argument this function never computes). This rewrite follows the
// callee, not the auto-generated summary.)
// address 0x504f60, size 314 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/physics.h object_node_collision_result (node_index 0x00, region_index 0x02,
//   permutation_index 0x04, embedded collision_bsp_segment_result at 0x08 -- the
//   "in_stack_00000014" stores in Ghidra's decompile land on exactly these int16 offsets);
//   collision_bsp_query_segment_init's own established 8-argument signature (this module),
//   whose (bsp, breakable_surface_count=0, breakable_surfaces=NULL, origin, delta, max_fraction)
//   stack arguments match this function's visible call site in order and count; the sibling
//   0x005050b0 (pill variant, same skeleton) whose fully Ghidra-recognized 5-parameter
//   signature (context, origin, delta, scale, out_result) is the basis for reconstructing this
//   function's own 4 parameters, which Ghidra only partly recognized (param_1 plus a lone
//   "in_stack_00000014" for the last one).
// register convention: param_1 is Ghidra's own recognized stack parameter (context). origin and
//   delta are UNSURE -- Ghidra shows zero reads of them anywhere in this function's own body
//   (they are loaded once at entry and forwarded untouched into matrix4x3_transform_point /
//   matrix4x3_transform_vector), so their presence and stack position are reconstructed from
//   0x005050b0's fully-recognized twin rather than read directly here.
//   // blam-cc: stack -> context, flags, origin, delta, out_result
// UNSURE: the flags (EAX) and result (ECX) register arguments collision_bsp_query_segment_init
// takes are invisible at this call site too; result is confidently &out_result->segment (the
// pre-seeded FLT_MAX flows through exactly that field), but flags is guessed as 0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point,
                                       real_matrix4x3 *m); // 0x4cbde0
extern void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v,
                                        real_matrix4x3 *m); // 0x4cbe50
extern uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result,
                                                 ModelCollisionGeometryBSP *bsp,
                                                 int16_t breakable_surface_count,
                                                 uint32_t *breakable_surfaces, real_point3d *origin,
                                                 real_vector3d *delta,
                                                 float max_fraction); // 0x502060, this module

// Tests a world-space segment (origin, delta) against every collision node of context's object,
// one node at a time: inverts that node's local-to-world matrix, transforms the segment into
// the node's local space, and runs it through the node's currently active BSP permutation via
// collision_bsp_query_segment_init. Keeps scanning every node (never stops early), narrowing
// out_result->segment.t on each hit so later nodes are only tested against whatever is left of
// the segment; returns whether any node was hit at all.
// FIXED (objdump 0x504f60, in game): FIVE stack arguments -- the second ([ebp+0xc]) is the query flags, handed to
//   collision_bsp_query_segment_init in EAX (callers pass 3 or their own type mask; they clean 0x14 bytes). The
//   draft had four, so origin/delta/out_result were read one slot early and the flags were a constant 0.
// blam-cc: stack -> context, flags, origin, delta, out_result
uint8_t object_collision_context_test_segment(object_collision_context *context, uint32_t flags,
                                               real_point3d *origin, real_vector3d *delta,
                                               object_node_collision_result *out_result)
{
    ModelCollisionGeometry *definition = (ModelCollisionGeometry *)context->definition;
    ModelCollisionGeometryNode *nodes = (ModelCollisionGeometryNode *)definition->nodes.pointer;
    int32_t node_index;
    uint8_t hit = 0;

    out_result->segment.t = 3.4028235e+38f; // FLT_MAX

    for (node_index = 0; node_index < (int32_t)definition->nodes.count; node_index++) {
        ModelCollisionGeometryNode *node = &nodes[node_index];

        if (node->region != 0xffff) {
            uint8_t permutation_byte = context->region_permutations[node->region];

            // FIXED (0x504fc7): the original zero-extends the byte and compares it with 0xffff, which never matches,
            // so a permutation byte of 0xff is NOT a skip -- it is clamped to the last BSP below. The draft skipped
            // those regions (in game: bullets passed through parts of NPCs, no blood or hit effects).
            if ((int32_t)node->bsps.count > 0) {
                int32_t permutation = permutation_byte;
                ModelCollisionGeometryBSP *bsps = (ModelCollisionGeometryBSP *)node->bsps.pointer;
                ModelCollisionGeometryBSP *bsp;

                if (permutation > (int32_t)node->bsps.count - 1) {
                    permutation = (int32_t)node->bsps.count - 1;
                }
                bsp = &bsps[permutation];

                if ((int32_t)bsp->bsp3d_nodes.count > 0) {
                    real_matrix4x3 inverse_matrix;
                    real_point3d local_origin;
                    real_vector3d local_delta;

                    matrix4x3_inverse(&inverse_matrix, &((real_matrix4x3 *)context->nodes)[node_index]);
                    matrix4x3_transform_point(&local_origin, origin, &inverse_matrix);
                    matrix4x3_transform_vector(&local_delta, delta, &inverse_matrix);

                    if (collision_bsp_query_segment_init(flags, &out_result->segment, bsp, 0, 0,
                                                          &local_origin, &local_delta,
                                                          out_result->segment.t)) {
                        out_result->node_index = (int16_t)node_index;
                        out_result->region_index = (int16_t)node->region;
                        out_result->permutation_index = (int16_t)permutation;
                        hit = 1;
                    }
                }
            }
        }
    }
    return hit;
}

#if 0
Original Ghidra decompilation (0x504f60):

/* WARNING: Removing unreachable block (ram,0x00504fe1) */

uint FUN_00504f60(int param_1)

{
  byte bVar1;
  short sVar2;
  char cVar3;
  ushort uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int *piVar8;
  undefined2 *in_stack_00000014;
  undefined1 local_5c [56];
  undefined1 local_24 [12];
  undefined1 local_18 [12];
  int local_c;
  undefined1 local_5;

  *(undefined4 *)(in_stack_00000014 + 4) = 0x7f7fffff;
  uVar5 = *(uint *)(param_1 + 4);
  local_5 = 0;
  local_c = 0;
  if (0 < *(int *)(uVar5 + 0x28c)) {
    iVar6 = 0;
    do {
      sVar2 = *(short *)(iVar6 * 0x40 + 0x20 + *(int *)(uVar5 + 0x290));
      iVar6 = iVar6 * 0x40 + *(int *)(uVar5 + 0x290);
      if (((sVar2 != -1) && (bVar1 = *(byte *)(*(int *)(param_1 + 8) + (int)sVar2), bVar1 != 0xffff)
          ) && (0 < *(int *)(iVar6 + 0x34))) {
        uVar4 = (ushort)bVar1;
        iVar7 = *(int *)(iVar6 + 0x34) + -1;
        if (iVar7 < (short)uVar4) {
          uVar4 = (ushort)iVar7;
        }
        piVar8 = (int *)((short)uVar4 * 0x60 + *(int *)(iVar6 + 0x38));
        if (0 < *piVar8) {
          matrix4x3_inverse();
          matrix4x3_transform_point(local_5c);
          matrix4x3_transform_vector(local_5c);
          cVar3 = FUN_00502060(piVar8,0,0,local_24,local_18,*(undefined4 *)(in_stack_00000014 + 4));
          if (cVar3 != '\0') {
            *in_stack_00000014 = (undefined2)local_c;
            in_stack_00000014[1] = *(undefined2 *)(iVar6 + 0x20);
            in_stack_00000014[2] = uVar4;
            local_5 = 1;
          }
        }
      }
      uVar5 = *(uint *)(param_1 + 4);
      local_c = local_c + 1;
      iVar6 = (int)(short)local_c;
    } while (iVar6 < *(int *)(uVar5 + 0x28c));
    return CONCAT31((int3)(uVar5 >> 8),local_5);
  }
  return uVar5 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
