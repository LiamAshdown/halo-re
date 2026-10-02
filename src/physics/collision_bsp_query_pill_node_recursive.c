// collision_bsp_query_pill_node_recursive  (Ghidra: FUN_005027a0, still unnamed there; name
// from out/phase2/results/physics_00.json)
// address 0x5027a0, size 1458 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/physics_types_notes.md section 2 derives collision_bsp_pill_query and
//   collision_bsp_pill_result from this function field-by-field (plane stack at 0x018,
//   projection cache at 0x218..0x228, result fields at 0x000..0x020). Structurally mirrors
//   collision_bsp_query_sphere_node_recursive (0x501a10): node descent with a margin test in
//   the else branch, leaf/bsp2d-reference processing in the if branch, but combined into one
//   function instead of split into two, and testing the swept sphere's closest approach instead
//   of a static overlap.
// register convention: none -- both arguments are Ghidra-recognized stack parameters.
//   // blam-cc: stack -> query, node_index
// UNSURE, significantly: two call sites here (bsp2d_node_find_leaf, then
// collision_bsp_surface_test_point_2d) show incomplete argument lists in Ghidra's decompile, the
// same dropped-register-argument pattern documented in collision_bsp_surface_test_point_leaf.c.
// The missing 2D point is reconstructed as the query origin projected onto the crossing plane
// (origin - side_at_origin * normal), which is exactly what this function computes moments
// later anyway to cache into query->projected_origin_i/j -- this rewrite computes it once and
// reuses it for both. breakable_surface_count/breakable_surfaces are passed as literal 0 to
// collision_bsp_surface_test_point_2d here, matching Ghidra's decompile exactly (pill queries
// evidently do not gate on breakable-surface state).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, types/math.h (const to
                                                        // match src/math/*.c own declaration)
extern double fabs(double x);
extern int32_t bsp2d_node_find_leaf(int32_t node_index, TagReflexive *bsp2d_nodes,
                                     real_point2d *point); // 0x501340, this batch
extern uint8_t collision_bsp_surface_test_point_2d(
    ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count,
    uint32_t *breakable_surfaces, int32_t surface_index, int16_t axis, uint8_t sign,
    real_point2d *point); // 0x502600, this batch
extern uint8_t collision_bsp_query_pill_leaf_recursive(collision_bsp_pill_query *query,
                                                        uint32_t bsp2d_node_index); // 0x502d60, this batch

