// vector2d_normalize  (Ghidra: vector2d_normalize, already named)
// address 0x4cd2e0, size 62 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: out/phase4/math_types_notes.md real_vector2d section; math_functions.md: "Normalizes
//   a 2D vector in place (no length returned, no epsilon tolerance)" -- unlike
//   vector2d_normalize_with_length @0x4018e0, this guards on exact fVar1 != 0.0, not a tolerance.
// register convention: vector pointer in ECX (in_ECX), no stack arguments.
//   // blam-cc: ECX -> v

#include "tags.h"
#include "math.h"
#include "fn_math.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// Normalizes a 2D vector in place; a zero-length vector is left unchanged.
void vector2d_normalize(real_vector2d *v)
{
    real length_squared;
    real inv_length;

    length_squared = v->j * v->j + v->i * v->i;
    if (length_squared != 0.0f) {
        inv_length = 1.0f / (real)sqrt((double)length_squared);
        v->i = inv_length * v->i;
        v->j = inv_length * v->j;
    }
}

#if 0
Original Ghidra decompilation (0x4cd2e0):

void vector2d_normalize(void)

{
  float fVar1;
  float *in_ECX;

  fVar1 = in_ECX[1] * in_ECX[1] + *in_ECX * *in_ECX;
  if (fVar1 != 0.0) {
    fVar1 = 1.0 / SQRT(fVar1);
    *in_ECX = fVar1 * *in_ECX;
    in_ECX[1] = fVar1 * in_ECX[1];
    return;
  }
  return;
}
#endif
