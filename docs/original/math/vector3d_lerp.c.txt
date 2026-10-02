// vector3d_lerp  (Ghidra: vector3d_lerp, already named)
// address 0x4cd8c0, size 57 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: math_functions.md: "Linearly interpolates componentwise between two 3D vectors
//   (ECX, EDX) by factor param_1, writing the result to EAX."
// register convention: output vector pointer in EAX (in_EAX), first vector pointer in ECX
//   (in_ECX), second vector pointer in EDX (in_EDX); factor as the recognized stack parameter
//   (param_1).
//   // blam-cc: EAX -> out, ECX -> a, EDX -> b, stack -> t

#include "tags.h"
#include "math.h"

// out = t*a + (1-t)*b
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void vector3d_lerp(real_vector3d *out, real_vector3d *a, real_vector3d *b, real t)
{
    real one_minus_t;

    one_minus_t = 1.0f - t;
    out->i = t * a->i + one_minus_t * b->i;
    out->j = t * a->j + one_minus_t * b->j;
    out->k = t * a->k + one_minus_t * b->k;
}

#if 0
Original Ghidra decompilation (0x4cd8c0):

void vector3d_lerp(float param_1)

{
  float fVar1;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;

  fVar1 = 1.0 - param_1;
  *in_EAX = param_1 * *in_ECX + fVar1 * *in_EDX;
  in_EAX[1] = param_1 * in_ECX[1] + fVar1 * in_EDX[1];
  in_EAX[2] = param_1 * in_ECX[2] + fVar1 * in_EDX[2];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