// blam-cc: stack -> query, node_index
uint8_t collision_bsp_query_pill_node_recursive(collision_bsp_pill_query *query,
                                                 int32_t node_index)
{
    ModelCollisionGeometryBSP *bsp = (ModelCollisionGeometryBSP *)query->bsp;
    ModelCollisionGeometryBSPPlane *planes =
        (ModelCollisionGeometryBSPPlane *)bsp->planes.pointer;
    real_point3d *origin = (real_point3d *)query->origin;
    real_vector3d *delta = (real_vector3d *)query->delta;
    collision_bsp_pill_result *result = (collision_bsp_pill_result *)query->result;
    uint8_t any_hit = 0;

    if (node_index < 0) {
        if (node_index == -1) {
            return 0;
        }
        {
            uint32_t leaf_index = (uint32_t)node_index & 0x7fffffffu;
            ModelCollisionGeometryBSPLeaf *leaf =
                &((ModelCollisionGeometryBSPLeaf *)bsp->leaves.pointer)[leaf_index];
            ModelCollisionGeometryBSP2DReference *references =
                (ModelCollisionGeometryBSP2DReference *)bsp->bsp2d_references.pointer;
            int32_t ref_index = (int32_t)leaf->first_bsp2d_reference;
            int32_t ref_end = ref_index + leaf->bsp2d_reference_count;

            if (ref_index < ref_end) {
                do {
                    ModelCollisionGeometryBSP2DReference *reference = &references[ref_index];

                    if (0 < query->plane_count) {
                        int32_t i;
                        for (i = 0; i < query->plane_count; i++) {
                            if (query->planes[i] == (int32_t)reference->plane) {
                                Plane3D *plane = &planes[reference->plane & 0x7fffffffu].plane;
                                float side_origin = plane->vector.i * origin->x +
                                                    plane->vector.j * origin->y +
                                                    plane->vector.k * origin->z - plane->w;
                                float dot_delta = plane->vector.i * delta->i +
                                                  plane->vector.j * delta->j +
                                                  plane->vector.k * delta->k;
                                float t = 0.0f;
                                real_point3d proj_origin;
                                int16_t dominant_axis;
                                uint8_t sign;
                                projection_axis_pair proj;
                                real_point2d point2d;
                                int32_t found_surface;

                                if (dot_delta != 0.0f) {
                                    float inv = 1.0f / dot_delta;
                                    t = -(side_origin * inv) -
                                        (float)fabs((double)inv) * query->radius;
                                    if (0.0f <= t) {
                                        if (1.0f < t) {
                                            t = 1.0f;
                                        }
                                    } else {
                                        t = 0.0f;
                                    }
                                }

                                if (t < result->t) {
                                    if (((float)fabs((double)plane->vector.k) <
                                         (float)fabs((double)plane->vector.j)) ||
                                        ((float)fabs((double)plane->vector.k) <
                                         (float)fabs((double)plane->vector.i))) {
                                        dominant_axis = ((float)fabs((double)plane->vector.j) <
                                                          (float)fabs((double)plane->vector.i))
                                                             ? 0
                                                             : 1;
                                    } else {
                                        dominant_axis = 2;
                                    }
                                    query->projection_axis = dominant_axis;
                                    sign = (0.0f < ((float *)&plane->vector)[dominant_axis]) !=
                                           ((reference->plane & 0x80000000u) != 0);
                                    query->projection_sign = sign;

                                    proj_origin.x = -side_origin * plane->vector.i + origin->x;
                                    proj_origin.y = -side_origin * plane->vector.j + origin->y;
                                    proj_origin.z = -side_origin * plane->vector.k + origin->z;
                                    proj = k_projection_axes[dominant_axis * 2 + sign];
                                    point2d.x = ((float *)&proj_origin)[proj.i];
                                    point2d.y = ((float *)&proj_origin)[proj.j];

                                    found_surface = bsp2d_node_find_leaf(
                                        (int32_t)reference->bsp2d_node, &bsp->bsp2d_nodes,
                                        &point2d);
                                    if (collision_bsp_surface_test_point_2d(
                                            bsp, 0, 0, found_surface, dominant_axis, sign,
                                            &point2d)) {
                                        ModelCollisionGeometryBSPSurface *surface =
                                            &((ModelCollisionGeometryBSPSurface *)
                                                  bsp->surfaces.pointer)[found_surface];
                                        result->t = t;
                                        if ((int32_t)reference->plane < 0) {
                                            result->plane_i = -plane->vector.i;
                                            result->plane_j = -plane->vector.j;
                                            result->plane_k = -plane->vector.k;
                                            result->plane_d = -plane->w;
                                        } else {
                                            result->plane_i = plane->vector.i;
                                            result->plane_j = plane->vector.j;
                                            result->plane_k = plane->vector.k;
                                            result->plane_d = plane->w;
                                        }
                                        result->surface_index = found_surface;
                                        result->material_index = (int16_t)surface->material;
                                        any_hit = 1;
                                    }

                                    query->projected_origin_i = point2d.x;
                                    query->projected_origin_j = point2d.y;
                                    {
                                        real_point3d proj_delta;
                                        real_point2d point2d_delta;
                                        proj_delta.x = -dot_delta * plane->vector.i + delta->i;
                                        proj_delta.y = -dot_delta * plane->vector.j + delta->j;
                                        proj_delta.z = -dot_delta * plane->vector.k + delta->k;
                                        point2d_delta.x = ((float *)&proj_delta)[proj.i];
                                        point2d_delta.y = ((float *)&proj_delta)[proj.j];
                                        query->projected_delta_i = point2d_delta.x;
                                        query->projected_delta_j = point2d_delta.y;

                                        if (collision_bsp_query_pill_leaf_recursive(
                                                query, reference->bsp2d_node)) {
                                            any_hit = 1;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                    ref_index = ref_index + 1;
                } while (ref_index < ref_end);
            }

            if (0xff < result->leaf_count) {
                result->leaves[255] = (int32_t)leaf_index;
            } else {
                result->leaves[result->leaf_count] = (int32_t)leaf_index;
                result->leaf_count += 1;
            }
        }
        return any_hit;
    } else {
        ModelCollisionGeometryBSP3DNode *node =
            &((ModelCollisionGeometryBSP3DNode *)bsp->bsp3d_nodes.pointer)[node_index];
        Plane3D *plane = &planes[node->plane].plane;
        float side_origin =
            plane->vector.i * origin->x + plane->vector.j * origin->y + plane->vector.k * origin->z -
            plane->w;
        float dot_delta =
            plane->vector.i * delta->i + plane->vector.j * delta->j + plane->vector.k * delta->k;
        float side_end = side_origin + dot_delta;
        float margin = query->radius + 0.00024414062f;
        int crosses_below = !((side_origin > margin) && (side_end > margin));
        int crosses_above =
            (-query->radius - 0.00024414062f <= side_origin) ||
            (-query->radius - 0.00024414062f <= side_end);
        uint8_t hit1, hit2;

        if (crosses_below && crosses_above) {
            uint32_t tagged_plane =
                (dot_delta <= 0.0f) ? (node->plane & 0x7fffffffu) : (node->plane | 0x80000000u);
            query->planes[query->plane_count] = (int32_t)tagged_plane;
            query->plane_count += 1;
            hit1 = collision_bsp_query_pill_node_recursive(
                query, (int32_t)((dot_delta <= 0.0f) ? node->back_child : node->front_child));
            query->plane_count -= 1;
            hit2 = collision_bsp_query_pill_node_recursive(
                query, (int32_t)((0.0f < dot_delta) ? node->front_child : node->back_child));
            return (hit2 != 0) ? 1 : (hit1 != 0);
        }
        hit1 = collision_bsp_query_pill_node_recursive(
            query, (int32_t)(crosses_above ? node->front_child : node->back_child));
        return hit1;
    }
}

#if 0
Original Ghidra decompilation (0x5027a0):

bool FUN_005027a0(int *param_1,float param_2)

{
  uint *puVar1;
  int iVar2;
  float *pfVar3;
  float *pfVar4;
  int iVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  bool bVar10;
  bool bVar11;
  char cVar12;
  char cVar13;
  float *pfVar14;
  int iVar15;
  int iVar16;
  short sVar17;
  uint uVar18;
  int iVar19;
  bool bVar20;
  undefined8 uVar21;
  bool local_4d;
  float local_18 [4];
  float local_8;
  float local_4;

  bVar20 = false;
  local_4d = false;
  if ((int)param_2 < 0) {
    if (param_2 != -NAN) {
      uVar18 = (uint)param_2 & 0x7fffffff;
      iVar15 = *(int *)(*param_1 + 0x1c);
      iVar19 = *(int *)(iVar15 + 4 + (int)param_2 * 8);
      iVar2 = iVar15 + (int)param_2 * 8;
      if (iVar19 < *(short *)(iVar15 + 2 + (int)param_2 * 8) + iVar19) {
        do {
          puVar1 = (uint *)(*(int *)(*param_1 + 0x28) + iVar19 * 8);
          if (0 < param_1[5]) {
            iVar15 = 0;
            sVar17 = 0;
            do {
              if (param_1[iVar15 + 6] == *puVar1) {
                pfVar3 = (float *)param_1[2];
                pfVar14 = (float *)(*puVar1 * 0x10 + *(int *)(*param_1 + 0x10));
                pfVar4 = (float *)param_1[1];
                param_2 = 0.0;
                fVar6 = (*pfVar4 * *pfVar14 + pfVar4[2] * pfVar14[2] + pfVar4[1] * pfVar14[1]) -
                        pfVar14[3];
                fVar7 = *pfVar14 * *pfVar3 + pfVar3[2] * pfVar14[2] + pfVar3[1] * pfVar14[1];
                if (fVar7 != 0.0) {
                  param_2 = -(fVar6 * (1.0 / fVar7)) - ABS(1.0 / fVar7) * (float)param_1[3];
                  if (0.0 <= param_2) {
                    if (1.0 < param_2) {
                      param_2 = 1.0;
                    }
                  }
                  else {
                    param_2 = 0.0;
                  }
                }
                if (param_2 < *(float *)param_1[4]) {
                  if ((ABS(pfVar14[2]) < ABS(pfVar14[1])) || (ABS(pfVar14[2]) < ABS(*pfVar14))) {
                    sVar17 = 1;
                    if (ABS(pfVar14[1]) < ABS(*pfVar14)) {
                      sVar17 = 0;
                    }
                  }
                  else {
                    sVar17 = 2;
                  }
                  *(short *)(param_1 + 0x86) = sVar17;
                  bVar20 = 0.0 < pfVar14[sVar17] != ((*puVar1 & 0x80000000) != 0);
                  *(bool *)((int)param_1 + 0x21a) = bVar20;
                  uVar21 = FUN_00501340();
                  iVar16 = (int)uVar21;
                  iVar15 = *param_1;
                  cVar12 = FUN_00502600(0,0,iVar16,sVar17,bVar20,(int)((ulonglong)uVar21 >> 0x20));
                  if (cVar12 != '\0') {
                    iVar15 = *(int *)(iVar15 + 0x40);
                    *(float *)param_1[4] = param_2;
                    if ((int)*puVar1 < 0) {
                      iVar5 = param_1[4];
                      *(float *)(iVar5 + 4) = -*pfVar14;
                      *(float *)(iVar5 + 8) = -pfVar14[1];
                      *(float *)(iVar5 + 0xc) = -pfVar14[2];
                      *(float *)(iVar5 + 0x10) = -pfVar14[3];
                    }
                    else {
                      iVar5 = param_1[4];
                      *(float *)(iVar5 + 4) = *pfVar14;
                      *(float *)(iVar5 + 8) = pfVar14[1];
                      *(float *)(iVar5 + 0xc) = pfVar14[2];
                      *(float *)(iVar5 + 0x10) = pfVar14[3];
                    }
                    *(int *)(param_1[4] + 0x14) = iVar16;
                    *(undefined2 *)(param_1[4] + 0x1a) = *(undefined2 *)(iVar15 + iVar16 * 0xc + 10)
                    ;
                    local_4d = true;
                  }
                  pfVar3 = (float *)param_1[1];
                  fVar6 = -fVar6;
                  local_18[0] = fVar6 * *pfVar14 + *pfVar3;
                  local_18[1] = fVar6 * pfVar14[1] + pfVar3[1];
                  local_18[2] = fVar6 * pfVar14[2] + pfVar3[2];
                  iVar15 = ((uint)*(byte *)((int)param_1 + 0x21a) + (short)param_1[0x86] * 2) * 4;
                  fVar6 = local_18[*(short *)(&DAT_0065c29c + iVar15)];
                  param_1[0x88] = (int)local_18[*(short *)(&DAT_0065c29e + iVar15)];
                  param_1[0x87] = (int)fVar6;
                  pfVar3 = (float *)param_1[2];
                  fVar7 = -fVar7;
                  local_18[3] = fVar7 * *pfVar14 + *pfVar3;
                  local_8 = fVar7 * pfVar14[1] + pfVar3[1];
                  iVar15 = ((uint)*(byte *)((int)param_1 + 0x21a) + (short)param_1[0x86] * 2) * 4;
                  local_4 = fVar7 * pfVar14[2] + pfVar3[2];
                  fVar6 = local_18[*(short *)(&DAT_0065c29c + iVar15) + 3];
                  param_1[0x8a] = (int)local_18[*(short *)(&DAT_0065c29e + iVar15) + 3];
                  param_1[0x89] = (int)fVar6;
                  cVar12 = FUN_00502d60(param_1,puVar1[1]);
                  if (cVar12 != '\0') {
                    local_4d = true;
                  }
                }
                break;
              }
              sVar17 = sVar17 + 1;
              iVar15 = (int)sVar17;
            } while (iVar15 < param_1[5]);
          }
          iVar19 = iVar19 + 1;
          bVar20 = local_4d;
        } while (iVar19 < (int)*(short *)(iVar2 + 2) + *(int *)(iVar2 + 4));
      }
      iVar2 = param_1[4];
      if (0xff < *(int *)(iVar2 + 0x1c)) {
        *(uint *)(iVar2 + 0x41c) = uVar18;
        return bVar20;
      }
      *(uint *)(iVar2 + 0x20 + *(int *)(iVar2 + 0x1c) * 4) = uVar18;
      *(int *)(param_1[4] + 0x1c) = *(int *)(param_1[4] + 0x1c) + 1;
    }
  }
  else {
    iVar2 = *(int *)(*param_1 + 0x10);
    pfVar3 = (float *)param_1[1];
    puVar1 = (uint *)(*(int *)(*param_1 + 4) + (int)param_2 * 0xc);
    uVar18 = *puVar1;
    pfVar14 = (float *)(uVar18 * 0x10 + iVar2);
    pfVar4 = (float *)param_1[2];
    fVar6 = (*pfVar3 * *pfVar14 +
            pfVar3[2] * pfVar14[2] + pfVar3[1] * *(float *)(uVar18 * 0x10 + 4 + iVar2)) - pfVar14[3]
    ;
    fVar9 = *pfVar14 * *pfVar4 + pfVar4[2] * pfVar14[2] + pfVar4[1] * pfVar14[1];
    fVar7 = fVar9 + fVar6;
    fVar8 = (float)param_1[3] + 0.00024414062;
    if ((fVar6 < fVar8 == (fVar6 == fVar8)) && (fVar7 < fVar8 == (fVar7 == fVar8))) {
      bVar11 = false;
    }
    else {
      bVar11 = true;
    }
    if ((-(float)param_1[3] - 0.00024414062 <= fVar6) ||
       (-(float)param_1[3] - 0.00024414062 <= fVar7)) {
      bVar10 = true;
    }
    else {
      bVar10 = false;
    }
    if ((bVar11) && (bVar10)) {
      if (fVar9 <= 0.0) {
        uVar18 = uVar18 & 0x7fffffff;
      }
      else {
        uVar18 = uVar18 | 0x80000000;
      }
      param_1[param_1[5] + 6] = uVar18;
      param_1[5] = param_1[5] + 1;
      cVar12 = FUN_005027a0(param_1,puVar1[(fVar9 <= 0.0) + 1]);
      param_1[5] = param_1[5] + -1;
      cVar13 = FUN_005027a0(param_1,puVar1[(0.0 < fVar9) + 1]);
      if (cVar13 != '\0') {
        return true;
      }
      return cVar12 != '\0';
    }
    cVar12 = FUN_005027a0(param_1,puVar1[bVar10 + 1]);
    if (cVar12 != '\0') {
      return true;
    }
  }
  return bVar20;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
