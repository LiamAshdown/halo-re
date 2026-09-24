// actor_point_in_directional_lane  (Ghidra: actor_point_in_directional_lane, renamed)
// address 0x414990, size 255 bytes
// name confidence: 0.35  rewrite confidence: 0.4
// evidence: phase-4 summary "tests whether one point lies within a directional lane/cone
// relative to another point and forward direction, using per-side angular thresholds"; the
// only caller in this address range (0x414bd0..0x414c85, inside the not-yet-named function at
// 0x414a90) passes EAX = &prop.unknown_e0, ECX = &actor.position_cache_b (0x5b0), EDX =
// &actor.position_cache_a (0x5a4), and a stack {float min_cos, float side_thresholds[2]} pair
// built just above the call; every Ghidra float compare in the body collapses to a plain
// boolean AND-chain (its NAN-aware condition-code bits never touch the returned byte's low
// bits on any early-exit path), which is how this rewrite reads a five-stage gate instead of
// Ghidra's five near-duplicated x87 comparisons.
// register convention: reconstructed from objdump -d -M intel over 0x414990..0x414a87 and its
// call site at 0x414c5e..0x414c76 (Ghidra dropped every register argument). to_point in EAX,
// forward in ECX, cone_axis in EDX, min_cos_threshold and side_thresholds on the stack
// (side_thresholds pushed first / lower address, min_cos_threshold pushed last / param_1).
// blam-cc: EAX -> to_point, ECX -> forward, EDX -> cone_axis, stack -> min_cos_threshold,
//   stack -> side_thresholds
// UNSURE: only the x/y components of each real_point3d argument are read; z is never
// touched, so this is a horizontal-plane (2D) test. cone_axis is normalized in place as a
// side effect of the length-nonzero check in stage 4 (vector2d_normalize_with_length writes
// through its pointer); to_point and forward are read into locals and are not mutated.
// UNSURE: the caller-side names (position_cache_a/_b, prop.unknown_e0) are not proven to be
// "forward" or "cone axis" in any general sense -- they are simply what this one caller
// passes; kept generic.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern double sqrt(double x); // FSQRT
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, vector in ECX

// blam-cc: EAX -> to_point, ECX -> forward, EDX -> cone_axis, stack -> min_cos_threshold,
//   stack -> side_thresholds
// Five-stage gate over the horizontal (x,y) components of three vectors: to_point must have
// a usable length; its normalized direction dotted with the raw cone_axis must reach
// min_cos_threshold; cone_axis itself must have a usable length (normalized in place here,
// its new direction unused); and the dot of normalized to_point with forward must reach
// whichever of side_thresholds[0]/[1] the sign of to_point x forward selects.
uint8_t actor_point_in_directional_lane(real_point3d *to_point, real_point3d *forward, real_point3d *cone_axis,
                                         float min_cos_threshold, float side_thresholds[2])
{
    float ax, ay;
    float length;
    float dot_axis;
    float axis_length;
    float dot_forward;
    float cross;
    float threshold;

    ax = to_point->x;
    ay = to_point->y;
    length = (float)sqrt((double)(ay * ay + ax * ax));
    if (length < 0.0001f) {
        return 0;
    }
    ax = ax * (1.0f / length);
    ay = ay * (1.0f / length);

    // UNSURE: Ghidra re-checks length (the same sqrt result, always >= 0) against 0.0 here;
    // it is unreachable given the 0.0001f check above and is kept only as a control-flow note.

    dot_axis = ax * cone_axis->x + ay * cone_axis->y;
    if (dot_axis < min_cos_threshold) {
        return 0;
    }

    axis_length = vector2d_normalize_with_length((real_vector2d *)cone_axis);
    if (axis_length < 0.0f) {
        return 0;
    }

    dot_forward = forward->y * ay + forward->x * ax;
    cross = ax * forward->y - forward->x * ay;
    threshold = side_thresholds[(cross > 0.0f) ? 1 : 0];
    if (dot_forward < threshold) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x414990):

uint FUN_00414990(float param_1,int param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float10 fVar5;
  float fVar6;
  float fVar7;
  float *in_EAX;
  uint uVar8;
  float *in_ECX;
  float *in_EDX;
  ushort uVar9;
  float10 fVar10;

  fVar1 = *in_EAX;
  fVar2 = in_EAX[1];
  fVar3 = *in_ECX;
  fVar4 = in_ECX[1];
  fVar6 = SQRT(fVar2 * fVar2 + fVar1 * fVar1);
  fVar7 = ABS(fVar6);
  uVar8 = (uint)(ushort)((ushort)(fVar7 < 0.0001) << 8 | (ushort)NAN(fVar7) << 10 |
                        (ushort)(fVar7 == 0.0001) << 0xe);
  if (fVar7 >= 0.0001) {
    fVar1 = fVar1 * (1.0 / fVar6);
    fVar2 = (1.0 / fVar6) * fVar2;
    uVar8 = (uint)(ushort)((ushort)(fVar6 < 0.0) << 8 | (ushort)NAN(fVar6) << 10 |
                          (ushort)(fVar6 == 0.0) << 0xe);
    if (fVar6 >= 0.0 && (fVar6 == 0.0) == 0) {
      fVar6 = fVar1 * *in_EDX + fVar2 * in_EDX[1];
      uVar8 = (uint)(ushort)((ushort)(fVar6 < param_1) << 8 |
                             (ushort)(NAN(fVar6) || NAN(param_1)) << 10 |
                            (ushort)(fVar6 == param_1) << 0xe);
      if (fVar6 >= param_1 && (fVar6 == param_1) == 0) {
        fVar10 = (float10)vector2d_normalize_with_length();
        fVar5 = (float10)0.0;
        uVar8 = (uint)(ushort)((ushort)(fVar10 < fVar5) << 8 |
                               (ushort)(NAN(fVar10) || NAN(fVar5)) << 10 |
                              (ushort)(fVar10 == fVar5) << 0xe);
        if (fVar10 >= fVar5 && (fVar10 == fVar5) == 0) {
          fVar6 = fVar4 * fVar2 + fVar3 * fVar1;
          fVar1 = *(float *)(param_2 + (uint)(0.0 < fVar1 * fVar4 - fVar3 * fVar2) * 4);
          uVar9 = (ushort)(fVar6 < fVar1) << 8 | (ushort)(NAN(fVar6) || NAN(fVar1)) << 10 |
                  (ushort)(fVar6 == fVar1) << 0xe;
          uVar8 = (uint)uVar9;
          if (fVar6 >= fVar1 && (fVar6 == fVar1) == 0) {
            return CONCAT31((uint3)(byte)(uVar9 >> 8),1);
          }
        }
      }
    }
  }
  return uVar8;
}

Disassembly cross-check for the call site (objdump -d -M intel bin/halo.exe, 0x414c5e..0x414c7d):

00414c5e: lea eax,[esi+0xe0]      ; esi = prop pointer, eax = &prop.unknown_e0
00414c64: lea ecx,[edi+0x5b0]     ; edi = actor pointer, ecx = &actor.position_cache_b
00414c6a: lea edx,[edi+0x5a4]     ; edx = &actor.position_cache_a
00414c70: push ebp                ; ebp = float min_cos_threshold (param_1, pushed last)
00414c71: call 0x414990
#endif
