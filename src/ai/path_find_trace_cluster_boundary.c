// path_find_trace_cluster_boundary  (Ghidra: path_find_trace_cluster_boundary, renamed)
// address 0x43d4b0, size 716 bytes
// name confidence: 0.3   rewrite confidence: 0.1
// evidence: phase-4 summary "walks a BSP cluster's boundary edges to find where a proposed
// straight-line move first crosses a blocked or permission boundary." Reads the same
// bsp-pointer-at-+0xb4 and request-block-at-+0x1e8 shape as path_find_gather_adjacent_edges.c
// and path_find_run.c's permission-bitmap test (DAT_006b8d78/DAT_0069e8d8), reused here.
//
// This is a very low confidence, close-to-decompiled rewrite: the boundary-edge table this
// walks (`context->bsp_generation + 0x4c` and `+0x58`, stride 0x18 for edge records and 0x10
// for vertex positions) belongs to the un-established structure_bsp cluster-boundary layout,
// not anything named in types/ai.h, and the control flow (a nested double loop with several
// early-exit conditions on which side of the moving segment a boundary vertex falls) is
// preserved as literally as this rewrite could manage without independent verification.
//
// register convention: EAX -> start_edge, ECX -> context; stack -> start, distance,
//   want_side, ignore_permission, out_result.
//   // blam-cc: EAX -> start_edge, ECX -> context, stack -> start, distance, want_side,
//   //   ignore_permission, out_result

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern double sqrt(double x); // FSQRT
extern double fabs(double x); // ABS
extern uint32_t ai_path_permission_table;    // 0x006b8d78, see path_find_run.c
extern int16_t local_command_list_generation; // 0x0069e8d8, see path_find_run.c

