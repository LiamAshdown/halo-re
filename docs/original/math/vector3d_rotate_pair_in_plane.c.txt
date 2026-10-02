// vector3d_rotate_pair_in_plane  (Ghidra: FUN_004cd790; renamed, no established name)
// address 0x4cd790, size 133 bytes
// name confidence: 0.4   rewrite confidence: 0.65
// evidence: math_functions.md: "Rotates two vectors (e.g. a pair of orthonormal basis axes)
//   together within their shared plane by the given sin/cos angle." a' = cos*a - sin*b,
//   b' = sin*a + cos*b -- a standard 2D rotation of the plane the two vectors span. b (ECX) is
//   written first using the still-original a (EAX); a is then written from values of b saved
//   before that write, which the rewrite preserves with explicit temporaries.
// register convention: first vector pointer in EAX (in_EAX), second vector pointer in ECX
//   (in_ECX, written first); sin/cos as the recognized stack parameters (param_1, param_2).
//   // blam-cc: EAX -> a, ECX -> b, stack -> (sin_angle, cos_angle)

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void vector3d_rotate_pair_in_plane(real_vector3d *a, real_vector3d *b, real sin_angle, real cos_angle)
{
    real old_b_i;
    real old_b_j;
    real old_b_k;

    old_b_i = b->i;
    old_b_j = b->j;
    old_b_k = b->k;

    b->i = sin_angle * a->i + cos_angle * b->i;
    b->j = sin_angle * a->j + cos_angle * b->j;
    b->k = sin_angle * a->k + cos_angle * b->k;

    a->i = -old_b_i * sin_angle + cos_angle * a->i;
    a->j = cos_angle * a->j + -old_b_j * sin_angle;
    a->k = -old_b_k * sin_angle + cos_angle * a->k;
}

#if 0
Original Ghidra decompilation (0x4cd790):

void FUN_004cd790(float param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  float *in_ECX;

  fVar1 = *in_ECX;
  fVar2 = in_ECX[1];
  fVar3 = in_ECX[2];
  *in_ECX = param_1 * *in_EAX + param_2 * *in_ECX;
  in_ECX[1] = param_1 * in_EAX[1] + param_2 * in_ECX[1];
  in_ECX[2] = param_1 * in_EAX[2] + param_2 * in_ECX[2];
  *in_EAX = -fVar1 * param_1 + param_2 * *in_EAX;
  in_EAX[1] = param_2 * in_EAX[1] + -fVar2 * param_1;
  in_EAX[2] = -fVar3 * param_1 + param_2 * in_EAX[2];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
