// vector3d_projection_band_test  (Ghidra: FUN_004cef90; renamed, no established name)
// address 0x4cef90, size 175 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: math_functions.md: "Performs a bounded-distance/quadratic test likely used to check
//   whether a moving sphere or cone stays within a plane-relative band; low confidence on exact
//   semantics." Projects (point_b - point_a) onto `axis`, checks the projection falls in
//   [-param_1, param_1+param_2], then checks a quadratic combining param_1/param_3/the
//   projection against |point_b-point_a|^2 scaled by param_4^2.
// register convention: axis pointer in EAX (in_EAX), first point pointer in ECX (in_ECX),
//   second point pointer in EDX (in_EDX); the four scalars as the recognized stack parameters
//   (param_1..param_4).
//   // blam-cc: EAX -> axis, ECX -> point_a, EDX -> point_b, stack -> (param_1, param_2,
//   param_3, param_4)
//
//
// VERIFIED against the disassembly at 0x4cef90 (objdump -d -M intel). Every one of Ghidra's
// CONCAT2/NAN(...) expressions here is just its rendering of fnstsw + `test ah,imm`; decoded
// against the x87 condition codes the three gates are exactly
//   0x4cefca  fld p1 / fchs / fcomp proj / test ah,0x41 / jp fail   -> need  -p1 <= proj
//   0x4cefda  fld p1 / fadd p2 / fcomp proj / test ah,0x01 / jne fail -> need p1 + p2 >= proj
//   0x4cf02a  fcompp / test ah,0x41 / jp fail                       -> need rhs <= lhs
// and the only two writes to the result are `mov al,1` (0x4cf033) and `xor al,al` (0x4cf03a),
// so the earlier "failure-path return value could not be decompiled" note is resolved: it is a
// plain byte 0/1. The quadratic itself is transcribed instruction for instruction:
//   lhs = p1*p1 + (2*p1*p3 + proj) * proj          (0x4cefec..0x4cf004)
//   rhs = (dx*dx + dy*dy + dz*dz) * p4 * p4        (0x4cf006..0x4cf026)
// The summation order below follows the FPU's: the projection accumulates dy*j, then dz*k,
// then dx*i, and |d|^2 accumulates dx^2, dy^2, dz^2.
// Its geometric meaning is still open: the shape of lhs (a squared radius plus a term linear in
// the projection) reads like a swept-sphere or cone-vs-segment test, but nothing in this module
// names the four scalars, so they are left as param_1..param_4.

// RETURN WIDTH (verified in the disassembly): the success/failure result is written with
// `mov al,1` / `xor al,al` and never zero-extended, so only AL carries the result and the
// return type is a byte, not an int. Declared uint8_t below; reading it as a 32-bit value
// would pick up whatever the upper 24 bits of EAX happened to hold.

#include "tags.h"
#include "math.h"

uint8_t vector3d_projection_band_test(real_vector3d *axis, real_point3d *point_a, real_point3d *point_b, real radius, real max_distance, real sin_max_angle, real cos_max_angle)
{
    real dx = point_b->x - point_a->x;
    real dy = point_b->y - point_a->y;
    real dz = point_b->z - point_a->z;
    real projection = dy * axis->j + dz * axis->k + dx * axis->i;

    if (-radius <= projection) {
        real upper = radius + max_distance;
        if (upper >= projection) {
            real lhs = radius * radius + (radius * sin_max_angle + radius * sin_max_angle + projection) * projection;
            real rhs = (dx * dx + dy * dy + dz * dz) * cos_max_angle * cos_max_angle;
            if (rhs <= lhs) {
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4cef90):

uint FUN_004cef90(float param_1,float param_2,float param_3,float param_4)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float *in_EAX;
  uint uVar6;
  float *in_ECX;
  float *in_EDX;
  ushort uVar7;

  fVar1 = *in_EDX - *in_ECX;
  fVar2 = in_EDX[1] - in_ECX[1];
  fVar3 = in_EDX[2] - in_ECX[2];
  fVar4 = fVar1 * *in_EAX + fVar3 * in_EAX[2] + fVar2 * in_EAX[1];
  fVar5 = -param_1;
  uVar6 = (uint)(ushort)((ushort)(fVar5 < fVar4) << 8 | (ushort)(NAN(fVar5) || NAN(fVar4)) << 10 |
                        (ushort)(fVar5 == fVar4) << 0xe);
  if (fVar5 < fVar4 != (fVar5 == fVar4)) {
    param_2 = param_1 + param_2;
    uVar6 = (uint)(ushort)((ushort)(param_2 < fVar4) << 8 |
                           (ushort)(NAN(param_2) || NAN(fVar4)) << 10 |
                          (ushort)(param_2 == fVar4) << 0xe);
    if (param_2 >= fVar4) {
      fVar4 = param_1 * param_1 + (param_1 * param_3 + param_1 * param_3 + fVar4) * fVar4;
      param_4 = (fVar3 * fVar3 + fVar2 * fVar2 + fVar1 * fVar1) * param_4 * param_4;
      uVar7 = (ushort)(param_4 < fVar4) << 8 | (ushort)(NAN(param_4) || NAN(fVar4)) << 10 |
              (ushort)(param_4 == fVar4) << 0xe;
      uVar6 = (uint)uVar7;
      if (param_4 < fVar4 != (param_4 == fVar4)) {
        return CONCAT31((uint3)(byte)(uVar7 >> 8),1);
      }
    }
  }
  return uVar6;
}
#endif
