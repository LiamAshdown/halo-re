// collision_bsp_surface_closest_edge_point_2d  (Ghidra: FUN_005015a0, still unnamed there; name
// from out/phase2/results/physics_00.json)
// address 0x5015a0, size 576 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x5015a0..0x5017e0 (flag logic re-derived; the gotos are equivalent).)
// evidence: same ModelCollisionGeometryBSP surfaces/edges/vertices layout and
//   k_projection_axes usage as collision_bsp_surface_get_vertices (0x501400) and
//   collision_bsp_surface_test_point_side_2d (0x5014a0).
// register convention: in_EAX -> bsp (ModelCollisionGeometryBSP *). param_1..param_5 are
//   Ghidra-recognized stack parameters (surface_index, axis, sign, point, out_point).
//   // blam-cc: EAX -> bsp, stack -> surface_index, axis, sign, point, out_point
// UNSURE: this is the closest-point-on-a-convex-2D-boundary algorithm, tracking two boolean
// "outside this edge's half-plane" flags per edge (this edge, and the previous one) to decide
// between three outcomes: a perpendicular foot on the current edge, a shared-vertex corner, or
// (falling out of the loop, having walked the whole boundary) either the query point unchanged
// or the starting edge's near vertex.
// Ghidra reuses one stack/register slot for several different roles across the walk (the axis
// parameter becomes a saved flag after its one read; a single local carries two different
// booleans within one iteration). Rather than guess a cleaner formulation and risk changing a
// corner case, this rewrite keeps the same reused locals with the same read/write order as the
// decompiled code, gated behind the same gotos, and only replaces the raw offsets with typed
// field access.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, types/math.h (const to
                                                        // match src/math/*.c own declaration)

// blam-cc: EAX -> bsp, stack -> surface_index, axis, sign, point, out_point
uint32_t collision_bsp_surface_closest_edge_point_2d(ModelCollisionGeometryBSP *bsp,
                                                      int32_t surface_index, uint16_t axis,
                                                      uint8_t sign, real_point2d *point,
                                                      real_point2d *out_point)
{
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
    ModelCollisionGeometryBSPEdge *edges = (ModelCollisionGeometryBSPEdge *)bsp->edges.pointer;
    // reinterpreted as float[4] per vertex (x, y, z, first_edge); only [0..2] are ever indexed
    float *vertex_floats = (float *)bsp->vertices.pointer;
    int32_t start_edge = (int32_t)surfaces[surface_index].first_edge;
    int32_t edge_index = start_edge;
    projection_axis_pair proj = k_projection_axes[(int16_t)axis * 2 + sign];

    // outside_near (bVar10) / outside_far (bVar16): this edge's two half-plane test results.
    // prev_outside_near / prev_outside_far (local_2f / local_2e): the previous edge's results.
    // first_outside_far (local_2d): the very first edge's outside_far, reused after the loop.
    uint8_t outside_near = 0, outside_far = 0;
    uint8_t prev_outside_near = 0, prev_outside_far = 0;
    uint8_t first_outside_far = 0;

    do {
        ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
        int owns_right_side = ((int32_t)edge->right_surface == surface_index);
        uint32_t near_vertex = owns_right_side ? edge->end_vertex : edge->start_vertex;
        uint32_t far_vertex = owns_right_side ? edge->start_vertex : edge->end_vertex;
        float near_i = vertex_floats[near_vertex * 4 + proj.i];
        float near_j = vertex_floats[near_vertex * 4 + proj.j];
        float edge_i = vertex_floats[far_vertex * 4 + proj.i] - near_i;
        float edge_j = vertex_floats[far_vertex * 4 + proj.j] - near_j;

        outside_far = 0;
        outside_near = 0;
        if (edge_j * (point->x - near_i) - (point->y - near_j) * edge_i <= 0.0f) {
        side_test_failed:
            outside_far = outside_near;
            outside_near = 0;
        } else {
            float dot = edge_i * (point->x - near_i) + edge_j * (point->y - near_j);
            if (0.0f <= dot) {
                float len2 = edge_i * edge_i + edge_j * edge_j;
                if (dot <= len2) {
                    float t = dot / len2;
                    out_point->x = edge_i * t + near_i;
                    out_point->y = edge_j * t + near_j;
                    return 0;
                }
                outside_near = 1;
                goto side_test_failed;
            }
            outside_near = 1;
        }

        if (edge_index == start_edge) {
            axis = (uint16_t)outside_near; // reuse the (already-consumed) axis parameter as a
                                            // saved flag, exactly as Ghidra's decompile does
            first_outside_far = outside_far;
        } else {
            if (prev_outside_far == 0) {
                if (outside_near == 0) {
                    goto next_edge;
                }
            } else {
                prev_outside_near = outside_far; // temporary reuse: "current outside_far",
                                                  // overwritten below before next iteration
                if (outside_near != 0) {
                    goto corner_hit;
                }
            }
            if (prev_outside_near == 0) {
            corner_hit:
                out_point->x = near_i;
                out_point->y = near_j;
                return 0;
            }
        }
    next_edge:
        edge_index = (int32_t)(owns_right_side ? edge->reverse_edge : edge->forward_edge);
        prev_outside_near = outside_near;
        prev_outside_far = outside_far;
    } while (edge_index != start_edge);

    if (outside_far == 0) {
        first_outside_far = outside_near;
        if ((int8_t)axis == 0) {
            goto point_unchanged;
        }
    } else if ((int8_t)axis != 0) {
        goto near_vertex_of_start_edge;
    }
    if (first_outside_far == 0) {
    near_vertex_of_start_edge: {
        // edge_index == start_edge here (the loop only exits when it wraps back around); this
        // recomputes the same "near vertex" selection the first iteration used.
        ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
        int owns_right_side = ((int32_t)edge->right_surface == surface_index);
        uint32_t near_vertex = owns_right_side ? edge->end_vertex : edge->start_vertex;
        out_point->x = vertex_floats[near_vertex * 4 + proj.i];
        out_point->y = vertex_floats[near_vertex * 4 + proj.j];
        return 0;
    }
    }
point_unchanged:
    out_point->x = point->x;
    out_point->y = point->y;
    return 1;
}

