// path_find_test_segment_unobstructed  (Ghidra: path_find_test_segment_unobstructed, renamed)
// address 0x43de90, size 988 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// VERIFIED against disassembly 0x43de90..0x43e26b (2026-09-30); rewritten from it. (The 1/length normal is kept in x87 extended
//   precision by the original; here it is float.) EAX = point A, EBX = the path find map, stack: ignore_permission,
//   surface A, point B, surface B, radius, flags, out_result. Tests the 2D segment A -> B for a body of the given
//   radius by tracing (path_find_trace_bsp_boundary) the two parallel segments offset by +/- radius along the
//   segment's normal (-dy, dx)/|AB|. Each side first moves its end points sideways by tracing from A (and from B)
//   to the offset point on their surfaces (a missing surface, -1, leaves that side's start unset); then traces
//   offset A -> offset B (target = offset B's surface). A side that hits a wall still counts as clear when the hit
//   has a surface, flags bit 0 is clear and the hit point reaches B directly (a trace from the hit to B, target
//   surface B). With both sides blocked the earlier hit (smaller fraction, ties to the -radius side) is taken;
//   a hit within radius of B is ignored. Blocked: the hit goes to out_result and 1 is returned; clear: the +radius
//   side's result is copied and 0 returned. A degenerate segment (|AB| < 0.0001) returns 0 without writing.
// blam-cc: EBX -> map, EAX -> point_a, stack -> ignore_permission, surface_a, point_b, surface_b, radius, flags,
//   out_result

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>

extern double sqrt(double x); // FSQRT
extern double fabs(double x); // FABS
extern uint8_t path_find_trace_bsp_boundary(void *map, uint8_t ignore_permission, real_point3d *start,
    int32_t start_surface, real_point3d *end, int32_t target_surface,
    path_find_boundary_crossing *out_result); // 0x43d9b0, stack

// 0x43df10..0x43e0eb: move an end point sideways onto the offset line, keeping it on the map.
static int32_t path_find_offset_end(void *map, uint8_t ignore_permission, real_point3d *point, int32_t surface,
    float ox, float oy, real_point3d *out_point)
{
    path_find_boundary_crossing scratch;

    out_point->x = ox + point->x;
    out_point->y = oy + point->y;
    if (surface == -1) {
        return -1;
    }
    path_find_trace_bsp_boundary(map, ignore_permission, point, surface, out_point, -1, &scratch);
    out_point->x = scratch.position.x;
    out_point->y = scratch.position.y;
    return (scratch.edge_a == -1) ? surface : scratch.edge_a;
}

// 0x43e0ee / 0x43e154: one side of the body; returns its found flag.
static uint8_t path_find_trace_side(void *map, uint8_t ignore_permission, real_point3d *from, int32_t from_surface,
    real_point3d *to, int32_t to_surface, real_point3d *point_b, int32_t surface_b, uint8_t flags,
    path_find_boundary_crossing *result)
{
    path_find_boundary_crossing scratch;

    if (from_surface == -1) {
        result->found = 0;
        return 0;
    }
    if (path_find_trace_bsp_boundary(map, ignore_permission, from, from_surface, to, to_surface, result) != 0 &&
        result->edge_a != -1 && (flags & 1) == 0 &&
        path_find_trace_bsp_boundary(map, ignore_permission, &result->position, result->edge_a, point_b, surface_b,
            &scratch) == 0) {
        result->found = 0;
    }
    return result->found;
}

uint8_t path_find_test_segment_unobstructed(void *map, real_point3d *point_a, uint8_t ignore_permission,
    int32_t surface_a, real_point3d *point_b, int32_t surface_b, float radius, uint8_t flags,
    path_find_boundary_crossing *out_result)
{
    float dx = point_b->x - point_a->x;
    float ny_neg = -(point_b->y - point_a->y);
    float length = (float)sqrt(ny_neg * ny_neg + dx * dx);
    float nx;
    float ny;
    real_point3d a_plus;
    real_point3d b_plus;
    real_point3d a_minus;
    real_point3d b_minus;
    int32_t a_plus_surface;
    int32_t b_plus_surface;
    int32_t a_minus_surface;
    int32_t b_minus_surface;
    path_find_boundary_crossing plus_result;
    path_find_boundary_crossing minus_result;
    path_find_boundary_crossing *chosen;
    uint8_t plus_hit;
    uint8_t minus_hit;

    if ((float)fabs(length) < 0.0001f || !(length > 0.0f)) {
        return 0; // 0x43e25e: degenerate
    }
    nx = ny_neg * (1.0f / length);
    ny = dx * (1.0f / length);
    memset(&plus_result, 0, sizeof(plus_result));
    memset(&minus_result, 0, sizeof(minus_result));
    a_plus = *point_a;
    b_plus = *point_b;
    a_minus = *point_a;
    b_minus = *point_b;
    a_plus_surface = path_find_offset_end(map, ignore_permission, point_a, surface_a, nx * radius, ny * radius, &a_plus);
    b_plus_surface = path_find_offset_end(map, ignore_permission, point_b, surface_b, nx * radius, ny * radius, &b_plus);
    a_minus_surface = path_find_offset_end(map, ignore_permission, point_a, surface_a, nx * -radius, ny * -radius,
        &a_minus);
    b_minus_surface = path_find_offset_end(map, ignore_permission, point_b, surface_b, nx * -radius, ny * -radius,
        &b_minus);

    plus_hit = path_find_trace_side(map, ignore_permission, &a_plus, a_plus_surface, &b_plus, b_plus_surface, point_b,
        surface_b, flags, &plus_result);
    minus_hit = path_find_trace_side(map, ignore_permission, &a_minus, a_minus_surface, &b_minus, b_minus_surface,
        point_b, surface_b, flags, &minus_result);

    if (plus_hit) {
        chosen = (minus_hit && !(plus_result.fraction < minus_result.fraction)) ? &minus_result : &plus_result;
    } else if (minus_hit) {
        chosen = &minus_result;
    } else {
        *out_result = plus_result;
        return 0;
    }
    {
        float ex = point_b->x - chosen->position.x;
        float ey = point_b->y - chosen->position.y;

        if (radius * radius > ey * ey + ex * ex) {
            *out_result = plus_result; // 0x43e240: a hit within radius of B does not count
            return 0;
        }
    }
    *out_result = *chosen;
    return 1;
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
