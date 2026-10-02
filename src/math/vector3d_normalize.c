// vector3d_normalize  (Ghidra: vector3d_normalize, already named)
// address 0x4cd320, size 81 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: out/phase4/math_types_notes.md real_vector3d section; same shape as
//   vector2d_normalize @0x4cd2e0 for 3 components (exact-zero guard, no epsilon, no length
//   returned).
// register convention: vector pointer in ECX (in_ECX), no stack arguments.
//   // blam-cc: ECX -> v

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// Normalizes a 3D vector in place; a zero-length vector is left unchanged.
void vector3d_normalize(real_vector3d *v)
{
    real length_squared;
    real inv_length;

    length_squared = v->k * v->k + v->j * v->j + v->i * v->i;
    if (length_squared != 0.0f) {
        inv_length = 1.0f / (real)sqrt((double)length_squared);
        v->i = inv_length * v->i;
        v->j = inv_length * v->j;
        v->k = inv_length * v->k;
    }
}

#if 0
Original Ghidra decompilation (0x4cd320):

void vector3d_normalize(void)

{
  float fVar1;
  float *in_ECX;

  fVar1 = in_ECX[2] * in_ECX[2] + in_ECX[1] * in_ECX[1] + *in_ECX * *in_ECX;
  if (fVar1 != 0.0) {
    fVar1 = 1.0 / SQRT(fVar1);
    *in_ECX = fVar1 * *in_ECX;
    in_ECX[1] = fVar1 * in_ECX[1];
    in_ECX[2] = fVar1 * in_ECX[2];
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
