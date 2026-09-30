// object_collision_context_test_pill  (Ghidra: FUN_005050b0, still unnamed there; phase-2
// guessed object_nodes_test_bsp_shape, and out/phase4/physics_functions.md's own summary calls
// it a "scaled segment query" -- both wrong: its only BSP-query callee is 0x00502730
// (collision_bsp_query_pill_init, this module), the swept-sphere/capsule query, not the segment
// one at 0x502060 that 0x00504f60 (this batch) actually uses. This rewrite follows the callee,
// not the auto-generated summary.)
// address 0x5050b0, size 321 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: same object_node_collision_result field map as 0x00504f60's own evidence note;
//   collision_bsp_query_pill_init's established 6-argument signature (this module), whose
//   (origin, delta, radius, max_fraction) stack arguments match this function's visible call
//   site in order and count; this function's own signature is FULLY Ghidra-recognized
//   (`FUN_005050b0(int param_1, undefined4 param_2, undefined4 param_3, float param_4,
//   undefined2 *param_5)`), which is what makes it the more reliable twin to reconstruct
//   0x00504f60's own partly-hidden parameters from.
// register convention: all five parameters are Ghidra's own recognized parameters (context,
//   origin, delta, radius_scale, out_result); origin (param_2) and delta (param_3) are UNSURE
//   as real_point3d*/real_vector3d* -- Ghidra shows zero reads of them anywhere in this
//   function's own body (loaded once, forwarded untouched into matrix4x3_transform_point /
//   matrix4x3_transform_vector), so their types are inferred from usage at the call site in
//   unit_find_placement_position.c (a candidate position and a direction) rather than read here.
//   // blam-cc: stack -> context, origin, delta, radius_scale, out_result
// UNSURE: collision_bsp_query_pill_init.c's own Ghidra decompile is genuinely `void` (it calls
//   collision_bsp_query_pill_node_recursive and returns without forwarding EAX in the C source),
//   but this call site reads its result as a boolean anyway (`cVar3 = FUN_00502730(...); if
//   (cVar3 != '\0')`) -- which is possible, and preserved here, because
//   collision_bsp_query_pill_node_recursive's own boolean return is still sitting in EAX when
//   collision_bsp_query_pill_init falls through to its own return with no intervening call. The
//   extern below is declared to return that value; collision_bsp_query_pill_init.c's own `void`
//   signature does not actually contradict this, it simply never named the value.
// UNSURE: flags (EAX) / result (ECX) for collision_bsp_query_pill_init are invisible at this
// call site, exactly as in 0x00504f60; result is confidently &out_result->segment (reusing the
// collision_bsp_pill_result-shaped view of the same embedded record, since 0x00505055b0 reads
// this same out_result back as a collision_bsp_segment_result -- see UNSURE below).
// UNSURE (major): object_node_collision_result's embedded record is typed
// collision_bsp_segment_result (0x418) in types/physics.h, but collision_bsp_pill_result is a
// different, incompatible 0x420-byte layout (t/plane/surface_index/material_index/leaf_count/
// leaves, no plane_index/surface_flags/breakable_surface_index split the way the segment result
// has them). This rewrite writes through a local collision_bsp_pill_result and copies only the
// fields object_node_collision_result actually has room to hold (t and leaf bookkeeping are
// compatible at the front of both layouts; anything pill-specific past that does not fit and is
// dropped), since object_node_collision_result's own struct comment (types/physics.h) documents
// it as being built for the segment variant only. Confidence on this function is accordingly low.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#include "fn_math.h"
#include "fn_physics.h"


extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point,
                                       real_matrix4x3 *m); // 0x4cbde0
extern void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v,
                                        real_matrix4x3 *m); // 0x4cbe50

    // 0x502730, this module; see UNSURE header note on its void-vs-bool return

// Tests a world-space swept sphere (origin, delta, radius * radius_scale) against every
// collision node of context's object, one node at a time, the pill counterpart of
// object_collision_context_test_segment. Keeps scanning every node, narrowing the shared
// fraction on each hit so later nodes only need to beat whatever is left of the sweep; returns
// whether any node was hit at all.
// blam-cc: stack -> context, origin, delta, radius_scale, out_result
uint8_t object_collision_context_test_pill(object_collision_context *context, real_point3d *origin,
                                            real_vector3d *delta, float radius_scale,
                                            object_node_collision_result *out_result)
{
    ModelCollisionGeometry *definition = context->definition;
    ModelCollisionGeometryNode *nodes = (ModelCollisionGeometryNode *)definition->nodes.pointer;
    int32_t node_index;
    uint8_t hit = 0;

    out_result->segment.t = 3.4028235e+38f; // FLT_MAX

    for (node_index = 0; node_index < (int32_t)definition->nodes.count; node_index++) {
        ModelCollisionGeometryNode *node = &nodes[node_index];

        if (node->region != 0xffff) {
            uint8_t permutation_byte = context->region_permutations[node->region];

            if (permutation_byte != 0xff && (int32_t)node->bsps.count > 0) {
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
                    collision_bsp_pill_result pill_result;

                    matrix4x3_inverse(&inverse_matrix, &((real_matrix4x3 *)context->nodes)[node_index]);
                    matrix4x3_transform_point(&local_origin, origin, &inverse_matrix);
                    matrix4x3_transform_vector(&local_delta, delta, &inverse_matrix);

                    if (collision_bsp_query_pill_init(bsp, &pill_result, &local_origin, &local_delta,
                                                       inverse_matrix.scale * radius_scale,
                                                       out_result->segment.t)) {
                        out_result->node_index = (int16_t)node_index;
                        out_result->region_index = (int16_t)node->region;
                        out_result->permutation_index = (int16_t)permutation;
                        out_result->segment.t = pill_result.t;
                        hit = 1;
                    }
                }
            }
        }
    }
    return hit;
}

#if 0
Original Ghidra decompilation (0x5050b0):

/* WARNING: Removing unreachable block (ram,0x00505131) */

uint FUN_005050b0(int param_1,undefined4 param_2,undefined4 param_3,float param_4,
                 undefined2 *param_5)

{
  byte bVar1;
  short sVar2;
  char cVar3;
  ushort uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  float local_5c [14];
  undefined1 local_24 [12];
  undefined1 local_18 [12];
  int local_c;
  undefined1 local_5;

  *(undefined4 *)(param_5 + 4) = 0x7f7fffff;
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
        if (0 < *(int *)((short)uVar4 * 0x60 + *(int *)(iVar6 + 0x38))) {
          matrix4x3_inverse();
          matrix4x3_transform_point(local_5c);
          matrix4x3_transform_vector(local_5c);
          cVar3 = FUN_00502730(local_24,local_18,local_5c[0] * param_4,*(undefined4 *)(param_5 + 4))
          ;
          if (cVar3 != '\0') {
            *param_5 = (undefined2)local_c;
            param_5[1] = *(undefined2 *)(iVar6 + 0x20);
            param_5[2] = uVar4;
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
