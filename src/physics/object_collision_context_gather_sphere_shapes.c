// object_collision_context_gather_sphere_shapes  (Ghidra: FUN_00505200, still unnamed there;
// phase-2 guessed object_nodes_apply_bsp_contact)
// address 0x505200, size 296 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (step 1: body checked against objdump -d 0x505200..0x505327) (raised from 0.2 by the phase-4
//   integration pass, which resolved FUN_00503d90's param_1 from this very call site)
//   -- still among the lower-confidence files in this
//   batch; see the UNSURE paragraphs below, which mirror physics_shape_build_proxies_from_query's
//   own already-low (0.15) confidence for the exact same callee.
// evidence: out/phase4/physics_functions.md ("Iterates an object's collision nodes, running a
//   sphere collision query at each and building local physics-model contact proxies for any
//   node that touches geometry."); same per-node loop skeleton as 0x00504f60/0x005050b0 (this
//   batch); collision_bsp_query_sphere_init's established signature (this module), whose
//   (breakable_surfaces=NULL, center, radius) stack arguments match this call site in order and
//   count; the caller in collision_gather_nearby_object_shapes.c
//   (`FUN_00505200(node_context, origin, radius, x_offset, y_offset, model)`), which is the
//   basis for this function's own 6-parameter signature (its own Ghidra decompile recognizes
//   all six, though 2 and 4/5 are typed undefined4/float rather than real_point3d*/float).
// register convention: all six parameters are Ghidra's own recognized parameters (context,
//   origin, radius_scale, margin, thickness, model). origin is never read directly in this
//   function's own body (forwarded untouched into matrix4x3_transform_point), so its type is
//   inferred from the sibling call site rather than read here.
//   // blam-cc: stack -> context, origin, radius_scale, margin, thickness, model
// UNSURE: this file names param_4/param_5 margin/thickness (matching
//   physics_shape_build_proxies_from_query's own parameter names, which they are forwarded to
//   unmodified) rather than collision_gather_nearby_object_shapes.c's x_offset/y_offset guess
//   for the same call-site position -- the two sibling files disagree on this pair's role.
// UNSURE (partly resolved by the phase-4 integration pass): the call to FUN_00503d90 shows all
// five of its stack arguments here -- `FUN_00503d90((short)uVar7 * 0x60 + iVar6, param_4, param_5,
// *param_1, param_6)` -- and that first one is the selected BSP permutation record, which is what
// fixed physics_shape_build_proxies_from_query's own param_1 as its bsp rather than a material
// type. result (EDI), matrix and material_type remain invisible at this call site: matrix is
// reconstructed as this loop's own just-computed inverse_matrix (the natural "moving reference
// frame" for this node), and material_type is passed as -1 (unresolved), matching this module's
// convention elsewhere.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point,
                                       real_matrix4x3 *m); // 0x4cbde0
extern uint32_t collision_bsp_query_sphere_init(ModelCollisionGeometryBSP *bsp,
                                                 int16_t breakable_surface_count,
                                                 collision_bsp_sphere_result *result,
                                                 uint32_t *breakable_surfaces, real_point3d *center,
                                                 float radius); // 0x501980, this module
extern void physics_shape_build_proxies_from_query(collision_bsp_sphere_result *result,
    real_matrix4x3 *matrix, ModelCollisionGeometryBSP *bsp, float margin, float thickness,
    int32_t object_index, physics_model *model); // 0x503d90, blam-cc: EDI result, EAX matrix

