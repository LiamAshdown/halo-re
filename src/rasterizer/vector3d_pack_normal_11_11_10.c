// vector3d_pack_normal_11_11_10  (Ghidra: FUN_005132d0, unnamed; named from
// types/rasterizer.h's lens_flare_instance.packed_direction comment, which already calls this
// function by this name as the pack side of vector3d_unpack_normal_11_11_10 @0x513400)
// address 0x5132d0, size 291 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: inverse of vector3d_unpack_normal_11_11_10 (same 11:11:10 bit layout, same 1023.5 /
//   511.5 scale factors used by the unpack side's 9.536743e-07 == 1/1023.5/1024 style constants).
// register convention: source vector in unaff_ESI (unresolved register read, callee-saved
//   pointer carried over from the caller). // blam-cc: unaff_ESI -> direction
// UNSURE: floor's exact effect. It is called elsewhere in the tree with both
//   "extern double floor(double value); // CRT helper" and a lone guess of floor(); here
//   it is applied to value*1023.5 (no +0.5) and the result is immediately re-rounded by the
//   caller's ROUND(), so whatever it does the net effect the original code computes is preserved
//   verbatim below rather than assumed to be floor().

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double floor(double x); // 0x00623e40

static float clamp_unit(float v)
{
    if (v < -1.0f) return -1.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// blam-cc: unaff_ESI -> direction
// Packs a unit-ish 3 component vector into an 11:11:10 bit signed encoding: each component is
// clamped to [-1,1], scaled (1023.5 for the two 11 bit components, 511.5 for the 10 bit one),
// passed through floor and rounded to an integer, then the three integers are packed
// low-to-high as (z << 22) | (y << 11) | x, each masked to its field width.
uint32_t vector3d_pack_normal_11_11_10(real_vector3d *direction)
{
    float cx, cy, cz;
    int32_t xi, yi, zi;

    cx = clamp_unit(direction->i);
    xi = (int32_t)(float)floor((double)(cx * 1023.5f)); // floor, stored as a float, then FISTP of that whole number

    cy = clamp_unit(direction->j);
    yi = (int32_t)(float)floor((double)(cy * 1023.5f)); // floor, stored as a float, then FISTP of that whole number

    cz = clamp_unit(direction->k);
    zi = (int32_t)(float)floor((double)(cz * 511.5f)); // floor, stored as a float, then FISTP of that whole number

    return (uint32_t)(((zi << 0xb | (yi & 0x7ff)) << 0xb) | (xi & 0x7ff));
}

#if 0
Original Ghidra decompilation (0x5132d0):

uint FUN_005132d0(void)

{
  float fVar1;
  float *unaff_ESI;
  float10 fVar2;
  float10 fVar3;
  float10 fVar4;

  if (-1.0 <= *unaff_ESI) {
    if (*unaff_ESI <= 1.0) {
      fVar1 = *unaff_ESI;
    }
    else {
      fVar1 = 1.0;
    }
  }
  else {
    fVar1 = -1.0;
  }
  fVar2 = (float10)FUN_00623e40((double)(fVar1 * 1023.5));
  if (-1.0 <= unaff_ESI[1]) {
    if (unaff_ESI[1] <= 1.0) {
      fVar1 = unaff_ESI[1];
    }
    else {
      fVar1 = 1.0;
    }
  }
  else {
    fVar1 = -1.0;
  }
  fVar3 = (float10)FUN_00623e40((double)(fVar1 * 1023.5));
  if (-1.0 <= unaff_ESI[2]) {
    if (unaff_ESI[2] <= 1.0) {
      fVar1 = unaff_ESI[2];
    }
    else {
      fVar1 = 1.0;
    }
  }
  else {
    fVar1 = -1.0;
  }
  fVar4 = (float10)FUN_00623e40((double)(fVar1 * 511.5));
  return ((int)ROUND((float)fVar4) << 0xb | (int)ROUND((float)fVar3) & 0x7ffU) << 0xb |
         (int)ROUND((float)fVar2) & 0x7ffU;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
