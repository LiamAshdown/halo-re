// collision_bsp_query_sphere_collect_geometry  (Ghidra: FUN_00501d20, still unnamed there; name
// from out/phase2/results/physics_00.json)
// address 0x501d20, size 830 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/physics_types_notes.md section 2 (collision_bsp_sphere_result's three
//   dedupe-and-append lists, vertices at 0x808/0x80c, edges at 0x404/0x408, surfaces at
//   0x000/0x004, all capped at 256 -- exactly the three append loops here);
//   ModelCollisionGeometryBSPSurfaceFlags bit 0x08 = breakable (types/tags.h).
// register convention: in_EAX -> query (collision_bsp_sphere_query *). param_1 is the
//   Ghidra-recognized stack parameter (surface_index).
//   // blam-cc: EAX -> query, stack -> surface_index
// UNSURE: the second pass's call to ray_intersects_sphere_test shows only the radius argument
//   in Ghidra's decompile of the call site; origin, center and direction are register-passed
//   and dropped from the visible text (the same pattern noted elsewhere in this project, e.g.
//   the cache module review "fixed 8 call sites where Ghidra dropped register arguments"). This
//   reconstructs them as the edge's own segment tested against the query centre, which matches
//   the callee's signature and this pass's role as the per-edge counterpart to pass 1's
//   per-vertex distance test; UNSURE pending confirmation against the disassembly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, types/math.h (const to
                                                        // match src/math/*.c own declaration)
extern uint8_t ray_intersects_sphere_test(real_point3d *center, real_point3d *origin,
                                           real_vector3d *direction, real radius); // 0x4ce6c0

