// vector3d_rotate_about_axis  (Ghidra: vector3d_rotate_about_axis, already named)
// address 0x4cd820, size 145 bytes
// name confidence: 0.75   rewrite confidence: 0.8
// evidence: math_functions.md: "Rotates the vector at EAX in place about the axis at ECX by the
//   angle whose sine is param_1 and cosine is param_2." Standard Rodrigues' rotation formula:
//   v' = cos*v + (1-cos)*(axis.v)*axis + sin*(axis x v), assuming axis is unit length (callers
//   such as vector3d_randomize_direction @0x4cd1b0 normalize it first).
// register convention: vector pointer in EAX (in_EAX, rotated in place), axis pointer in ECX
//   (in_ECX); sin/cos as the recognized stack parameters (param_1, param_2).
//   // blam-cc: EAX -> v, ECX -> axis, stack -> (sin_angle, cos_angle)

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle)
{
    real parallel_term; // (1 - cos) * dot(v, axis)
    real orig_i;
    real orig_j;

    parallel_term = (1.0f - cos_angle) * (v->i * axis->i + axis->k * v->k + v->j * axis->j);
    orig_i = v->i;
    orig_j = v->j;

    v->i = (parallel_term * axis->i + cos_angle * orig_i) - (orig_j * axis->k - v->k * axis->j) * sin_angle;
    v->j = (parallel_term * axis->j + cos_angle * orig_j) - (axis->i * v->k - orig_i * axis->k) * sin_angle;
    v->k = (cos_angle * v->k + parallel_term * axis->k) - (orig_i * axis->j - orig_j * axis->i) * sin_angle;
}

#if 0
Original Ghidra decompilation (0x4cd820):

void vector3d_rotate_about_axis(float param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float *in_EAX;
  float *in_ECX;

  fVar8 = (1.0 - param_2) * (*in_EAX * *in_ECX + in_ECX[2] * in_EAX[2] + in_EAX[1] * in_ECX[1]);
  fVar1 = *in_ECX;
  fVar2 = *in_EAX;
  fVar3 = in_ECX[2];
  fVar4 = *in_EAX;
  fVar5 = in_ECX[1];
  fVar6 = in_EAX[1];
  fVar7 = *in_ECX;
  *in_EAX = (fVar8 * *in_ECX + param_2 * *in_EAX) -
            (in_EAX[1] * in_ECX[2] - in_EAX[2] * in_ECX[1]) * param_1;
  in_EAX[1] = (fVar8 * in_ECX[1] + param_2 * in_EAX[1]) -
              (fVar1 * in_EAX[2] - fVar2 * fVar3) * param_1;
  in_EAX[2] = (param_2 * in_EAX[2] + fVar8 * in_ECX[2]) - (fVar4 * fVar5 - fVar6 * fVar7) * param_1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
