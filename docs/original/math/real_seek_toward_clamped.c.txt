// real_seek_toward_clamped  (Ghidra: FUN_004cf360; renamed per math_types_notes.md)
// address 0x4cf360, size 457 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x4cf360..0x4cf528 (DL wrap, ESI velocity, EDI value); the range comparisons now follow the binary's orderings exactly.)
// evidence: math_functions.md: "Advances a tracked scalar value and its velocity toward a
//   target, clamped (and optionally wrapped) to a [param_4,param_5] range -- used for smoothly
//   seeking a value such as an aim or turn angle." out/phase4/math_types_notes.md item 6: "keeps
//   its state in two separate float pointers (value in EDI, velocity in ESI) plus a
//   [param_4,param_5] range in registers -- there is no struct in the binary. Suggested name
//   real_seek_toward_clamped." A velocity-limited critically-damped seek: if the target is
//   reachable within one step's worth of acceleration, snap directly to it (velocity -> 0);
//   otherwise take a braking-curve step (desired speed = min(max_speed,
//   sqrt(2*accel*|delta|)), sign-matched to the delta, clamped to +-accel per step) and
//   integrate the value, wrapping the delta (and, if requested, the result) modulo the
//   [range_min,range_max] span first.
// register convention: wrap-flag in DL (in_DL, part of the EDX slot), velocity pointer in ESI
//   (unaff_ESI, updated in place), value pointer in EDI (unaff_EDI, updated in place); target,
//   acceleration, max speed and range bounds as the recognized stack parameters (param_1..5).
//   // blam-cc: EDX(DL) -> wrap, ESI -> velocity, EDI -> value, stack -> (target, accel,
//   max_speed, range_min, range_max)
//
// UNSURE, significantly:
// - The first computed `fVar6` in the `wrap` branch (a bit-reinterpretation of the original
//   velocity's upper 16 bits shifted back down) never feeds into any value this function
//   actually writes anywhere; it only ever flows into the `uVar8` half of the CONCAT2 return-
//   value packing that every other function in this batch has shown to be Ghidra failing to
//   decompile a clean boolean. Dropped here as decompiler noise rather than transliterated.
// - Every return statement's *value* is reconstructed rather than transliterated: the two
//   "already within reach" early-outs build their CONCAT with an explicit literal `1` in the
//   low byte (so they return true regardless of the packed comparison flags), while the two
//   returns inside the accelerated-step branch pack only comparison-flag bits at bit position 8
//   and above, leaving the low byte (what a `char`/`bool` caller would actually read) zero. This
//   function is reconstructed as returning 1 when it snapped straight to the target and 0 when
//   it took an accelerated step, on that basis.

// RETURN WIDTH (verified in the disassembly): the success/failure result is written with
// `mov al,cl` at every one of the four exits and never zero-extended, so only AL carries the result and the
// return type is a byte, not an int. Declared uint8_t below; reading it as a 32-bit value
// would pick up whatever the upper 24 bits of EAX happened to hold.

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double fabs(double x); // ABS is a single x87 FABS instruction
extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

