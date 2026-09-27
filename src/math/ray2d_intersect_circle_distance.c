// ray2d_intersect_circle_distance  (Ghidra: FUN_0043c380, renamed)
// address 0x43c380, size 125 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (verified against objdump)
// evidence: out/phase2/ai/08.md; caller FUN_0043c8f0 @0x43c8f0 (confirmed via objdump at the
//   0x43c949 call site: EAX = ECX = &entry[esp+0x1c] (self position doubling as the ray
//   direction at this particular call site), EDX = &entry.position (ebp+8), ESI = &local output
//   float, stack float = a summed radius). The math is the standard 2D ray/circle formula: with
//   L = center - origin and tca = dot(L, direction), origin-inside-circle is fast-pathed to a
//   hit at distance 0, otherwise the discriminant tca^2 - (|L|^2 - radius^2) is the usual
//   perpendicular-distance test and the near hit distance is tca - sqrt(discriminant). This is
//   the 2D sibling of ray_intersect_sphere_distance @0x4ce7d0 (3D, different registers -- LTCG
//   assigns per function, not reused here).
// register convention (confirmed via objdump at 0x43c380 and the 0x43c8f0/0x43c949 call site):
//   direction vector in EAX (in_EAX, only [0]/[1] read), ray origin in ECX (in_ECX), circle
//   center in EDX (in_EDX), output hit-distance pointer in ESI (unaff_ESI, a genuine register
//   parameter, never assigned inside the function); radius as the recognized stack parameter
//   (param_1). The disassembly shows `mov al,0x1` / `xor al,al` on every return path, so only AL
//   carries the result -- the return type is a byte, not an int.
//   // blam-cc: EAX -> direction, ECX -> origin, EDX -> center, ESI -> out_distance, stack -> radius
//
// review (phase 4 math gate): every branch re-traced against objdump 0x43c380..0x43c3fc:
//   0x43c3a4 `test ah,0x41/jne` exits unless tca > 0; 0x43c3c7 `test ah,0x41/jp` takes the
//   0-distance hit for d < 0 or d == 0 (NaN falls through); 0x43c3e8 `test ah,0x1/jne` exits
//   on disc < 0 or NaN. The C below matches all three, including NaN.
// UNSURE: Ghidra could not decompile this function's boolean return cleanly (raw CONCAT/NAN
// register-half packing on two of the three return sites, the same known Ghidra failure mode
// documented on ray_intersects_sphere_test @0x4ce6c0). The branch conditions below are taken
// directly off the original comparisons (`tca >= 0.0 && tca != 0.0` and the `d < 0.0 !=
// (d == 0.0)` parity trick, both of which are false for NaN exactly like the equivalent plain
// C comparisons `tca > 0.0f` / `d <= 0.0f` used here), and the *values* are reconstructed as
// clean 0/1 rather than transliterating the unusable CONCAT/NAN expressions.

#include "tags.h"
#include "math.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// Tests a 2D ray (origin, direction) against a circle (center, radius); on a hit, writes the
// distance to the near intersection to *out_distance and returns true. The ray is only
// considered to hit when it points toward the circle (tca > 0), so an origin already inside the
// circle only counts as a hit (at distance 0) when the direction also satisfies that check.
uint8_t ray2d_intersect_circle_distance(const real_vector2d *direction, const real_point2d *origin,
                                         const real_point2d *center, real *out_distance, real radius)
{
    real dx = center->x - origin->x;
    real dy = center->y - origin->y;
    real tca = dx * direction->i + dy * direction->j;

    if (tca > 0.0f) {
        real d = (dy * dy + dx * dx) - radius * radius;

        if (d <= 0.0f) {
            *out_distance = 0.0f;
            return 1;
        }

        {
            real disc = tca * tca - d;
            if (disc >= 0.0f) {
                *out_distance = tca - (real)sqrt((double)disc);
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x43c380):

uint FUN_0043c380(float param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;
  ushort uVar4;

  fVar1 = *in_EDX - *in_ECX;
  fVar2 = in_EDX[1] - in_ECX[1];
  fVar3 = fVar1 * *in_EAX + fVar2 * in_EAX[1];
  uVar4 = (ushort)(fVar3 < 0.0) << 8 | (ushort)NAN(fVar3) << 10 | (ushort)(fVar3 == 0.0) << 0xe;
  if (fVar3 >= 0.0 && (fVar3 == 0.0) == 0) {
    fVar1 = (fVar2 * fVar2 + fVar1 * fVar1) - param_1 * param_1;
    if (fVar1 < 0.0 != (fVar1 == 0.0)) {
      *unaff_ESI = 0.0;
      return CONCAT31((uint3)(byte)(fVar1 < 0.0 | (byte)((ushort)((ushort)NAN(fVar1) << 10) >> 8) |
                                   (byte)((ushort)((ushort)(fVar1 == 0.0) << 0xe) >> 8)),1);
    }
    fVar1 = fVar3 * fVar3 - fVar1;
    uVar4 = (ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 | (ushort)(fVar1 == 0.0) << 0xe;
    if (fVar1 >= 0.0) {
      *unaff_ESI = fVar3 - SQRT(fVar1);
      return CONCAT31((uint3)(byte)(uVar4 >> 8),1);
    }
  }
  return (uint)uVar4;
}
#endif