// Tests a world-space sphere (origin, radius_scale) against every collision node of context's
// object, one node at a time, building physics_model proxies (via
// physics_shape_build_proxies_from_query) for every node whose active BSP permutation the
// sphere actually touches. Unlike the segment/pill node tests this never narrows a shared
// fraction -- every touched node contributes its own proxies to model.
// blam-cc: stack -> context, origin, radius_scale, margin, thickness, model
uint8_t object_collision_context_gather_sphere_shapes(object_collision_context *context,
                                                        real_point3d *origin, float radius_scale,
                                                        float margin, float thickness,
                                                        physics_model *model)
{
    ModelCollisionGeometry *definition = context->definition;
    ModelCollisionGeometryNode *nodes = (ModelCollisionGeometryNode *)definition->nodes.pointer;
    int32_t node_index;
    uint8_t hit = 0;

    for (node_index = 0; node_index < (int32_t)definition->nodes.count; node_index++) {
        ModelCollisionGeometryNode *node = &nodes[node_index];

        if (node->region != 0xffff) {
            // FIXED (objdump 0x505251..0x50525d): the region index is sign-extended, and the byte is
            // zero-extended into DX before `cmp dx,0xffff`, which can never match -- so 0xff is not a
            // "no permutation" marker here; it is clamped to the last BSP like any other value
            uint8_t permutation_byte = context->region_permutations[(int16_t)node->region];

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
                    real_point3d local_center;
                    collision_bsp_sphere_result sphere_result;

                    matrix4x3_inverse(&inverse_matrix, &((real_matrix4x3 *)context->nodes)[node_index]);
                    matrix4x3_transform_point(&local_center, origin, &inverse_matrix);

                    if (collision_bsp_query_sphere_init(bsp, 0, &sphere_result, 0, &local_center,
                                                         inverse_matrix.scale * radius_scale)) {
                        // FIXED (objdump 0x50529e, 0x5052f0): EAX is EBX = &nodes[node_index], the
                        // node's own matrix -- the query runs in node space and the proxies are
                        // carried back to world space; the draft passed the inverse
                        physics_shape_build_proxies_from_query(&sphere_result,
                            &((real_matrix4x3 *)context->nodes)[node_index], bsp, margin, thickness,
                            context->object_index, model);
                        hit = 1;
                    }
                }
            }
        }
    }
    return hit;
}

#if 0
Original Ghidra decompilation (0x505200):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Removing unreachable block (ram,0x00505273) */

undefined4
FUN_00505200(undefined4 *param_1,undefined4 param_2,float param_3,undefined4 param_4,
            undefined4 param_5,undefined4 param_6)

{
  byte bVar1;
  short sVar2;
  char cVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  ushort uVar7;
  float local_54 [18];
  int local_c;
  undefined3 uStack_8;
  undefined1 local_5;

  iVar6 = param_1[1];
  uVar5 = 0x1000;
  uStack_8 = 0x50520d;
  local_5 = 0;
  local_c = 0;
  if (0 < *(int *)(iVar6 + 0x28c)) {
    iVar4 = 0;
    do {
      sVar2 = *(short *)(iVar4 * 0x40 + 0x20 + *(int *)(iVar6 + 0x290));
      iVar6 = iVar4 * 0x40 + *(int *)(iVar6 + 0x290);
      if (((sVar2 != -1) && (bVar1 = *(byte *)(param_1[2] + (int)sVar2), bVar1 != 0xffff)) &&
         (iVar4 = *(int *)(iVar6 + 0x34), 0 < iVar4)) {
        uVar7 = (ushort)bVar1;
        iVar4 = iVar4 + -1;
        if (iVar4 < (short)uVar7) {
          uVar7 = (ushort)iVar4;
        }
        iVar6 = *(int *)(iVar6 + 0x38);
        if (0 < *(int *)((short)uVar7 * 0x60 + iVar6)) {
          matrix4x3_inverse();
          uVar5 = matrix4x3_transform_point(local_54);
          cVar3 = FUN_00501980(0,uVar5,local_54[0] * param_3);
          if (cVar3 != '\0') {
            FUN_00503d90((short)uVar7 * 0x60 + iVar6,param_4,param_5,*param_1,param_6);
            local_5 = 1;
          }
        }
      }
      iVar6 = param_1[1];
      local_c = local_c + 1;
      iVar4 = (int)(short)local_c;
    } while (iVar4 < *(int *)(iVar6 + 0x28c));
    uVar5 = CONCAT31((int3)(char)((uint)local_c >> 8),local_5);
  }
  return uVar5;
}
#endif