#if 0
Original Ghidra decompilation (0x5015a0):

undefined4 FUN_005015a0(int param_1,ushort param_2,byte param_3,float *param_4,float *param_5)

{
  int iVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  byte bVar10;
  int in_EAX;
  int iVar11;
  uint uVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  byte bVar16;
  int iVar17;
  bool bVar18;
  byte local_2f;
  byte local_2e;
  byte local_2d;

  iVar4 = *(int *)(*(int *)(in_EAX + 0x40) + 4 + param_1 * 0xc);
  iVar5 = *(int *)(in_EAX + 0x4c);
  iVar14 = *(int *)(in_EAX + 0x58);
  iVar11 = ((uint)param_3 + (short)param_2 * 2) * 4;
  iVar17 = *(short *)(&DAT_0065c29c + iVar11) * 4;
  iVar15 = *(short *)(&DAT_0065c29e + iVar11) * 4;
  iVar11 = iVar4;
  do {
    iVar1 = iVar5 + iVar11 * 0x18;
    bVar18 = *(int *)(iVar1 + 0x14) == param_1;
    uVar12 = (uint)bVar18;
    iVar13 = *(int *)(iVar1 + uVar12 * 4) * 0x10 + iVar14;
    fVar2 = *(float *)(iVar13 + iVar17);
    fVar3 = *(float *)(iVar15 + iVar13);
    iVar13 = *(int *)(iVar1 + (uint)!bVar18 * 4) * 0x10 + iVar14;
    bVar16 = 0;
    fVar6 = *(float *)(iVar13 + iVar17) - fVar2;
    fVar7 = *(float *)(iVar15 + iVar13) - fVar3;
    bVar10 = 0;
    if (fVar7 * (*param_4 - fVar2) - (param_4[1] - fVar3) * fVar6 <= 0.0) {
LAB_005016d1:
      bVar16 = bVar10;
      bVar10 = 0;
    }
    else {
      fVar8 = fVar6 * (*param_4 - fVar2) + fVar7 * (param_4[1] - fVar3);
      if (0.0 <= fVar8) {
        fVar9 = fVar6 * fVar6 + fVar7 * fVar7;
        if (fVar8 <= fVar9) {
          fVar8 = fVar8 / fVar9;
          *param_5 = fVar6 * fVar8 + fVar2;
          param_5[1] = fVar7 * fVar8 + fVar3;
          return 0;
        }
        bVar10 = 1;
        goto LAB_005016d1;
      }
      bVar10 = 1;
    }
    if (iVar11 == iVar4) {
      param_2 = (ushort)bVar10;
      local_2d = bVar16;
    }
    else {
      if (local_2e == 0) {
        if (bVar10 == 0) goto LAB_0050171c;
      }
      else {
        local_2f = bVar16;
        if (bVar10 != 0) goto LAB_005016eb;
      }
      if (local_2f == 0) {
LAB_005016eb:
        *param_5 = fVar2;
        param_5[1] = fVar3;
        return 0;
      }
    }
LAB_0050171c:
    iVar11 = *(int *)(iVar1 + 8 + uVar12 * 4);
    local_2f = bVar10;
    local_2e = bVar16;
  } while (iVar11 != iVar4);
  if (bVar16 == 0) {
    local_2d = bVar10;
    if ((char)param_2 == '\0') goto LAB_0050177c;
  }
  else if ((char)param_2 != '\0') goto LAB_005017a4;
  if (local_2d == 0) {
LAB_005017a4:
    iVar14 = *(int *)(iVar5 + iVar11 * 0x18 +
                     (uint)(*(int *)(iVar5 + 0x14 + iVar11 * 0x18) == param_1) * 4) * 0x10 + iVar14;
    fVar2 = *(float *)(iVar15 + iVar14);
    *param_5 = *(float *)(iVar14 + iVar17);
    param_5[1] = fVar2;
    return 0;
  }
LAB_0050177c:
  *param_5 = *param_4;
  param_5[1] = param_4[1];
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
