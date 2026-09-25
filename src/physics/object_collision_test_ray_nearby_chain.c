// object_collision_test_ray_nearby_chain  (Ghidra: FUN_005055b0, still unnamed there; phase-2
// guessed object_test_collision_ray_recursive)
// address 0x5055b0, size 714 bytes
// name confidence: 0.4   rewrite confidence: 0.15 -- among the lowest-confidence files in this
//   batch; see the UNSURE paragraphs below. The ray counterpart of
//   object_collision_test_nearby_chain (0x505350, this batch).
// evidence: out/phase4/physics_functions.md ("Recursively casts a ray/segment against a chain of
//   nearby objects, keeping track of the closest valid collision result."); types/objects.h
//   object (flags 0x010, type 0x0b4, bounding_radius 0x0ac, next_object 0x114,
//   first_child_object 0x118 -- the identical chain-walk idiom as 0x505350); types/physics.h
//   object_collision_context ("0x005055b0 reserves 16 bytes for one" -- local_46c's declared
//   size) and object_physics_context ("0x005055b0 reserves exactly 60 bytes for one" --
//   local_45c's declared size), which is what identifies the two branches' buffers; the local
//   chain local_420/41e/41c/418/410/40c/408/407/406 is one contiguous
//   object_node_collision_result (0x420) read field by field (node_index, region_index,
//   permutation_index, then its embedded collision_bsp_segment_result's t/surface_index/
//   plane_index/surface_flags/breakable_surface_index/material_index in order), and
//   local_480/47c/478/474/470 is one contiguous object_physics_ray_result (t, plane_i/j/k/d);
//   types/projectiles.h collision_result's own field offsets (t 0x14, normal 0x24, unknown_30,
//   material_type 0x34, object_index 0x38, unknown_3c, marker_index 0x3e, surface_index 0x44,
//   surface_flags 0x4c, unknown_4d, unknown_4e) match every store into param_7 here exactly.
// register convention: all seven parameters are Ghidra's own recognized parameters
//   (start_object_index, type_mask, origin, delta, radius_scale, exclude_object_index,
//   out_result). Every callee in this function is invoked with far fewer visible arguments than
//   it needs (0x00504e10, 0x00504f60, 0x005074b0, antenna_test_ray_against_vertex_spheres,
//   matrix4x3_transform_plane, plane3d_negate, 0x00505330 all show 0-2 of their real arguments);
//   this rewrite supplies what each callee's own established signature (this batch, or the
//   object_physics module) requires directly, rather than trying to match Ghidra's confused
//   visible-argument count.
//   // blam-cc: stack -> start_object_index, type_mask, test_flags, origin, delta,
//   //          exclude_object_index, out_result
// UNSURE (major): radius_scale (param_5) is visibly forwarded into the mass-point branch's ray
// test call but never appears anywhere in the node-vs-bsp branch's call to
// object_collision_context_test_segment (which, per that function's own from-scratch analysis,
// takes no radius at all) -- this rewrite passes only (context, origin, delta, out_result) to
// it and leaves radius_scale unused on that path, exactly as Ghidra's own decompile of
// object_collision_context_test_segment's definition shows.
// UNSURE: out_result->leaf (0x0c) and ->point (0x18) are never written anywhere in this
// function's own decompile and are left untouched here too, matching that omission literally.
// reconciled: R23 collision_result: normal -> plane.normal, unknown_30 -> plane.d, unknown_04 -> first_leaf/first_cluster, unknown_3c -> region_index, marker_index -> node_index, unknown_40 -> permutation_index (int16), unknown_48 -> plane_index, unknown_4d -> breakable_surface_index, unknown_4e -> collision_material_index

// FIXED (objdump 0x5055b0, depth traced to the 0x505745 call): the third stack argument is the collision test
//   flags (only forwarded to object_collision_context_test_segment), the fourth the origin and the fifth the delta;
//   the draft read origin/delta one slot early and called the fifth 'radius_scale' (collision_test_movement_segment
//   already declared the flags argument).
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "physics.h"
#include "projectiles.h"

extern data_array *object_data; // 0x008603b0

extern void matrix4x3_transform_plane(real_plane3d *out, real_matrix4x3 *m,
                                       real_plane3d *plane); // 0x4cbf10, math module
extern void plane3d_negate(real_plane3d *out, real_plane3d *in); // 0x44da20, effects module

extern uint8_t object_collision_context_build(uint32_t object_index,
    object_collision_context *out_context); // 0x504e10, this module
extern uint8_t object_collision_context_test_segment(object_collision_context *context, uint32_t flags,
    real_point3d *origin, real_vector3d *delta, object_node_collision_result *out_result); // 0x504f60 // 0x504f60, this batch