// blam-cc: EAX -> start_edge, ECX -> context, stack -> start, distance, want_side,
//   ignore_permission, out_result
//
// UNSURE: see file header -- kept close to the Ghidra decompilation throughout.
uint8_t path_find_trace_cluster_boundary(int32_t start_edge, void *context, real_point2d *start, float distance,
                                         uint8_t want_side, uint8_t ignore_permission, real_point2d *out_result)
{
    uint8_t *bsp = *(uint8_t **)((uint8_t *)context + 0xb4);
    uint8_t *permission_row = (uint8_t *)&ai_path_permission_table + local_command_list_generation * 0x20 + 1;
    int32_t prev_edge = -1;
    int32_t closed_edge = -1;
    uint8_t *edge_table = *(uint8_t **)(bsp + 0x4c);
    uint8_t *vertex_table = *(uint8_t **)(bsp + 0x58);
    uint8_t *edge = edge_table + start_edge * 0x18;
    int32_t cur_edge = start_edge;

    for (;;) {
        int32_t prev_iter_edge = cur_edge;
        uint8_t flag_byte = *(uint8_t *)(*(int32_t *)(edge + 0x10) + *(int32_t *)((uint8_t *)context + 0x1e8));
        uint8_t use_second = (flag_byte >> 6) & 1;
        float *vertex_a, *vertex_b;
        float ex, ey, ex_neg, elen;
        float side_x, side_y;
        uint8_t bend;
        uint32_t which;
        int32_t next_edge;

        if ((ignore_permission == 0) && use_second && ((int8_t)flag_byte < 0)) {
            uint8_t perm_index = *(uint8_t *)(*(int32_t *)(bsp + 0x40) + *(int32_t *)(edge + 0x10) * 0xc + 9);
            use_second = (*(uint32_t *)(permission_row + (perm_index >> 5) * 4) & (1u << (perm_index & 0x1f))) != 0;
        }

        vertex_a = (float *)(*(int32_t *)(edge + use_second * 4) * 0x10 + vertex_table);
        vertex_b = (float *)(*(int32_t *)(edge + (use_second == 0) * 4) * 0x10 + vertex_table);
        ex = vertex_b[0] - vertex_a[0];
        ey = vertex_b[1] - vertex_a[1];
        ex_neg = -ex;
        elen = (float)sqrt(ey * ey + ex_neg * ex_neg);
        side_x = ex_neg;
        side_y = ey;
        if (0.0001 <= fabs(elen)) {
            elen = 1.0f / elen;
            side_x = elen * ex_neg;
            side_y = elen * ey;
        }

        bend = 0;
        {
            float dx = vertex_a[0] - (side_y * distance + start->x);
            float dy = vertex_a[1] - (side_x * distance + start->y);
            if (((dx * ex + dy * ey < 0.0f) == (want_side != 0)) && (dx * ey - dy * ex < 0.0f)) {
                bend = 1;
            }
        }
        {
            float dx2 = vertex_a[0] - (-distance * side_y + start->x);
            float dy2 = vertex_a[1] - (side_x * -distance + start->y);
            if ((dx2 * ey - dy2 * ex) < 0.0f) {
                bend = 1;
            }
        }
        if (closed_edge == -1) {
            bend = 1;
        }

        which = (bend != use_second) != (want_side != 0);
        next_edge = *(int32_t *)(edge + 4 + which * -4);

        if (next_edge == prev_edge) {
            out_result->x = *(float *)(vertex_table + next_edge * 0x10);
            out_result->y = *(float *)(vertex_table + next_edge * 0x10 + 4);
            return 1;
        }
        if (next_edge == closed_edge) {
            return 0;
        }
        if (closed_edge == -1) {
            closed_edge = next_edge;
        }

        for (;;) {
            uint8_t use_b = (next_edge != *(int32_t *)(edge + 4));
            uint8_t flag2 = *(uint8_t *)(*(int32_t *)(edge + 0x10 + use_b * 4) + *(int32_t *)((uint8_t *)context + 0x1e8));
            uint8_t side2 = (flag2 >> 6) & 1;

            if ((ignore_permission == 0) && side2 && ((int8_t)flag2 < 0)) {
                uint8_t perm_index2 = *(uint8_t *)(*(int32_t *)(bsp + 0x40) +
                                                   *(int32_t *)(edge + 0x10 + use_b * 4) * 0xc + 9);
                side2 = (*(uint32_t *)(permission_row + (perm_index2 >> 5) * 4) & (1u << (perm_index2 & 0x1f))) != 0;
            }

            prev_edge = next_edge;
            if (side2 == want_side) {
                break;
            }

            cur_edge = *(int32_t *)(edge + 8 + use_b * 4);
            edge = edge_table + cur_edge * 0x18;
            if (cur_edge == prev_iter_edge) {
                return 0;
            }
        }
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043d4b0 @ 0x43d4b0) ----
ulonglong FUN_0043d4b0(float *param_1,float param_2,byte param_3,char param_4,undefined4 *param_5)

{
  int iVar1;
  byte bVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 uVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  int iVar12;
  int in_EAX;
  int iVar13;
  bool bVar14;
  int in_ECX;
  uint uVar15;
  float *pfVar16;
  int iVar17;
  bool bVar18;
  int iVar19;
  int iVar20;
  int local_40;
  int local_34;
  int local_30;
  float local_18;

  iVar3 = *(int *)(in_ECX + 0xb4);
  iVar1 = DAT_0069e8d8 * 0x20 + 1 + DAT_006b8d78;
  local_40 = -1;
  local_30 = -1;
  iVar4 = *(int *)(iVar3 + 0x4c);
  iVar19 = in_EAX * 0x18 + iVar4;
  iVar5 = *(int *)(iVar3 + 0x58);
  local_34 = in_EAX;
  while( true ) {
    iVar12 = local_34;
    bVar2 = *(byte *)(*(int *)(iVar19 + 0x10) + *(int *)(in_ECX + 0x1e8));
    bVar14 = (bool)(bVar2 >> 6 & 1);
    if (((param_4 == '\0') && (bVar14 != false)) && ((char)bVar2 < '\0')) {
      bVar2 = *(byte *)(*(int *)(iVar3 + 0x40) + *(int *)(iVar19 + 0x10) * 0xc + 9);
      bVar14 = (*(uint *)(iVar1 + (uint)(bVar2 >> 5) * 4) & 1 << (bVar2 & 0x1f)) != 0;
    }
    pfVar16 = (float *)(*(int *)(iVar19 + (uint)bVar14 * 4) * 0x10 + iVar5);
    bVar18 = false;
    iVar20 = *(int *)(iVar19 + (uint)(bVar14 == false) * 4) * 0x10;
    fVar7 = *(float *)(iVar20 + iVar5) - *pfVar16;
    fVar10 = *(float *)(iVar20 + iVar5 + 4) - pfVar16[1];
    fVar9 = -fVar7;
    fVar8 = SQRT(fVar10 * fVar10 + fVar9 * fVar9);
    local_18 = fVar10;
    if (0.0001 <= ABS(fVar8)) {
      fVar8 = 1.0 / fVar8;
      fVar9 = fVar8 * fVar9;
      local_18 = fVar8 * fVar10;
    }
    fVar8 = *pfVar16 - (local_18 * param_2 + *param_1);
    fVar11 = pfVar16[1] - (fVar9 * param_2 + param_1[1]);
    if ((fVar8 * fVar7 + fVar11 * fVar10 < 0.0 == (bool)param_3) &&
       (fVar8 * fVar10 - fVar11 * fVar7 < 0.0)) {
      bVar18 = true;
    }
    if ((*pfVar16 - (-param_2 * local_18 + *param_1)) * fVar10 -
        (pfVar16[1] - (fVar9 * -param_2 + param_1[1])) * fVar7 < 0.0) {
      bVar18 = true;
    }
    if (local_40 == -1) {
      bVar18 = true;
    }
    uVar15 = (uint)((bVar18 != bVar14) != (bool)param_3);
    iVar20 = *(int *)(iVar19 + 4 + uVar15 * -4);
    iVar17 = 1 - uVar15;
    if (iVar20 == local_30) {
      uVar6 = *(undefined4 *)(iVar5 + iVar20 * 0x10);
      *param_5 = uVar6;
      param_5[1] = *(undefined4 *)(iVar5 + 4 + iVar20 * 0x10);
      return CONCAT44(uVar6,CONCAT31((int3)((uint)param_5 >> 8),1));
    }
    iVar13 = local_40;
    if (iVar20 == local_40) break;
    if (local_40 == -1) {
      local_40 = iVar20;
    }
    while( true ) {
      uVar15 = (uint)(iVar20 != *(int *)(iVar19 + 4));
      iVar17 = *(int *)(iVar19 + 0x10 + uVar15 * 4);
      bVar2 = *(byte *)(iVar17 + *(int *)(in_ECX + 0x1e8));
      bVar14 = (bool)(bVar2 >> 6 & 1);
      if (((param_4 == '\0') && (bVar14 != false)) && ((char)bVar2 < '\0')) {
        bVar2 = *(byte *)(*(int *)(iVar3 + 0x40) + iVar17 * 0xc + 9);
        bVar14 = (*(uint *)(iVar1 + (uint)(bVar2 >> 5) * 4) & 1 << (bVar2 & 0x1f)) != 0;
      }
      local_30 = iVar20;
      if (bVar14 == (bool)param_3) break;
      local_34 = *(int *)(iVar19 + 8 + uVar15 * 4);
      iVar17 = local_34 * 3;
      iVar19 = iVar4 + local_34 * 0x18;
      iVar13 = iVar4;
      if (local_34 == iVar12) goto LAB_0043d74b;
    }
  }
LAB_0043d74b:
  return CONCAT44(iVar17,iVar13) & 0xffffffffffffff00;
}
#endif
