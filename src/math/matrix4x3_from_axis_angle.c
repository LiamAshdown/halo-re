// matrix4x3_from_axis_angle  (Ghidra: matrix4x3_from_axis_angle, already named)
// address 0x4cb880, size 230 bytes
// name confidence: 0.65   rewrite confidence: 0.75
// evidence: out/phase4/math_functions.md ("Builds a matrix4x3 rotation from a rotation axis
//   and precomputed sin/cos of the angle (Rodrigues' formula)"); types/math.h real_matrix4x3
//   section confirms scale=1 and zeroed translation are written here. Field-by-field expansion
//   of the standard Rodrigues axis-angle rotation matrix.
// register convention: output matrix in EAX (in_EAX), axis vector in ECX (in_ECX); sin/cos of
//   the angle as the two recognized stack parameters (param_1, param_2).
//   // blam-cc: EAX -> out, ECX -> axis, stack -> (sin_angle, cos_angle)

#include "tags.h"
#include "math.h"

// Builds a matrix4x3 rotation from a rotation axis and precomputed sin/cos of the angle
// (Rodrigues' formula).
void matrix4x3_from_axis_angle(real_matrix4x3 *out, real_vector3d *axis, real sin_angle, real cos_angle)
{
    real one_minus_cos;
    real ii, jj, kk;
    real ij, ik, jk;

    one_minus_cos = 1.0f - cos_angle;
    ii = axis->i * axis->i;
    jj = axis->j * axis->j;
    kk = axis->k * axis->k;
    ij = axis->j * axis->i * one_minus_cos;
    ik = axis->k * axis->i * one_minus_cos;
    jk = axis->k * axis->j * one_minus_cos;

    out->scale = 1.0f;
    out->forward.i = (1.0f - ii) * cos_angle + ii;
    out->forward.j = ij + sin_angle * axis->k;
    out->left.i = ij - sin_angle * axis->k;
    out->left.j = (1.0f - jj) * cos_angle + jj;
    out->forward.k = ik - sin_angle * axis->j;
    out->up.i = ik + sin_angle * axis->j;
    out->up.k = (1.0f - kk) * cos_angle + kk;
    out->position.z = 0.0f;
    out->position.y = 0.0f;
    out->position.x = 0.0f;
    out->left.k = jk + sin_angle * axis->i;
    out->up.j = jk - sin_angle * axis->i;
}

#if 0
Original Ghidra decompilation (0x4cb880):

void matrix4x3_from_axis_angle(float param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  undefined4 *in_EAX;
  float *in_ECX;

  fVar4 = *in_ECX * *in_ECX;
  fVar5 = in_ECX[1] * in_ECX[1];
  fVar6 = in_ECX[2] * in_ECX[2];
  fVar1 = *in_ECX;
  fVar2 = in_ECX[1];
  fVar3 = in_ECX[2];
  *in_EAX = 0x3f800000;
  in_EAX[1] = (1.0 - fVar4) * param_2 + fVar4;
  fVar7 = 1.0 - param_2;
  fVar4 = in_ECX[1] * *in_ECX * fVar7;
  in_EAX[2] = fVar4;
  in_EAX[4] = fVar4 - param_1 * fVar3;
  in_EAX[2] = param_1 * fVar3 + (float)in_EAX[2];
  in_EAX[5] = (1.0 - fVar5) * param_2 + fVar5;
  fVar3 = in_ECX[2] * *in_ECX * fVar7;
  in_EAX[3] = fVar3;
  in_EAX[7] = fVar3 + param_1 * fVar2;
  in_EAX[3] = (float)in_EAX[3] - param_1 * fVar2;
  in_EAX[9] = (1.0 - fVar6) * param_2 + fVar6;
  fVar2 = in_ECX[2];
  fVar3 = in_ECX[1];
  in_EAX[0xc] = 0;
  in_EAX[0xb] = 0;
  fVar7 = fVar2 * fVar3 * fVar7;
  in_EAX[10] = 0;
  in_EAX[6] = fVar7;
  in_EAX[8] = fVar7 - param_1 * fVar1;
  in_EAX[6] = param_1 * fVar1 + (float)in_EAX[6];
  return;
}
#endif