extern int16_t model_collision_geometry_resolve_material_type(int16_t material_index,
    ModelCollisionGeometry *definition); // 0x505330, this batch
extern uint8_t object_physics_context_build(uint32_t object_index,
    object_physics_context *out_context); // 0x5074b0, this module
extern uint8_t object_physics_test_ray_against_mass_points(real_point3d *world_origin,
    real_vector3d *world_direction, object_physics_ray_result *out_result,
    object_physics_context *context); // 0x507610, this module
extern uint8_t ray_intersects_sphere_test(real_point3d *origin, real_point3d *center,
    real_vector3d *direction, real radius); // 0x4ce6c0, math module

// Walks the object chain starting at start_object_index exactly like
// object_collision_test_nearby_chain, but against a swept ray (origin, delta, radius_scale)
// instead of a point, keeping the single closest hit across the whole chain in *out_result
// (which the caller must pre-seed, t = FLT_MAX). Vehicles with flag bit 0x400000 set are tested
// against their physics mass points; every other matching object against its collision-node
// segment geometry, whose local-space hit plane is transformed back to world space (and negated
// on a back-face hit) before being written into *out_result. Recurses into first_child_object
// for every object visited, and into next_object for the walk itself; returns whether the chain
// (including any recursion) improved *out_result at all.
// blam-cc: stack -> start_object_index, type_mask, test_flags, origin, delta,
//          exclude_object_index, out_result
uint8_t object_collision_test_ray_nearby_chain(uint32_t start_object_index, uint32_t type_mask,
    uint32_t test_flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index,
    collision_result *out_result)
{
    uint32_t object_index = start_object_index;
    uint8_t improved = 0;

    do {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

        if (object_index != exclude_object_index && (obj->flags & 1) == 0) {
            uint8_t type = (uint8_t)obj->type;

            if ((type_mask & (1u << ((type + 8) & 0x1f))) != 0 &&
                ray_intersects_sphere_test(origin, &obj->bounding_center, delta,
                                            obj->bounding_radius)) {
                if (((1 << (type & 0x1f)) & 2) == 0 || (type_mask & 0x400000) == 0) {
                    object_collision_context node_ctx;

                    if (object_collision_context_build(object_index, &node_ctx)) {
                        object_node_collision_result node_result;

                        if (object_collision_context_test_segment(&node_ctx, test_flags, origin, delta, /* 0x505745: argument 3 */
                                                                    &node_result) &&
                            node_result.segment.t < out_result->t) {
                            real_matrix4x3 *node_matrix =
                                &((real_matrix4x3 *)node_ctx.nodes)[node_result.node_index];

                            out_result->t = node_result.segment.t;
                            out_result->type = 3;
                            matrix4x3_transform_plane((real_plane3d *)&out_result->plane.normal,
                                                       node_matrix,
                                                       (real_plane3d *)node_result.segment.plane);
                            if (node_result.segment.plane_index < 0) {
                                plane3d_negate((real_plane3d *)&out_result->plane.normal,
                                               (real_plane3d *)&out_result->plane.normal);
                            }
                            out_result->material_type = model_collision_geometry_resolve_material_type(
                                node_result.segment.material_index,
                                (ModelCollisionGeometry *)node_ctx.definition);
                            out_result->region_index = node_result.region_index;
                            out_result->node_index = node_result.node_index;
                            out_result->permutation_index = node_result.permutation_index;
                            out_result->plane_index = node_result.segment.plane_index;
                            out_result->object_index = object_index;
                            out_result->surface_index = node_result.segment.surface_index;
                            out_result->surface_flags = node_result.segment.surface_flags;
                            out_result->breakable_surface_index = node_result.segment.breakable_surface_index;
                            out_result->collision_material_index = node_result.segment.material_index;
                            improved = 1;
                        }
                    }
                } else {
                    object_physics_context phys_ctx;

                    if (object_physics_context_build(object_index, &phys_ctx)) {
                        object_physics_ray_result ray_result;

                        if (object_physics_test_ray_against_mass_points(origin, delta, &ray_result,
                                                                          &phys_ctx) &&
                            ray_result.t < out_result->t) {
                            out_result->t = ray_result.t;
                            out_result->plane.normal.i = ray_result.plane_i;
                            out_result->plane.normal.j = ray_result.plane_j;
                            out_result->plane.normal.k = ray_result.plane_k;
                            out_result->plane.d = ray_result.plane_d;
                            out_result->type = 3;
                            out_result->material_type = -1;
                            out_result->object_index = object_index;
                            out_result->region_index = -1;
                            out_result->node_index = -1;
                            out_result->plane_index = 0xffffffff;
                            out_result->surface_index = -1;
                            out_result->surface_flags = 0;
                            out_result->breakable_surface_index = 0;
                            out_result->collision_material_index = -1;
                            improved = 1;
                        }
                    }
                }

                if (obj->first_child_object != k_datum_index_none) {
                    if (object_collision_test_ray_nearby_chain(obj->first_child_object, type_mask,
                            test_flags, origin, delta, exclude_object_index, out_result)) {
                        improved = 1;
                    }
                }
            }
        }
        object_index = obj->next_object;
    } while (object_index != k_datum_index_none);

    return improved;
}

