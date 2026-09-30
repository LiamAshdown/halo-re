// physics_model_build_from_sphere_query  (Ghidra: FUN_00506440; renamed)
// address 0x506440, size 659 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (step 1: checked against objdump -d 0x506440..0x5066d3) (raised from 0.4 by the phase-4
//   integration pass, which replaced two invented callee signatures -- FUN_00501980 and
//   FUN_00503d90 -- with the module-wide ones)
// evidence: types/physics.h collision_bsp_sphere_query/_result (the sphere-centre-and-radius
//   call into FUN_00501980 matches physics.h's own citation of this exact call site: "0x506440
//   passes... the sphere centre and radius + 0.0625"); physics_model (param_7's first three
//   int16 fields, tested all-zero at the end exactly like object_physics_add_mass_point_shapes'
//   equivalent parameter, are sphere_count/pill_count/shape_count); the object.cluster_stamp /
//   object_cluster_stamp / collideable_cluster_first dedup idiom already established by
//   collision_test_movement_segment and src/objects/object_collect_in_clusters.c; the
//   006e3f01/04/08 cluster-visit-stamp trio physics.h documents as shared between this function
//   and collision_test_movement_segment.
// register convention: none recognized as in_EAX etc; all seven are Ghidra's own ordinary
//   parameters (`FUN_00506440(uint param_1, undefined4 param_2, float param_3, undefined4
//   param_4, undefined4 param_5, undefined4 param_6, short *param_7)`).
// UNSURE: the exact hidden-register arguments FUN_00501980 and FUN_00503d90 receive besides the
//   ones visible here (the sphere query's own result pointer, and FUN_00503d90's bsp/result
//   pair) -- same caveat as collision_test_movement_segment's FUN_00502060 note.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "physics.h"
#include "fn_physics.h"

extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98
extern ScenarioStructureBSP *global_structure_bsp;        // 0x00746f9c
extern breakable_surface_globals *breakable_surface_state;  // 0x006b8d78
extern int16_t global_structure_bsp_index;                  // 0x0069e8d8

extern data_array *object_data;                    // 0x008603b0
extern object_globals *object_globals_pointer;     // 0x006b8cbc
extern int32_t object_cluster_stamp;                // 0x008603cc
extern datum_index *collideable_cluster_first;      // 0x008603d0
extern data_array *collideable_object_references;   // 0x008603d4
extern uint8_t cluster_flood_in_progress;  // 0x006e3f01
extern int32_t cluster_flood_stamp;       // 0x006e3f04
extern int32_t cluster_visit_stamp[];               // 0x006e3f08

// blam-cc: EAX -> bsp, ECX -> breakable_surface_count, ESI -> result,
//          stack -> breakable_surfaces, center, radius  (declaration shared with every other
//          call site in this module)

// blam-cc: EDI -> result, stack -> bsp, margin, thickness, object_index, model; matrix and
//          material_type are reconstructed and listed last


// Runs a sphere query (center, radius + 0.0625 margin) against the structure BSP. When flags bit
// 0x20 is set and the query found geometry, converts it into physics_model proxies via
// FUN_00503d90. When flags bit 0x80 is set and any leaf was touched, walks every object in every
// touched cluster (deduping clusters and objects exactly like collision_test_movement_segment)
// and folds each into the same physics_model via collision_gather_nearby_object_shapes. Returns
// whether *model ended up with any sphere, pill or shape proxies.
uint8_t physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center, float radius,
    float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model)
{
    collision_bsp_sphere_result sphere_result;
    uint8_t found_surface;

    model->sphere_count = 0;
    model->pill_count = 0;
    model->shape_count = 0;

    if ((flags & 0x20) != 0 || (flags & 0xc0) != 0) {
        found_surface = (uint8_t)collision_bsp_query_sphere_init(global_structure_collision_bsp,
            k_maximum_breakable_surfaces_per_bsp, &sphere_result,
            breakable_surface_state->active[global_structure_bsp_index], center,
            radius + 0.0625f);

        if (found_surface && (flags & 0x20) != 0) {
            // the original passes DAT_00746f98 as the first STACK argument, which is what pins
            // physics_shape_build_proxies_from_query's param_1 down as its bsp
            physics_shape_build_proxies_from_query(&sphere_result, (real_matrix4x3 *)0,
                global_structure_collision_bsp, x_offset, y_offset, -1, model); // EAX = 0
        }

        if ((flags & 0x80) != 0 && sphere_result.leaf_count > 0) {
            int32_t stamp;
            int32_t i;

            if ((flags & 0xfff00) == 0) {
                flags |= 0xfff00;
            }
            cluster_flood_stamp++;
            object_globals_pointer->collecting_in_clusters = 1;
            stamp = object_cluster_stamp + 1;
            cluster_flood_in_progress = 1;
            object_cluster_stamp = stamp;

            for (i = 0; i < sphere_result.leaf_count; i++) {
                int16_t cluster_index = ((ScenarioStructureBSPLeaf *)
                    global_structure_bsp->leaves.pointer)[sphere_result.leaves[i] & 0x7fffffff].cluster;
                // FIXED (objdump 0x506593): the leaf index is masked with 0x7fffffff first

                if (cluster_visit_stamp[cluster_index] != cluster_flood_stamp) {
                    datum_index ref;

                    cluster_visit_stamp[cluster_index] = cluster_flood_stamp;
                    ref = collideable_cluster_first[cluster_index];
                    while (ref != k_datum_index_none) {
                        object_cluster_reference *node = (object_cluster_reference *)
                            collideable_object_references->data + (ref & 0xffff);
                        datum_index object_index = node->object_index;
                        object *obj;

                        // FIXED (objdump 0x5065e5, 0x506670): the walk also ends at a reference
                        // whose object_index is -1
                        if (object_index == k_datum_index_none) {
                            break;
                        }
                        obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

                        if (obj->cluster_stamp != stamp) {
                            obj->cluster_stamp = stamp;
                            collision_gather_nearby_object_shapes(flags, object_index, center,
                                radius + 0.0625f, x_offset, y_offset, exclude_object_index, model);
                        }
                        ref = node->next_reference;
                    }
                }
            }

            object_globals_pointer->collecting_in_clusters = 0;
            cluster_flood_in_progress = 0;
        }
    }

    return (model->sphere_count != 0 || model->pill_count != 0 || model->shape_count != 0);
}

