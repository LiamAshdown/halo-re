// quaternion_lerp  (Ghidra: quaternion_lerp, already named)
// address 0x4cdcc0, size 117 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: math_functions.md: "Performs a shortest-path linear (nlerp-style) interpolation
//   between two quaternions (ECX, EDX) by factor param_1, writing the result to ESI." The
//   1-t complement is computed from the original t *before* the shortest-path sign flip, so it
//   is kept as a separate variable rather than folded into `1 - signed_t`.
// register convention: first quaternion pointer in ECX (in_ECX), second quaternion pointer in
//   EDX (in_EDX), output pointer in ESI (unaff_ESI); t as the recognized stack parameter
//   (param_1).
//   // blam-cc: ECX -> a, EDX -> b, ESI -> out, stack -> t

#include "tags.h"
#include "math.h"

// out = signed_t*a + (1-t)*b, where signed_t is +-t chosen so the interpolation takes the
// shorter path between the two quaternions (dot(a,b) >= 0).
void quaternion_lerp(real_quaternion *a, real_quaternion *b, real_quaternion *out, real t)
{
    real one_minus_t;
    real signed_t;

    one_minus_t = 1.0f - t;
    signed_t = t;
    if (a->w * b->w + b->i * a->i + a->j * b->j + a->k * b->k < 0.0f) {
        signed_t = -t;
    }

    out->i = signed_t * a->i + one_minus_t * b->i;
    out->j = one_minus_t * b->j + signed_t * a->j;
    out->k = one_minus_t * b->k + signed_t * a->k;
    out->w = one_minus_t * b->w + signed_t * a->w;
}

#if 0
Original Ghidra decompilation (0x4cdcc0):

void quaternion_lerp(float param_1)

{
  float fVar1;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;

  fVar1 = 1.0 - param_1;
  if (in_ECX[3] * in_EDX[3] + *in_EDX * *in_ECX + in_ECX[1] * in_EDX[1] + in_ECX[2] * in_EDX[2] <
      0.0) {
    param_1 = -param_1;
  }
  *unaff_ESI = param_1 * *in_ECX + fVar1 * *in_EDX;
  unaff_ESI[1] = fVar1 * in_EDX[1] + param_1 * in_ECX[1];
  unaff_ESI[2] = fVar1 * in_EDX[2] + param_1 * in_ECX[2];
  unaff_ESI[3] = fVar1 * in_EDX[3] + param_1 * in_ECX[3];
  return;
}
#endif
