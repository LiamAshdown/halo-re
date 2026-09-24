// vector3d_clamp_length  (Ghidra: vector3d_clamp_length, already named)
// address 0x459300, size 83 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: math_functions-style helper; scales a vector down to a maximum length. All 3
//   call sites (0x455401, 0x4c5678-ish and 0x4c6080-ish, grep of out/halo_decompiled.c) invoke
//   this as a bare statement and never use its return value.
// register convention: vector pointer in ECX (in_ECX), maximum length as the recognized stack
//   parameter (param_1).
//   // blam-cc: ECX -> v, stack -> max_length
//
// UNSURE: Ghidra's return type is a packed byte built from leftover FPU-comparison flags
// (NAN/equality test bits) concatenated with a 1/0 "clamped" flag. Every caller invokes this as
// a plain statement and ignores the result, so the return value carries no real information;
// this rewrite makes the function void rather than inventing a meaning for those flag bits.

#include "tags.h"
#include "math.h"

extern double sqrt(double x); // 0x628140 area, x87 FSQRT

// Scales `v` down so its length does not exceed `max_length`; leaves `v` unchanged when it is
// already within bounds.
void vector3d_clamp_length(real_vector3d *v, real max_length)
{
    real length_squared;
    real max_squared;
    real scale;

    length_squared = v->k * v->k + v->j * v->j + v->i * v->i;
    max_squared = max_length * max_length;
    if (length_squared >= max_squared && length_squared != max_squared) {
        scale = max_length / (real)sqrt((double)length_squared);
        v->i = scale * v->i;
        v->j = scale * v->j;
        v->k = scale * v->k;
    }
}

#if 0
Original Ghidra decompilation (0x459300), from tools/pack.py 0x459300:

short vector3d_clamp_length(float param_1)

{
  float fVar1;
  float fVar2;
  byte bVar3;
  float *in_ECX;

  fVar1 = in_ECX[2] * in_ECX[2] + in_ECX[1] * in_ECX[1] + *in_ECX * *in_ECX;
  fVar2 = param_1 * param_1;
  bVar3 = fVar1 < fVar2 | (byte)((ushort)((ushort)(NAN(fVar1) || NAN(fVar2)) << 10) >> 8) |
          (byte)((ushort)((ushort)(fVar1 == fVar2) << 0xe) >> 8);
  if (fVar1 >= fVar2 && (fVar1 == fVar2) == 0) {
    param_1 = param_1 / SQRT(fVar1);
    *in_ECX = param_1 * *in_ECX;
    in_ECX[1] = param_1 * in_ECX[1];
    in_ECX[2] = param_1 * in_ECX[2];
    return CONCAT11(bVar3,1);
  }
  return (ushort)bVar3 << 8;
}
#endif