#if 0
Original Ghidra decompilation (0x506440):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4
FUN_00506440(uint param_1,undefined4 param_2,float param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6,short *param_7)

{
  short sVar1;
  uint uVar2;
  undefined4 uVar3;
  char cVar4;
  short sVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  int local_404;
  int aiStack_400 [255];
  undefined4 uStack_4;

  uStack_4 = 0x50644a;
  param_7[0] = 0;
  param_7[1] = 0;
  param_7[2] = 0;
  uVar3 = DAT_00746f98;
  if (((param_1 & 0x20) != 0) || ((param_1 & 0xc0) != 0)) {
    cVar4 = FUN_00501980(DAT_0069e8d8 * 0x20 + 1 + DAT_006b8d78,param_2,param_3 + 0.0625);
    if ((cVar4 != '\0') && ((param_1 & 0x20) != 0)) {
      FUN_00503d90(uVar3,param_4,param_5,0xffffffff,param_7);
    }
    if (((param_1 >> 7 & 1) != 0) && (0 < local_404)) {
      if ((param_1 & 0xfff00) == 0) {
        param_1 = param_1 | 0xfff00;
      }
      DAT_006e3f04 = DAT_006e3f04 + 1;
      *(undefined1 *)(DAT_006b8cbc + 1) = 1;
      iVar6 = 0;
      iVar9 = DAT_008603cc + 1;
      DAT_006e3f01 = 1;
      sVar5 = 0;
      iVar8 = DAT_008603d4;
      DAT_008603cc = iVar9;
      if (0 < local_404) {
        do {
          sVar1 = *(short *)(aiStack_400[iVar6] * 0x10 + *(int *)(DAT_00746f9c + 0xe4) + 8);
          if (*(int *)(&DAT_006e3f08 + sVar1 * 4) != DAT_006e3f04) {
            *(int *)(&DAT_006e3f08 + sVar1 * 4) = DAT_006e3f04;
            uVar7 = *(uint *)(DAT_008603d0 + sVar1 * 4);
            if (uVar7 == 0xffffffff) {
              uVar7 = 0xffffffff;
              uVar2 = 0xffffffff;
            }
            else {
              uVar7 = uVar7 & 0xffff;
              uVar2 = *(uint *)(*(int *)(iVar8 + 0x34) + 8 + uVar7 * 0xc);
              uVar7 = *(uint *)(*(int *)(iVar8 + 0x34) + uVar7 * 0xc + 4);
            }
            while (uVar7 != 0xffffffff) {
              iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
              if (*(int *)(iVar6 + 0x14) != iVar9) {
                *(int *)(iVar6 + 0x14) = iVar9;
                FUN_005061c0(param_1,uVar7,param_2,param_3 + 0.0625,param_4,param_5,param_6,param_7)
                ;
                iVar8 = DAT_008603d4;
              }
              if (uVar2 == 0xffffffff) {
                uVar7 = 0xffffffff;
                uVar2 = 0xffffffff;
              }
              else {
                uVar7 = uVar2 & 0xffff;
                uVar2 = *(uint *)(*(int *)(iVar8 + 0x34) + 8 + uVar7 * 0xc);
                uVar7 = *(uint *)(*(int *)(iVar8 + 0x34) + uVar7 * 0xc + 4);
              }
            }
          }
          sVar5 = sVar5 + 1;
          iVar6 = (int)sVar5;
        } while (iVar6 < local_404);
      }
      *(undefined1 *)(DAT_006b8cbc + 1) = 0;
      DAT_006e3f01 = 0;
    }
  }
  if (((*param_7 == 0) && (param_7[1] == 0)) && (param_7[2] == 0)) {
    return 0;
  }
  return 1;
}
#endif
