// quaternion_normalize  (Ghidra: quaternion_normalize, already named)
// address 0x4cdb20, size 111 bytes
// name confidence: 0.85   rewrite confidence: 0.85
// evidence: out/phase4/math_types_notes.md real_quaternion section: writes {0,0,0,1} on a
//   degenerate quaternion, which is what fixes element 3 as the scalar part (w).
// register convention: quaternion pointer in ECX (in_ECX), no stack arguments.
//   // blam-cc: ECX -> q

#include "tags.h"
#include "math.h"
#include "fn_math.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// Normalizes the quaternion in place, falling back to the identity quaternion when its length
// is zero (note: guards on `0.0 < length_squared`, not a tolerance).
void quaternion_normalize(real_quaternion *q)
{
    real length_squared;
    real inv_length;

    length_squared = q->w * q->w + q->k * q->k + q->j * q->j + q->i * q->i;
    if (0.0f < length_squared) {
        inv_length = 1.0f / (real)sqrt((double)length_squared);
        q->i = inv_length * q->i;
        q->j = inv_length * q->j;
        q->k = inv_length * q->k;
        q->w = inv_length * q->w;
        return;
    }
    q->i = 0.0f;
    q->j = 0.0f;
    q->k = 0.0f;
    q->w = 1.0f;
}

#if 0
Original Ghidra decompilation (0x4cdb20):

void quaternion_normalize(void)

{
  float fVar1;
  float *in_ECX;

  fVar1 = in_ECX[3] * in_ECX[3] + in_ECX[2] * in_ECX[2] + in_ECX[1] * in_ECX[1] + *in_ECX * *in_ECX;
  if (0.0 < fVar1) {
    fVar1 = 1.0 / SQRT(fVar1);
    *in_ECX = fVar1 * *in_ECX;
    in_ECX[1] = fVar1 * in_ECX[1];
    in_ECX[2] = fVar1 * in_ECX[2];
    in_ECX[3] = fVar1 * in_ECX[3];
    return;
  }
  *in_ECX = 0.0;
  in_ECX[1] = 0.0;
  in_ECX[2] = 0.0;
  in_ECX[3] = 1.0;
  return;
}
#endif
