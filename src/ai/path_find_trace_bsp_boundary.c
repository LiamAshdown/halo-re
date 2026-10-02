// path_find_trace_bsp_boundary  (Ghidra: path_find_trace_bsp_boundary, renamed)
// address 0x43d9b0, size 1237 bytes
// name confidence: 0.35  rewrite confidence: 0.85
// REWRITTEN from objdump 0x43d9b0..0x43de84. Walks the 2D segment start -> end across the path find map's
//   collision surfaces (map +0xb4 bsp: surfaces +0x40 (12 bytes: plane | flip bit, first edge, flags, breakable
//   index), edges +0x4c (0x18: two vertices, two next edges, two surfaces), vertices +0x58 (16 bytes), planes
//   +0x10) starting on start_surface. In each surface every edge is tested (walked forwards on the side the
//   surface lies on): the end point beyond the edge marks "outside"; a segment that crosses the edge moves into
//   the neighbouring surface when that surface is walkable (map flag table +0x1e8 bit 0x40) and not intact glass
//   (flag 0x80 with the breakable bit clear; ignore_permission skips both tests), otherwise the trace stops at the
//   crossing: position = start + D * t with t = (A x E - |E| / 128) / (D x E) (A = edge start - start),
//   z from the surface plane (0x44d860), result {1, position, surface, edge, t}, return 1.
//   With no crossing edge: an end point inside the surface finishes {0, end on the surface, surface, -1, 1.0},
//   return 0 -- unless a target surface was given, this is not it and no edge borders it, which reports
//   {1, start on the start surface, -1, -1, 0}. An end point outside with no crossing (numerical corner case)
//   averages the surface's vertices (x, y), traces from there back to the start once (recursively into a local
//   result) and, when that trace is clear, retries from the surface it ended in; otherwise it reports the start as blocked.
// blam-cc: stack -> map, ignore_permission, start, start_surface, end, target_surface, out_result

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t *breakable_surface_state;          // 0x006b8d78
extern int16_t global_structure_bsp_index;        // 0x0069e8d8
extern real_point3d *global_zero_vector3d_pointer; // 0x006966f8 -> 0x0065c230 {0,0,0}
extern double sqrt(double x); // FSQRT
extern real_point3d *decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis,
    const real_plane3d *plane, const real_point2d *known); // 0x44d860, stack out, AL, SI, EBX, EDI
extern real_point3d *collision_bsp_surface_solve_third_axis(ModelCollisionGeometryBSP *collision_bsp,
    int32_t surface_index, uint8_t component_sign, real_point3d *out, int32_t dominant_axis,
    const real_point2d *known); // 0x501470, ECX, EAX, stack, stack, ESI, EDI

#define SURFACE(bsp, i) (*(uint8_t **)((bsp) + 0x40) + (i) * 12)
#define EDGE(bsp, i) (*(uint8_t **)((bsp) + 0x4c) + (i) * 0x18)
#define VERTEX(bsp, i) ((float *)(*(uint8_t **)((bsp) + 0x58) + (i) * 16))
#define SURFACE_PLANE(bsp, i) \
    ((real_plane3d *)(*(uint8_t **)((bsp) + 0x10) + (*(uint32_t *)SURFACE(bsp, i) & 0x7fffffff) * 16))

