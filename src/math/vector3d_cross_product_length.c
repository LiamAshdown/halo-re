// vector3d_cross_product_length  (Ghidra: vector3d_cross_product_length, already named)
// address 0x4cd380, size 108 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: math_functions.md: "Returns the magnitude of the cross product of two 3D vectors
//   (e.g. twice the area of the triangle they span)." Straightforward cross product followed by
//   a length computation, matching vector3d_length's shape.
// register convention: first vector pointer in EAX (in_EAX), second vector pointer in ECX
//   (in_ECX), no stack arguments.
//   // blam-cc: EAX -> a, ECX -> b

#include "tags.h"
#include "math.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// Returns the magnitude of the cross product of two 3D vectors.
real vector3d_cross_product_length(real_vector3d *a, real_vector3d *b)
{
    real cross_i;
    real cross_j;
    real cross_k;

    cross_i = a->k * b->j - b->k * a->j;
    cross_j = b->k * a->i - a->k * b->i;
    cross_k = a->i * b->j - b->i * a->j;

    return (real)sqrt((double)(cross_i * cross_i + cross_k * cross_k + cross_j * cross_j));
}

#if 0
Original Ghidra decompilation (0x4cd380):

float10 vector3d_cross_product_length(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  float *in_ECX;

  fVar1 = in_EAX[2] * in_ECX[1] - in_ECX[2] * in_EAX[1];
  fVar2 = in_ECX[2] * *in_EAX - in_EAX[2] * *in_ECX;
  fVar3 = *in_ECX * in_EAX[1] - *in_EAX * in_ECX[1];
  return SQRT((float10)fVar1 * (float10)fVar1 +
              (float10)fVar3 * (float10)fVar3 + (float10)fVar2 * (float10)fVar2);
}
#endif