uint8_t real_seek_toward_clamped(int wrap, real *velocity, real *value, real target, real accel, real max_speed, real range_min, real range_max)
{
    real old_velocity;
    real delta;
    real max_accel;

    old_velocity = *velocity;
    delta = target - *value;

    if (wrap) {
        real half_range = (range_max - range_min) * 0.5f;
        if (delta <= half_range) {
            if (delta < -half_range) {
                delta = half_range + half_range + delta;
            }
        } else {
            delta = delta - (half_range + half_range);
        }
    }

    max_accel = accel;
    if (max_speed < accel) {
        max_accel = max_speed;
    }

    if (max_accel < (real)fabs((double)(delta - old_velocity))) {
        // accelerated step: braking-curve desired speed, clamped to +-accel, integrated in
        real desired_speed_sq = (accel + accel) * (real)fabs((double)delta);
        real desired_speed = max_speed;
        real velocity_delta;
        real clamped_velocity_delta;
        real new_velocity;
        real new_value;

        if (desired_speed_sq < max_speed * max_speed) {
            desired_speed = (real)sqrt((double)desired_speed_sq);
        }
        if (delta < 0.0f) {
            desired_speed = -desired_speed;
        }

        velocity_delta = desired_speed - old_velocity;
        clamped_velocity_delta = velocity_delta;
        if (accel < (real)fabs((double)velocity_delta)) {
            clamped_velocity_delta = (velocity_delta < 0.0f) ? -accel : accel;
        }

        new_velocity = old_velocity + clamped_velocity_delta;
        new_value = clamped_velocity_delta * 0.5f + new_velocity + *value;

        if (wrap) {
            if (new_value < range_min) {
                new_value = (range_max - range_min) + new_value;
            } else if (new_value > range_max) {
                new_value = new_value - (range_max - range_min);
            }
        }

        *velocity = new_velocity;
        if (new_value < range_min) {
            *value = range_min;
        } else {
            *value = (new_value <= range_max) ? new_value : range_max;
        }
        return 0;
    }

    // within one step's reach: snap straight to the (range-clamped) target
    *velocity = 0.0f;
    if (target < range_min) {
        *value = range_min;
    } else {
        *value = (target <= range_max) ? target : range_max;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4cf360):

undefined4 FUN_004cf360(float param_1,float param_2,float param_3,float param_4,float param_5)

{
  float fVar1;
  bool bVar2;
  bool bVar3;
  bool bVar4;
  float fVar5;
  float fVar6;
  undefined2 uVar8;
  char in_DL;
  float *unaff_ESI;
  float *unaff_EDI;
  float local_c;
  ushort uVar7;

  fVar1 = *unaff_ESI;
  local_c = param_1 - *unaff_EDI;
  fVar6 = fVar1;
  if (in_DL != '\0') {
    fVar5 = (param_5 - param_4) * 0.5;
    uVar7 = (ushort)((uint)fVar1 >> 0x10);
    fVar6 = (float)((uint)uVar7 << 0x10);
    if (local_c <= fVar5) {
      fVar6 = (float)((uint)uVar7 << 0x10);
      if (local_c < -fVar5) {
        local_c = fVar5 + fVar5 + local_c;
      }
    }
    else {
      local_c = local_c - (fVar5 + fVar5);
    }
  }
  uVar8 = (undefined2)((uint)fVar6 >> 0x10);
  fVar6 = param_2;
  if (param_3 < param_2) {
    fVar6 = param_3;
  }
  if (fVar6 < ABS(local_c - fVar1)) {
    fVar6 = (param_2 + param_2) * ABS(local_c);
    if (fVar6 < param_3 * param_3) {
      param_3 = SQRT(fVar6);
    }
    if (local_c < 0.0) {
      param_3 = -param_3;
    }
    param_3 = param_3 - fVar1;
    fVar6 = param_3;
    if ((param_2 < ABS(param_3)) && (fVar6 = param_2, param_3 < 0.0)) {
      fVar6 = -param_2;
    }
    fVar1 = fVar1 + fVar6;
    fVar6 = fVar6 * 0.5 + fVar1 + *unaff_EDI;
    if (in_DL != '\0') {
      if (param_4 <= fVar6) {
        if (param_5 < fVar6) {
          fVar6 = fVar6 - (param_5 - param_4);
        }
      }
      else {
        fVar6 = (param_5 - param_4) + fVar6;
      }
    }
    if (fVar6 >= param_4) {
      fVar5 = fVar6;
      if (fVar6 >= param_5 && (fVar6 == param_5) == 0) {
        fVar5 = param_5;
      }
      *unaff_ESI = fVar1;
      *unaff_EDI = fVar5;
      return CONCAT22(uVar8,(ushort)(fVar6 < param_5) << 8 |
                            (ushort)(NAN(fVar6) || NAN(param_5)) << 10 |
                            (ushort)(fVar6 == param_5) << 0xe);
    }
    *unaff_ESI = fVar1;
    *unaff_EDI = param_4;
    return CONCAT22(uVar8,(ushort)(fVar6 < param_4) << 8 |
                          (ushort)(NAN(fVar6) || NAN(param_4)) << 10 |
                          (ushort)(fVar6 == param_4) << 0xe);
  }
  if (param_1 >= param_4) {
    bVar2 = NAN(param_1);
    bVar3 = param_1 < param_5;
    bVar4 = param_1 == param_5;
    if (bVar3 == 0 && bVar4 == 0) {
      param_1 = param_5;
    }
    *unaff_ESI = 0.0;
    *unaff_EDI = param_1;
    return CONCAT31((int3)(CONCAT22(uVar8,(ushort)bVar3 << 8 | (ushort)(bVar2 || NAN(param_5)) << 10
                                          | (ushort)bVar4 << 0xe) >> 8),1);
  }
  *unaff_ESI = 0.0;
  *unaff_EDI = param_4;
  return CONCAT31((int3)(CONCAT22(uVar8,(ushort)(param_1 < param_4) << 8 |
                                        (ushort)(NAN(param_1) || NAN(param_4)) << 10 |
                                        (ushort)(param_1 == param_4) << 0xe) >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
