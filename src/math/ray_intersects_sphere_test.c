// ray_intersects_sphere_test  (Ghidra: ray_intersects_sphere_test, already named)
// address 0x4ce6c0, size 263 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x4ce6c0..0x4ce7c6 (EAX centre, ECX origin, EDX direction).)
// evidence: math_functions.md: "Boolean-only ray/sphere intersection test against a sphere of
//   radius param_1, returning whether the ray hits without computing hit details." Structurally
//   mirrors ray_intersects_sphere @0x4ce3a0 (origin-inside-sphere fast path, then a quadratic
//   test), reformulated to decide the near-root-in-range test by squaring instead of an
//   explicit sqrt division, which is presumably why this is a separate function from the
//   distance-returning versions.
// register convention: origin pointer in EAX (in_EAX), sphere-center pointer in ECX (in_ECX),
//   direction pointer in EDX (in_EDX); radius as the recognized stack parameter (param_1).
//   // blam-cc: EAX -> origin, ECX -> center, EDX -> direction, stack -> radius
//
// UNSURE, significantly: Ghidra could not decompile this function's actual boolean return value
// -- it prints raw CONCAT2/CONCAT3 register-half packing and NAN()-flag pseudo-expressions
// instead of a clean 0/1 (a known Ghidra failure mode for bit-trick boolean returns). The
// control flow below (branch conditions, which paths return 1 vs fall through to 0) is read
// directly off the original; the *values themselves* are reconstructed as clean int 0/1 rather
// than transliterating the unusable CONCAT/NAN expressions. Also UNSURE: the `b < 0.0` branch
// condition is taken exactly as decompiled even though, by the usual closest-approach argument,
// dot(direction, center-origin) < 0 reads as "ray moving away from the sphere" -- the opposite
// of when you would expect to compute the discriminant.

// RETURN WIDTH (verified in the disassembly): the success/failure result is written with
// `xor al,al` on one path (and Ghidra types the call site in
// segment3d_within_radius_of_segment as `char`), so despite the `mov eax,1`/`xor eax,eax` on the
// other paths only AL is guaranteed, so only AL carries the result and the
// return type is a byte, not an int. Declared uint8_t below; reading it as a 32-bit value
// would pick up whatever the upper 24 bits of EAX happened to hold.

#include "tags.h"
#include "math.h"
#include "fn_math.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// FIXED (call sites 0x50562f, 0x4ce6c0 callers): EAX (first parameter) is the SPHERE CENTRE and ECX the ray/segment
// origin at every original call site; the parameters were named the other way round (the body was already right).
uint8_t ray_intersects_sphere_test(real_point3d *center, real_point3d *origin, real_vector3d *direction, real radius)
{
    real from_center_i;
    real from_center_j;
    real from_center_k;
    real c;

    from_center_i = origin->x - center->x;
    from_center_j = origin->y - center->y;
    from_center_k = origin->z - center->z;
    c = (from_center_i * from_center_i + from_center_j * from_center_j + from_center_k * from_center_k) - radius * radius;

    if (c < 0.0f) {
        return 1;
    }

    {
        real b = direction->i * from_center_i + direction->j * from_center_j + direction->k * from_center_k;
        if (b < 0.0f) {
            real a = direction->i * direction->i + direction->j * direction->j + direction->k * direction->k;
            real discriminant = b * b - a * c;
            if (0.0f < discriminant) {
                real near_no_sqrt = -a - b;
                if (0.0f <= near_no_sqrt) {
                    return (discriminant <= near_no_sqrt * near_no_sqrt) ? 0 : 1;
                }
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ce6c0):

undefined4 ray_intersects_sphere_test(float param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float *in_EAX;
  undefined4 uVar8;
  undefined2 uVar9;
  float *in_ECX;
  float *in_EDX;
  ushort uVar10;

  fVar4 = *in_ECX - *in_EAX;
  fVar6 = in_ECX[1] - in_EAX[1];
  fVar7 = in_ECX[2] - in_EAX[2];
  fVar5 = (fVar4 * fVar4 + fVar6 * fVar6 + fVar7 * fVar7) - param_1 * param_1;
  uVar8 = CONCAT22((short)((uint)in_EAX >> 0x10),
                   (ushort)(fVar5 < 0.0) << 8 | (ushort)NAN(fVar5) << 10 |
                   (ushort)(fVar5 == 0.0) << 0xe);
  if (fVar5 < 0.0) {
LAB_004ce701:
    return CONCAT31((int3)((uint)uVar8 >> 8),1);
  }
  fVar1 = *in_EDX;
  fVar2 = in_EDX[1];
  fVar3 = in_EDX[2];
  fVar4 = fVar1 * fVar4 + fVar2 * fVar6 + fVar3 * fVar7;
  uVar10 = (ushort)(fVar4 < 0.0) << 8 | (ushort)NAN(fVar4) << 10 | (ushort)(fVar4 == 0.0) << 0xe;
  uVar9 = (undefined2)((uint)fVar1 >> 0x10);
  if (fVar4 < 0.0) {
    fVar6 = fVar1 * fVar1 + fVar2 * fVar2 + fVar3 * fVar3;
    fVar5 = fVar4 * fVar4 - fVar6 * fVar5;
    uVar10 = (ushort)(fVar5 < 0.0) << 8 | (ushort)NAN(fVar5) << 10 | (ushort)(fVar5 == 0.0) << 0xe;
    if (fVar5 < 0.0 == (fVar5 == 0.0)) {
      fVar4 = -fVar6 - fVar4;
      uVar8 = CONCAT22(uVar9,(ushort)(fVar4 < 0.0) << 8 | (ushort)NAN(fVar4) << 10 |
                             (ushort)(fVar4 == 0.0) << 0xe);
      if (fVar4 >= 0.0) {
        if (fVar5 <= fVar4 * fVar4) {
          return 0;
        }
        return 1;
      }
      goto LAB_004ce701;
    }
  }
  return CONCAT22(uVar9,uVar10);
}
#endif
