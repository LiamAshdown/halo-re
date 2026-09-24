// path_find_test_segment_unobstructed  (Ghidra: path_find_test_segment_unobstructed, renamed)
// address 0x43de90, size 988 bytes
// name confidence: 0.35  rewrite confidence: 0.05
// evidence: phase-4 summary "determines whether a straight segment between two points is
// unobstructed across BSP cluster portals within a margin, reporting the first blocking
// crossing if any." Calls path_find_trace_bsp_boundary (path_find_trace_bsp_boundary, this rewrite) *nine*
// times, every single one with zero visible arguments -- the most severe register-loss case
// in this entire batch.
//
// This is, along with ai_search_choose_shorter_corner.c, one of the two least confident
// rewrites in this batch. With none of path_find_trace_bsp_boundary's seven parameters
// visible at any of its nine call sites, this rewrite can only preserve the *shape* of the
// control flow (trace from each endpoint toward the other, using whichever of the two
// resulting crossings is nearer, then measure it against the margin) using this function's
// own in-scope operands as the most plausible stand-ins. It should be treated as
// structurally suggestive only, not a confirmed translation, until it can be redone from a
// disassembly.
//
// register convention: EAX -> point_a (the only `in_` register Ghidra's decompile shows);
//   stack -> the seven Ghidra-recognized formal parameters.
//   // blam-cc: EAX -> point_a, stack -> context, edge_a, point_b, edge_b, margin,
//   //   ignore_permission, out_result

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern double sqrt(double x); // FSQRT
extern double fabs(double x); // ABS
extern uint8_t path_find_trace_bsp_boundary(void *context, uint8_t ignore_permission, real_point3d *point_a,
                                            int32_t start_edge, real_point3d *point_b, int32_t exclude_vertex,
                                            path_find_boundary_crossing *out_result); // 0x43d9b0