uint8_t path_find_trace_bsp_boundary(void *map, uint8_t ignore_permission, real_point3d *start, int32_t start_surface,
    real_point3d *end, int32_t target_surface, path_find_boundary_crossing *out_result)
{
    uint8_t *bsp = *(uint8_t **)((uint8_t *)map + 0xb4);
    uint8_t *walkable = *(uint8_t **)((uint8_t *)map + 0x1e8);
    uint32_t *broken = (uint32_t *)(breakable_surface_state + 1 + global_structure_bsp_index * 32);
    float dx = end->x - start->x;
    float dy = end->y - start->y;
    uint8_t retried = 0;
    int32_t surface = start_surface;

    for (;;) {
        uint8_t *record = SURFACE(bsp, surface);
        int32_t edge_index = *(int32_t *)(record + 4);
        real_point3d centroid = *global_zero_vector3d_pointer;
        int32_t vertex_count = 0;
        uint8_t outside = 0;
        uint8_t borders_target = 0;
        int32_t next_surface = -1;

        do {
            uint8_t *edge = EDGE(bsp, edge_index);
            uint8_t side = (uint8_t)(surface == *(int32_t *)(edge + 0x14));
            float *va = VERTEX(bsp, *(int32_t *)(edge + (side ? 0 : 1) * 4));
            float *vb = VERTEX(bsp, *(int32_t *)(edge + side * 4));
            int32_t other = *(int32_t *)(edge + 0x10 + (side ? 0 : 1) * 4);
            float ex = vb[0] - va[0];
            float ey = vb[1] - va[1];
            float px = end->x - va[0];
            float py = end->y - va[1];
            float ax = va[0] - start->x;
            float ay = va[1] - start->y;
            float bx = vb[0] - start->x;
            float by = vb[1] - start->y;

            if (other == target_surface) {
                borders_target = 1;
            }
            centroid.x += va[0];
            centroid.y += va[1];
            centroid.z += va[2];
            vertex_count++;
            if (ex * py - ey * px > 0.0f) {
                outside = 1;
                if (ay * dx - dy * ax > 0.0f && dy * bx - by * dx > 0.0f) {
                    // 0x43dba2: the segment leaves through this edge
                    uint8_t flags = walkable[other];
                    uint8_t passable = (uint8_t)((flags >> 6) & 1);

                    if (!ignore_permission && passable && (flags & 0x80) != 0) {
                        uint32_t bit = SURFACE(bsp, other)[9];

                        passable = (uint8_t)((broken[bit >> 5] & (1u << (bit & 0x1f))) != 0);
                    }
                    if (passable) {
                        next_surface = other;
                        break;
                    }
                    {
                        // 0x43dca4: blocked at this edge
                        float length = (float)sqrt(ey * ey + ex * ex);
                        float t = (ay * ex - ey * ax - length * 0.0078125f) / (dy * ex - ey * dx);
                        real_point2d hit;

                        hit.x = dx * t + start->x;
                        hit.y = dy * t + start->y;
                        decal_plane_solve_third_axis(&out_result->position, 1, 2, SURFACE_PLANE(bsp, surface), &hit);
                        out_result->edge_a = surface;
                        out_result->edge_b = edge_index;
                        out_result->found = 1;
                        out_result->fraction = t;
                        return 1;
                    }
                }
            }
            edge_index = *(int32_t *)(edge + 8 + side * 4);
        } while (edge_index != *(int32_t *)(record + 4));

        if (next_surface != -1) {
            surface = next_surface;
            continue;
        }
        if (!outside) {
            // 0x43ddc7: the end point lies in this surface
            if (surface != target_surface && !borders_target && target_surface != -1) {
                collision_bsp_surface_solve_third_axis((ModelCollisionGeometryBSP *)bsp, start_surface, 1,
                    &out_result->position, 2, (real_point2d *)start);
                out_result->edge_a = -1;
                out_result->edge_b = -1;
                out_result->found = 1;
                out_result->fraction = 0.0f;
                return 1;
            }
            decal_plane_solve_third_axis(&out_result->position, 1, 2, SURFACE_PLANE(bsp, surface), (real_point2d *)end);
            out_result->edge_a = surface;
            out_result->edge_b = -1;
            out_result->found = 0;
            out_result->fraction = 1.0f;
            return 0;
        }
        {
            // 0x43dc1d: outside without a crossing edge
            float scale = 1.0f / (float)vertex_count;
            path_find_boundary_crossing local_result;

            centroid.x *= scale;
            centroid.y *= scale;
            if (!retried && walkable[surface] != 0 &&
                path_find_trace_bsp_boundary(map, ignore_permission, &centroid, surface, start, -1, &local_result) == 0) {
                retried = 1;
                surface = local_result.edge_a; // 0x43dc8e: where the trace back to the start ended
                continue;
            }
        }
        // 0x43dd6d: report the start as blocked
        decal_plane_solve_third_axis(&out_result->position, 1, 2, SURFACE_PLANE(bsp, start_surface),
            (real_point2d *)start);
        out_result->edge_a = -1;
        out_result->edge_b = -1;
        out_result->found = 1;
        out_result->fraction = 0.0f;
        return 1;
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043d9b0 @ 0x43d9b0) ----
undefined4
FUN_0043d9b0(int param_1,undefined4 param_2,float *param_3,int param_4,float *param_5,int param_6,
            undefined1 *param_7)

{
  int iVar1;
  int iVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  byte bVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  bool bVar16;
  bool bVar17;
  char cVar18;
  short sVar19;
  int iVar20;
  float *pfVar21;
  bool bVar22;
  int iVar23;
  float *pfVar24;
  int iVar25;
  bool bVar26;
  float local_28;
  float local_24;
  float local_20;
  undefined1 local_1c [16];
  int local_c;

  iVar9 = *(int *)(param_1 + 0xb4);
  iVar10 = *(int *)(param_1 + 0x1e8);
  iVar3 = DAT_0069e8d8 * 0x20 + 1 + DAT_006b8d78;
  fVar12 = *param_5 - *param_3;
  bVar16 = false;
  fVar13 = param_5[1] - param_3[1];
LAB_0043da13:
  do {
    iVar20 = param_4;
    iVar1 = *(int *)(iVar9 + 0x40) + iVar20 * 0xc;
    iVar25 = *(int *)(iVar1 + 4);
    local_28 = *(float *)PTR_DAT_006966f8;
    local_24 = *(float *)(PTR_DAT_006966f8 + 4);
    local_20 = *(float *)(PTR_DAT_006966f8 + 8);
    iVar11 = *(int *)(iVar9 + 0x58);
    sVar19 = 0;
    bVar22 = false;
    bVar17 = false;
    do {
      iVar2 = *(int *)(iVar9 + 0x4c) + iVar25 * 0x18;
      bVar26 = iVar20 == *(int *)(*(int *)(iVar9 + 0x4c) + 0x14 + iVar25 * 0x18);
      iVar23 = *(int *)(iVar2 + (uint)bVar26 * 4) * 0x10;
      pfVar24 = (float *)(iVar23 + iVar11);
      pfVar21 = (float *)(*(int *)(iVar2 + (uint)!bVar26 * 4) * 0x10 + iVar11);
      fVar14 = *(float *)(iVar23 + iVar11) - *pfVar21;
      fVar15 = pfVar24[1] - pfVar21[1];
      fVar4 = *pfVar21;
      fVar5 = *param_3;
      fVar6 = pfVar21[1];
      fVar7 = param_3[1];
      if (*(int *)(iVar2 + 0x10 + (uint)!bVar26 * 4) == param_6) {
        bVar17 = true;
      }
      local_28 = local_28 + *pfVar21;
      local_24 = local_24 + pfVar21[1];
      local_20 = local_20 + pfVar21[2];
      sVar19 = sVar19 + 1;
      if (((0.0 < fVar14 * (param_5[1] - pfVar21[1]) - fVar15 * (*param_5 - *pfVar21)) &&
          (bVar22 = true, 0.0 < (fVar6 - fVar7) * fVar12 - fVar13 * (fVar4 - fVar5))) &&
         (0.0 < fVar13 * (*pfVar24 - *param_3) - (pfVar24[1] - param_3[1]) * fVar12)) {
        param_4 = *(int *)(iVar2 + 0x10 + (uint)!bVar26 * 4);
        bVar8 = *(byte *)(param_4 + iVar10);
        bVar22 = (bool)(bVar8 >> 6 & 1);
        if ((char)param_2 == '\0') {
          if (bVar22 == false) goto LAB_0043dca4;
          if ((char)bVar8 < '\0') {
            bVar8 = *(byte *)(*(int *)(iVar9 + 0x40) + 9 + param_4 * 0xc);
            bVar22 = (*(uint *)(iVar3 + (uint)(bVar8 >> 5) * 4) & 1 << (bVar8 & 0x1f)) != 0;
          }
        }
        if (bVar22 == false) {
LAB_0043dca4:
          FUN_0044d860(param_7 + 4);
          *(int *)(param_7 + 0x10) = iVar20;
          *(int *)(param_7 + 0x14) = iVar25;
          *param_7 = 1;
          *(float *)(param_7 + 0x18) =
               (((fVar6 - fVar7) * fVar14 - fVar15 * (fVar4 - fVar5)) -
               SQRT(fVar14 * fVar14 + fVar15 * fVar15) * 0.0078125) /
               (fVar13 * fVar14 - fVar15 * fVar12);
          return 1;
        }
        goto LAB_0043da13;
      }
      iVar25 = *(int *)(iVar2 + 8 + (uint)bVar26 * 4);
    } while (iVar25 != *(int *)(iVar1 + 4));
    if (!bVar22) {
      if (((iVar20 != param_6) && (!bVar17)) && (param_6 != -1)) {
        FUN_00501470(1,param_7 + 4);
        *(undefined4 *)(param_7 + 0x10) = 0xffffffff;
        *(undefined4 *)(param_7 + 0x14) = 0xffffffff;
        *param_7 = 1;
        *(undefined4 *)(param_7 + 0x18) = 0;
        return 1;
      }
      FUN_0044d860(param_7 + 4);
      *(int *)(param_7 + 0x10) = iVar20;
      *(undefined4 *)(param_7 + 0x14) = 0xffffffff;
      *param_7 = 0;
      *(undefined4 *)(param_7 + 0x18) = 0x3f800000;
      return 0;
    }
    local_28 = local_28 * (1.0 / (float)(int)sVar19);
    local_24 = (1.0 / (float)(int)sVar19) * local_24;
    if (((bVar16) || (*(char *)(iVar20 + iVar10) == '\0')) ||
       (cVar18 = FUN_0043d9b0(param_1,param_2,&local_28,iVar20,param_3,0xffffffff,local_1c),
       cVar18 != '\0')) {
      FUN_0044d860(param_7 + 4);
      *(undefined4 *)(param_7 + 0x10) = 0xffffffff;
      *(undefined4 *)(param_7 + 0x14) = 0xffffffff;
      *param_7 = 1;
      *(undefined4 *)(param_7 + 0x18) = 0;
      return 0xffffff01;
    }
    bVar16 = true;
    param_4 = local_c;
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