// blam-cc: EAX -> query, stack -> surface_index
void collision_bsp_query_sphere_collect_geometry(collision_bsp_sphere_query *query,
                                                  int32_t surface_index)
{
    ModelCollisionGeometryBSP *bsp = (ModelCollisionGeometryBSP *)query->bsp;
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
    ModelCollisionGeometryBSPEdge *edges = (ModelCollisionGeometryBSPEdge *)bsp->edges.pointer;
    float *vertex_floats = (float *)bsp->vertices.pointer;
    ModelCollisionGeometryBSPSurface *surface = &surfaces[surface_index];
    collision_bsp_sphere_result *result = (collision_bsp_sphere_result *)query->result;
    uint8_t surface_breakable_index = (uint8_t)surface->breakable_surface;

    if ((surface->flags & 0x08) == 0 ||
        (int16_t)query->breakable_surface_count <= (int16_t)(uint16_t)surface_breakable_index ||
        (query->breakable_surfaces[surface_breakable_index >> 5] &
         (1u << (surface_breakable_index & 0x1f))) != 0) {
        int32_t start_edge = (int32_t)surface->first_edge;
        int32_t edge_index;
        int any_hit = 0;
        float radius_sq = query->radius * query->radius;
        real_point3d *center = (real_point3d *)query->center;

        // pass 1: each vertex within radius of the sphere centre
        edge_index = start_edge;
        do {
            ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
            int owns_right_side = ((int32_t)edge->right_surface == surface_index);
            uint32_t near_vertex = owns_right_side ? edge->end_vertex : edge->start_vertex;
            float vx = vertex_floats[near_vertex * 4 + 0];
            float vy = vertex_floats[near_vertex * 4 + 1];
            float vz = vertex_floats[near_vertex * 4 + 2];
            float dist_sq = (vz - center->z) * (vz - center->z) +
                            (vy - center->y) * (vy - center->y) +
                            (vx - center->x) * (vx - center->x);

            // equivalent to (dist_sq <= radius_sq); see the XOR idiom in
            // breakable_surface_apply_damage.c
            if (dist_sq <= radius_sq) {
                int32_t i;
                int found = 0;
                for (i = 0; i < result->vertex_count; i++) {
                    if (result->vertices[i] == (int32_t)near_vertex) {
                        found = 1;
                        break;
                    }
                }
                if (!found && result->vertex_count < 0x100) {
                    result->vertices[result->vertex_count] = (int32_t)near_vertex;
                    result->vertex_count += 1;
                }
                any_hit = 1;
            }
            edge_index = (int32_t)(owns_right_side ? edge->reverse_edge : edge->forward_edge);
        } while (edge_index != start_edge);

        // pass 2: each edge segment within radius of the sphere
        edge_index = start_edge;
        do {
            ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
            real_point3d edge_origin;
            real_vector3d edge_direction;

            edge_origin.x = vertex_floats[edge->start_vertex * 4 + 0];
            edge_origin.y = vertex_floats[edge->start_vertex * 4 + 1];
            edge_origin.z = vertex_floats[edge->start_vertex * 4 + 2];
            edge_direction.i = vertex_floats[edge->end_vertex * 4 + 0] - edge_origin.x;
            edge_direction.j = vertex_floats[edge->end_vertex * 4 + 1] - edge_origin.y;
            edge_direction.k = vertex_floats[edge->end_vertex * 4 + 2] - edge_origin.z;

            if (ray_intersects_sphere_test(center, &edge_origin, &edge_direction,
                                            query->radius)) {
                int32_t i;
                int found = 0;
                for (i = 0; i < result->edge_count; i++) {
                    if (result->edges[i] == edge_index) {
                        found = 1;
                        break;
                    }
                }
                if (!found && result->edge_count < 0x100) {
                    result->edges[result->edge_count] = edge_index;
                    result->edge_count += 1;
                }
                any_hit = 1;
            }
            edge_index = ((int32_t)edge->right_surface == surface_index)
                             ? (int32_t)edge->reverse_edge
                             : (int32_t)edge->forward_edge;
        } while (edge_index != start_edge);

        if (!any_hit) {
            // fallback: is the query's cached projected centre inside this surface's 2D
            // boundary? Same edge-loop cross-product idiom as
            // collision_bsp_surface_test_point_side_2d, but against the sphere query's own
            // cached projection (set by the caller, collision_bsp_query_sphere_node_recursive)
            // instead of re-deriving one.
            projection_axis_pair proj =
                k_projection_axes[query->projection_axis * 2 + query->projection_sign];
            edge_index = start_edge;
            do {
                ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
                int owns_right_side = ((int32_t)edge->right_surface == surface_index);
                uint32_t near_vertex = owns_right_side ? edge->end_vertex : edge->start_vertex;
                uint32_t far_vertex = owns_right_side ? edge->start_vertex : edge->end_vertex;
                float near_i =
                    vertex_floats[near_vertex * 4 + proj.i] - query->projected_center_i;
                float near_j =
                    vertex_floats[near_vertex * 4 + proj.j] - query->projected_center_j;
                float far_i = vertex_floats[far_vertex * 4 + proj.i] - query->projected_center_i;
                float far_j = vertex_floats[far_vertex * 4 + proj.j] - query->projected_center_j;

                if (near_i * far_j - near_j * far_i < 0.0f) {
                    return;
                }
                edge_index = (int32_t)(owns_right_side ? edge->reverse_edge : edge->forward_edge);
            } while (edge_index != start_edge);
        }

        {
            int32_t i;
            for (i = 0; i < result->surface_count; i++) {
                if (result->surfaces[i] == surface_index) {
                    return;
                }
            }
            if (result->surface_count < 0x100) {
                result->surfaces[result->surface_count] = surface_index;
                result->surface_count += 1;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x501d20):

void FUN_00501d20(int param_1)

{
  int iVar1;
  int iVar2;
  float *pfVar3;
  int *piVar4;
  float fVar5;
  float fVar6;
  char cVar7;
  int *in_EAX;
  float *pfVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  short sVar15;
  uint uVar16;
  bool bVar17;

  iVar1 = *(int *)(*in_EAX + 0x40) + param_1 * 0xc;
  if ((((*(byte *)(iVar1 + 8) & 8) == 0) ||
      ((short)in_EAX[1] <= (short)(ushort)*(byte *)(iVar1 + 9))) ||
     ((*(uint *)(in_EAX[2] + (uint)(*(byte *)(iVar1 + 9) >> 5) * 4) &
      1 << (*(byte *)(iVar1 + 9) & 0x1f)) != 0)) {
    iVar11 = *(int *)(iVar1 + 4);
    bVar17 = false;
    fVar5 = (float)in_EAX[4] * (float)in_EAX[4];
    do {
      iVar11 = *(int *)(*in_EAX + 0x4c) + iVar11 * 0x18;
      uVar16 = (uint)(*(int *)(iVar11 + 0x14) == param_1);
      iVar2 = *(int *)(iVar11 + uVar16 * 4);
      pfVar8 = (float *)(iVar2 * 0x10 + *(int *)(*in_EAX + 0x58));
      pfVar3 = (float *)in_EAX[3];
      fVar6 = (pfVar8[2] - pfVar3[2]) * (pfVar8[2] - pfVar3[2]) +
              (pfVar8[1] - pfVar3[1]) * (pfVar8[1] - pfVar3[1]) +
              (*pfVar8 - *pfVar3) * (*pfVar8 - *pfVar3);
      if (fVar6 < fVar5 != (fVar6 == fVar5)) {
        iVar10 = in_EAX[5];
        iVar9 = *(int *)(iVar10 + 0x808);
        sVar15 = 0;
        if (0 < iVar9) {
          iVar12 = 0;
          do {
            if (*(int *)(iVar10 + 0x80c + iVar12 * 4) == iVar2) goto LAB_00501e20;
            sVar15 = sVar15 + 1;
            iVar12 = (int)sVar15;
          } while (iVar12 < *(int *)(iVar10 + 0x808));
        }
        if (iVar9 < 0x100) {
          *(int *)(iVar10 + 0x80c + iVar9 * 4) = iVar2;
          *(int *)(iVar10 + 0x808) = *(int *)(iVar10 + 0x808) + 1;
        }
LAB_00501e20:
        bVar17 = true;
      }
      iVar11 = *(int *)(iVar11 + 8 + uVar16 * 4);
    } while (iVar11 != *(int *)(iVar1 + 4));
    iVar11 = *(int *)(iVar1 + 4);
    do {
      iVar2 = *(int *)(*in_EAX + 0x4c) + iVar11 * 0x18;
      iVar10 = *(int *)(iVar2 + 0x14);
      cVar7 = ray_intersects_sphere_test(in_EAX[4]);
      if (cVar7 != '\0') {
        iVar9 = in_EAX[5];
        iVar12 = *(int *)(iVar9 + 0x404);
        sVar15 = 0;
        if (0 < iVar12) {
          iVar13 = 0;
          do {
            if (*(int *)(iVar9 + 0x408 + iVar13 * 4) == iVar11) goto LAB_00501eea;
            sVar15 = sVar15 + 1;
            iVar13 = (int)sVar15;
          } while (iVar13 < *(int *)(iVar9 + 0x404));
        }
        if (iVar12 < 0x100) {
          *(int *)(iVar9 + 0x408 + iVar12 * 4) = iVar11;
          *(int *)(iVar9 + 0x404) = *(int *)(iVar9 + 0x404) + 1;
        }
LAB_00501eea:
        bVar17 = true;
      }
      iVar11 = *(int *)(iVar2 + 8 + (uint)(iVar10 == param_1) * 4);
    } while (iVar11 != *(int *)(iVar1 + 4));
    if (!bVar17) {
      iVar11 = *(int *)(*in_EAX + 0x4c);
      iVar2 = *(int *)(*in_EAX + 0x58);
      iVar9 = ((uint)*(byte *)((int)in_EAX + 0x21e) + (short)in_EAX[0x87] * 2) * 4;
      iVar10 = *(int *)(iVar1 + 4);
      do {
        iVar12 = iVar11 + iVar10 * 0x18;
        bVar17 = *(int *)(iVar11 + 0x14 + iVar10 * 0x18) == param_1;
        uVar16 = (uint)bVar17;
        iVar10 = *(int *)(iVar12 + uVar16 * 4) * 0x10 + iVar2;
        iVar13 = *(int *)(iVar12 + (uint)!bVar17 * 4) * 0x10 + iVar2;
        iVar14 = ((uint)*(byte *)((int)in_EAX + 0x21e) + (short)in_EAX[0x87] * 2) * 4;
        if ((*(float *)(iVar10 + *(short *)(&DAT_0065c29c + iVar14) * 4) - (float)in_EAX[0x88]) *
            (*(float *)(*(short *)(&DAT_0065c29e + iVar9) * 4 + iVar13) - (float)in_EAX[0x89]) -
            (*(float *)(iVar10 + *(short *)(&DAT_0065c29e + iVar14) * 4) - (float)in_EAX[0x89]) *
            (*(float *)(*(short *)(&DAT_0065c29c + iVar9) * 4 + iVar13) - (float)in_EAX[0x88]) < 0.0
           ) {
          return;
        }
        iVar10 = *(int *)(iVar12 + 8 + uVar16 * 4);
      } while (iVar10 != *(int *)(iVar1 + 4));
    }
    piVar4 = (int *)in_EAX[5];
    iVar1 = *piVar4;
    sVar15 = 0;
    if (0 < iVar1) {
      iVar11 = 0;
      do {
        if (piVar4[iVar11 + 1] == param_1) {
          return;
        }
        sVar15 = sVar15 + 1;
        iVar11 = (int)sVar15;
      } while (iVar11 < *piVar4);
    }
    if (iVar1 < 0x100) {
      piVar4[iVar1 + 1] = param_1;
      *piVar4 = *piVar4 + 1;
    }
  }
  return;
}
#endif
