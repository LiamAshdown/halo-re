// path_find_trace_bsp_boundary  (Ghidra: path_find_trace_bsp_boundary, renamed)
// address 0x43d9b0, size 1237 bytes
// name confidence: 0.35  rewrite confidence: 0.1
// evidence: phase-4 summary "recursively traces a straight segment across a BSP cluster's
// connected edges to find the first portal boundary it crosses." Reuses the same
// bsp-pointer-at-+0xb4 / request-block-at-+0x1e8 / permission-bitmap shape as the other
// path_find_trace_* functions in this batch. Calls itself recursively, decal_plane_solve_third_axis and
// collision_bsp_surface_solve_third_axis (both outside this rewrite's range).
//
// Kept close to the Ghidra decompilation and at very low confidence, for the same reasons as
// path_find_trace_cluster_boundary.c and path_find_trace_cluster_boundary_from_vertex.c: the
// boundary-edge/cluster-centroid walk here belongs to an un-established structure_bsp
// sub-layout, and this function's own five callers already show three different reduced
// argument counts for it (this module's established convention when that happens is a
// locally-scoped reduced declaration per call site, which those three files already use).
//
// register convention: stack -> the seven Ghidra-recognized formal parameters.
//   // blam-cc: stack -> context, ignore_permission, point_a, start_edge, point_b,
//   //   exclude_vertex, out_result
// reconciled: R79 0x006b8d78 ai_path_permission_table -> physics.h breakable_surface_globals *breakable_surface_state (the code took the global's ADDRESS; the binary loads the pointer: mov edx,ds:0x6b8d78) and 0x0069e8d8 local_command_list_generation -> global_structure_bsp_index; the row is active[bsp index] (intact breakable surfaces)

// NOTE (orphan pass 4 review, not fixed here): the recursive call at 0x43dc60..0x43dc7e passes a
//   local 0x1c-byte result (esp+0x68) as out_result, not the caller's; the draft passes
//   out_result through. The three decal_plane_solve_third_axis calls were resolved (see the body).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "physics.h"
#include <stdint.h> // uintptr_t

extern double sqrt(double x); // FSQRT
extern breakable_surface_globals *breakable_surface_state; // 0x006b8d78, physics.h
extern int16_t global_structure_bsp_index; // 0x0069e8d8, physics.h (the structure BSP index)
extern real_point3d *ai_bsp_trace_seed_centroid; // 0x006966f8, UNSURE: an initial centroid accumulator seed
extern real_point3d *decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis,
    const real_plane3d *plane, const real_point2d *known);
    // 0x44d860, src/math; blam-cc: stack out, AL component_sign, SI dominant_axis, EBX plane, EDI known

// The plane of collision surface `surface` (0xc-byte surfaces at bsp + 0x40, plane index at +0 with
// the flip bit 31 masked off; 0x10-byte planes at bsp + 0x10), as every decal_plane_solve_third_axis
// call below loads it into EBX.
static const real_plane3d *path_find_surface_plane(uint8_t *bsp, int32_t surface)
{
    uint32_t plane = *(uint32_t *)(*(int32_t *)(bsp + 0x40) + surface * 0xc) & 0x7fffffff;
    return (const real_plane3d *)(uintptr_t)(*(int32_t *)(bsp + 0x10) + plane * 0x10);
}
extern real_point3d *collision_bsp_surface_solve_third_axis(ModelCollisionGeometryBSP *collision_bsp,
    int32_t surface_index, uint8_t component_sign, real_point3d *out, int32_t dominant_axis,
    const real_point2d *known); // 0x501470, src/physics; blam-cc: ECX bsp, EAX surface, ESI axis,
    // EDI known, stack (component_sign, out)