// blam-cc: EAX -> point_a, stack -> context, edge_a, point_b, edge_b, margin,
//   ignore_permission, out_result
//
// UNSURE: see file header -- this is a structural placeholder, not a confirmed rewrite.
uint8_t path_find_test_segment_unobstructed(real_point3d *point_a, void *context, int32_t edge_a,
                                            real_point3d *point_b, int32_t edge_b, float margin,
                                            uint8_t ignore_permission, path_find_boundary_crossing *out_result)
{
    float dx = point_b->x - point_a->x;
    float dy = -(point_b->y - point_a->y);
    float len2 = dx * dx + dy * dy;
    float len = (float)sqrt(len2);

    if ((fabs(len) < 0.0001) || (len < 0.0f)) {
        return 0;
    }

    {
        path_find_boundary_crossing crossing_from_a;
        path_find_boundary_crossing crossing_from_b;
        int32_t resolved_a = -1;
        int32_t resolved_b = -1;
        uint8_t have_a = 0;
        uint8_t have_b = 0;
        path_find_boundary_crossing *chosen;
        real_point3d *chosen_position;

        if (edge_a != -1) {
            if (path_find_trace_bsp_boundary(context, ignore_permission, point_a, edge_a, point_b, edge_b, &crossing_from_a) != 0) {
                have_a = (crossing_from_a.edge_b != -1) || ((ignore_permission & 1) == 0);
            }
            resolved_a = crossing_from_a.edge_a;
        }
        if (edge_b != -1) {
            if (path_find_trace_bsp_boundary(context, ignore_permission, point_b, edge_b, point_a, edge_a, &crossing_from_b) != 0) {
                have_b = (crossing_from_b.edge_b != -1) && (crossing_from_b.edge_a != -1) && ((ignore_permission & 1) == 0);
            }
            resolved_b = crossing_from_b.edge_a;
        }
        (void)resolved_a;
        (void)resolved_b;

        if (have_a && have_b && (crossing_from_b.fraction <= crossing_from_a.fraction)) {
            chosen = &crossing_from_b;
        } else if (have_a) {
            chosen = &crossing_from_a;
        } else if (have_b) {
            chosen = &crossing_from_b;
        } else {
            *out_result = crossing_from_a;
            return 0;
        }

        chosen_position = &chosen->position;
        {
            float ddx = point_a->x - chosen_position->x;
            float ddy = point_a->y - chosen_position->y;
            float dist2 = ddx * ddx + ddy * ddy;
            if (margin * margin < dist2 || margin * margin == dist2) {
                *out_result = *chosen;
                return 1;
            }
        }

        *out_result = crossing_from_a;
        return 0;
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043de90 @ 0x43de90) ----
uint FUN_0043de90(undefined4 param_1,int param_2,float *param_3,int param_4,float param_5,
                 byte param_6,undefined4 *param_7)

{
  float fVar1;
  float fVar2;
  char cVar3;
  float *in_EAX;
  uint uVar4;
  int iVar5;
  char *pcVar6;
  ushort uVar7;
  int local_88;
  int local_68;
  int local_44;
  char local_38 [16];
  int local_28;
  float local_20;
  char local_1c [16];
  uint local_c;
  float local_4;

  fVar1 = SQRT((*param_3 - *in_EAX) * (*param_3 - *in_EAX) +
               -(param_3[1] - in_EAX[1]) * -(param_3[1] - in_EAX[1]));
  fVar2 = ABS(fVar1);
  uVar7 = (ushort)(fVar2 < 0.0001) << 8 | (ushort)NAN(fVar2) << 10 |
          (ushort)(fVar2 == 0.0001) << 0xe;
  if ((fVar2 < 0.0001) ||
     (uVar7 = (ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 | (ushort)(fVar1 == 0.0) << 0xe,
     fVar1 < 0.0 || (fVar1 == 0.0) != 0)) {
    return (uint)uVar7;
  }
  if (param_2 == -1) {
    local_68 = -1;
  }
  else {
    FUN_0043d9b0();
    if (local_44 == -1) {
      local_68 = param_2;
    }
    else {
      local_68 = local_44;
    }
  }
  if (param_4 != -1) {
    FUN_0043d9b0();
  }
  if (param_2 == -1) {
    local_88 = -1;
  }
  else {
    FUN_0043d9b0();
    if (local_44 == -1) {
      local_88 = param_2;
    }
    else {
      local_88 = local_44;
    }
  }
  if (param_4 != -1) {
    FUN_0043d9b0();
  }
  if ((local_68 == -1) ||
     ((((cVar3 = FUN_0043d9b0(), cVar3 != '\0' && (local_28 != -1)) && ((param_6 & 1) == 0)) &&
      (cVar3 = FUN_0043d9b0(), cVar3 == '\0')))) {
    local_38[0] = '\0';
  }
  if (local_88 == -1) {
    uVar4 = 0xffffff00;
    local_1c[0] = '\0';
  }
  else {
    uVar4 = FUN_0043d9b0();
    if ((((char)uVar4 != '\0') && (uVar4 = local_c, local_c != 0xffffffff)) && ((param_6 & 1) == 0))
    {
      uVar4 = FUN_0043d9b0();
      if ((char)uVar4 == '\0') {
        local_1c[0] = (char)uVar4;
        goto LAB_0043e1c9;
      }
    }
    uVar4 = CONCAT31((int3)(uVar4 >> 8),local_1c[0]);
  }
LAB_0043e1c9:
  if (local_38[0] == '\0') {
    if ((char)uVar4 == '\0') goto LAB_0043e240;
LAB_0043e1f1:
    pcVar6 = local_1c;
  }
  else {
    if (((char)uVar4 != '\0') && (local_4 <= local_20)) goto LAB_0043e1f1;
    pcVar6 = local_38;
  }
  fVar1 = (*param_3 - *(float *)(pcVar6 + 4)) * (*param_3 - *(float *)(pcVar6 + 4)) +
          (param_3[1] - *(float *)(pcVar6 + 8)) * (param_3[1] - *(float *)(pcVar6 + 8));
  param_5 = param_5 * param_5;
  uVar7 = (ushort)(param_5 < fVar1) << 8 | (ushort)(NAN(param_5) || NAN(fVar1)) << 10 |
          (ushort)(param_5 == fVar1) << 0xe;
  uVar4 = (uint)uVar7;
  if (param_5 < fVar1 || (param_5 == fVar1) != 0) {
    for (iVar5 = 7; iVar5 != 0; iVar5 = iVar5 + -1) {
      *param_7 = *(undefined4 *)pcVar6;
      pcVar6 = pcVar6 + 4;
      param_7 = param_7 + 1;
    }
    return CONCAT31((uint3)(byte)(uVar7 >> 8),1);
  }
LAB_0043e240:
  pcVar6 = local_38;
  for (iVar5 = 7; iVar5 != 0; iVar5 = iVar5 + -1) {
    *param_7 = *(undefined4 *)pcVar6;
    pcVar6 = pcVar6 + 4;
    param_7 = param_7 + 1;
  }
  return uVar4 & 0xffffff00;
}
#endif