#if 0
Original Ghidra decompilation (0x5055b0):

undefined1
FUN_005055b0(uint param_1,uint param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            uint param_6,undefined2 *param_7)

{
  int iVar1;
  byte bVar2;
  int iVar3;
  undefined4 uVar4;
  char cVar5;
  undefined2 uVar6;
  undefined1 local_485;
  float local_480;
  undefined4 local_47c;
  undefined4 local_478;
  undefined4 local_474;
  undefined4 local_470;
  undefined1 local_46c [16];
  undefined1 local_45c [60];
  undefined2 local_420;
  undefined2 local_41e;
  undefined2 local_41c;
  float local_418;
  undefined4 local_410;
  int local_40c;
  undefined1 local_408;
  undefined1 local_407;
  undefined4 local_406;

  local_485 = 0;
  do {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    if ((param_1 != param_6) && ((*(byte *)(iVar1 + 0x10) & 1) == 0)) {
      bVar2 = (byte)*(undefined2 *)(iVar1 + 0xb4);
      if (((param_2 & 1 << (bVar2 + 8 & 0x1f)) != 0) &&
         (cVar5 = ray_intersects_sphere_test(*(undefined4 *)(iVar1 + 0xac)), cVar5 != '\0')) {
        if (((1 << (bVar2 & 0x1f) & 2U) == 0) || ((param_2 & 0x400000) == 0)) {
          cVar5 = FUN_00504e10();
          if ((cVar5 != '\0') &&
             ((cVar5 = FUN_00504f60(local_46c,param_3,param_4,param_5,&local_420), cVar5 != '\0' &&
              (local_418 < *(float *)(param_7 + 10))))) {
            *(float *)(param_7 + 10) = local_418;
            *param_7 = 3;
            matrix4x3_transform_plane();
            iVar3 = local_40c;
            if (local_40c < 0) {
              plane3d_negate();
            }
            uVar4 = local_406;
            uVar6 = FUN_00505330();
            param_7[0x1a] = uVar6;
            param_7[0x1e] = local_41e;
            param_7[0x1f] = local_420;
            param_7[0x20] = local_41c;
            *(int *)(param_7 + 0x24) = iVar3;
            *(uint *)(param_7 + 0x1c) = param_1;
            *(undefined4 *)(param_7 + 0x22) = local_410;
            *(undefined1 *)(param_7 + 0x26) = local_408;
            *(undefined1 *)((int)param_7 + 0x4d) = local_407;
            param_7[0x27] = (short)uVar4;
            local_485 = 1;
          }
        }
        else {
          cVar5 = FUN_005074b0();
          if (((cVar5 != '\0') &&
              (cVar5 = antenna_test_ray_against_vertex_spheres(local_45c,param_5), cVar5 != '\0'))
             && (local_480 < *(float *)(param_7 + 10))) {
            *(float *)(param_7 + 10) = local_480;
            *(undefined4 *)(param_7 + 0x12) = local_47c;
            *(undefined4 *)(param_7 + 0x14) = local_478;
            *(undefined4 *)(param_7 + 0x16) = local_474;
            *(undefined4 *)(param_7 + 0x18) = local_470;
            *param_7 = 3;
            param_7[0x1a] = 0xffff;
            *(uint *)(param_7 + 0x1c) = param_1;
            param_7[0x1e] = 0xffff;
            param_7[0x1f] = 0xffff;
            param_7[0x20] = 0xffff;
            *(undefined4 *)(param_7 + 0x22) = 0xffffffff;
            *(undefined4 *)(param_7 + 0x24) = 0xffffffff;
            *(undefined1 *)(param_7 + 0x26) = 0;
            *(undefined1 *)((int)param_7 + 0x4d) = 0;
            param_7[0x27] = 0xffff;
            local_485 = 1;
          }
        }
        if ((*(int *)(iVar1 + 0x118) != -1) &&
           (cVar5 = FUN_005055b0(*(int *)(iVar1 + 0x118),param_2,param_3,param_4,param_5,param_6,
                                 param_7), cVar5 != '\0')) {
          local_485 = 1;
        }
      }
    }
    param_1 = *(uint *)(iVar1 + 0x114);
  } while (param_1 != 0xffffffff);
  return local_485;
}
#endif