// blam-cc: stack -> context, ignore_permission, point_a, start_edge, point_b, exclude_vertex,
//   out_result
//
// UNSURE: see file header -- kept close to the Ghidra decompilation throughout.
uint8_t path_find_trace_bsp_boundary(void *context, uint8_t ignore_permission, real_point3d *point_a,
                                     int32_t start_edge, real_point3d *point_b, int32_t exclude_vertex,
                                     path_find_boundary_crossing *out_result)
{
    uint8_t *bsp = *(uint8_t **)((uint8_t *)context + 0xb4);
    uint8_t *permission_flags = (uint8_t *)context + 0x1e8;
    uint8_t *permission_row = (uint8_t *)breakable_surface_state->active[global_structure_bsp_index];
    float dx = point_b->x - point_a->x;
    float dy = point_b->y - point_a->y;
    uint8_t recursed = 0;
    int32_t cur_edge = start_edge;

    for (;;) {
        int32_t cluster = cur_edge;
        int32_t first_edge = *(int32_t *)(*(int32_t *)(bsp + 0x40) + cluster * 0xc + 4);
        int32_t edge = first_edge;
        real_point3d centroid = *ai_bsp_trace_seed_centroid;
        int16_t edge_count = 0;
        uint8_t any_edge = 0;
        uint8_t crossed = 0;
        int32_t next_cluster = -1;

        do {
            uint8_t *edge_rec = *(uint8_t **)(bsp + 0x4c) + edge * 0x18;
            uint8_t second_endpoint = (cluster == *(int32_t *)(edge_rec + 0x14));
            float *va = (float *)(*(int32_t *)(edge_rec + (second_endpoint ? 4 : 0)) * 0x10 + *(int32_t *)(bsp + 0x58));
            float *vb = (float *)(*(int32_t *)(edge_rec + (second_endpoint ? 0 : 4)) * 0x10 + *(int32_t *)(bsp + 0x58));
            float ex = va[0] - vb[0];
            float ey = va[1] - vb[1];

            if (*(int32_t *)(edge_rec + 0x10 + (second_endpoint ? 0 : 4)) == exclude_vertex) {
                any_edge = 1;
            }
            centroid.x += vb[0]; centroid.y += vb[1]; centroid.z += vb[2];
            edge_count = edge_count + 1;

            if ((0.0f < ex * (point_b->y - vb[1]) - ey * (point_b->x - vb[0])) &&
                (crossed = 1, 0.0f < (vb[1] - point_a->y) * dx - dy * (vb[0] - point_a->x)) &&
                (0.0f < dy * (va[0] - point_a->x) - (va[1] - point_a->y) * dx)) {
                int32_t neighbor = *(int32_t *)(edge_rec + 0x10 + (second_endpoint ? 0 : 4));
                uint8_t flag = permission_flags[neighbor];
                uint8_t crossable = (flag >> 6) & 1;

                if (ignore_permission == 0) {
                    if (crossable == 0) {
                        goto emit_crossing;
                    }
                    if ((int8_t)flag < 0) {
                        uint8_t perm_index = *(uint8_t *)(*(int32_t *)(bsp + 0x40) + 9 + neighbor * 0xc);
                        crossable = (*(uint32_t *)(permission_row + (perm_index >> 5) * 4) &
                                    (1u << (perm_index & 0x1f))) != 0;
                    }
                }

                if (crossable == 0) {
                emit_crossing:
                    {
                        // 0x43dca8..0x43dd3c: the fraction is computed first; the crossing point
                        // point_a + (dx, dy) * fraction (EDI = esp+0x58) is lifted onto the plane of
                        // the current surface (EBX), solving z
                        float fraction = (((vb[1] - point_a->y) * ex - ey * (va[0] - point_a->x)) -
                                          (float)sqrt(ex * ex + ey * ey) * 0.0078125f) /
                                         (dy * ex - ey * dx);
                        real_point2d crossing;

                        crossing.x = dx * fraction + point_a->x;
                        crossing.y = dy * fraction + point_a->y;
                        decal_plane_solve_third_axis(&out_result->position, 1, 2,
                            path_find_surface_plane(bsp, cluster), &crossing);
                        out_result->edge_a = cluster;
                        out_result->edge_b = edge;
                        out_result->found = 1;
                        out_result->fraction = fraction;
                    }
                    return 1;
                }

                next_cluster = neighbor;
                goto continue_outer;
            }

            edge = *(int32_t *)(edge_rec + 8 + (second_endpoint ? 4 : 0));
        } while (edge != first_edge);

        if (crossed == 0) {
            if ((cluster != exclude_vertex) && (!any_edge) && (exclude_vertex != -1)) {
                // 0x43dde5..0x43de02: ECX = bsp, EAX = start_edge (arg 3), ESI = 2, EDI = point_a
                // (loaded at 0x43d9e6); the earlier rewrite dropped all four
                collision_bsp_surface_solve_third_axis((ModelCollisionGeometryBSP *)bsp, start_edge, 1,
                    &out_result->position, 2, (const real_point2d *)point_a);
                out_result->edge_a = -1;
                out_result->edge_b = -1;
                out_result->found = 1;
                out_result->fraction = 0.0f;
                return 1;
            }
            // 0x43de25..0x43de54: point_b (EDI = EBP, loaded at 0x43da9b) on the current surface
            decal_plane_solve_third_axis(&out_result->position, 1, 2, path_find_surface_plane(bsp, cluster),
                (const real_point2d *)point_b);
            out_result->edge_a = cluster;
            out_result->edge_b = -1;
            out_result->found = 0;
            out_result->fraction = 1.0f;
            return 0;
        }

        centroid.x *= 1.0f / (float)edge_count;
        centroid.y *= 1.0f / (float)edge_count;

        if ((recursed) || (permission_flags[cluster] == 0) ||
            (path_find_trace_bsp_boundary(context, ignore_permission, &centroid, cluster, point_a, -1, out_result) != 0)) {
            // 0x43dd6d..0x43dda1: point_a (EDI, loaded at 0x43d9e6) on the plane of start_edge
            // (stack parameter 4)
            decal_plane_solve_third_axis(&out_result->position, 1, 2, path_find_surface_plane(bsp, start_edge),
                (const real_point2d *)point_a);
            out_result->edge_a = -1;
            out_result->edge_b = -1;
            out_result->found = 1;
            out_result->fraction = 0.0f;
            return 0xffffff01 & 0xff; // matches the original's 0xffffff01 truncated to the observable low byte
        }
        recursed = 1;

    continue_outer:
        cur_edge = next_cluster;
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
