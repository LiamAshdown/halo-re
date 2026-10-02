// quaternion_rotate_vector  (Ghidra: quaternion_rotate_vector, already named)
// address 0x4cdd40, size 153 bytes
// name confidence: 0.8   rewrite confidence: 0.65
// evidence: math_functions.md: "Rotates the 3D vector at in_ECX by the quaternion param_1,
//   writing the rotated vector to in_EDX." Optimized quaternion-vector rotation:
//   v' = a*v + b*q_v + c*(q_v x v), a = 2*w^2-1, b = 2*dot(q_v,v), c = 2*w.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: ECX -> v, EDX -> out, stack -> q
void quaternion_rotate_vector(real_quaternion *q, real_vector3d *v, real_vector3d *out)
{
    real a; // 2*w^2 - 1
    real b; // 2*dot(q_v, v)
    real c; // 2*w

    a = (q->w * q->w) * 2.0f - 1.0f;
    b = (q->j * v->j + q->k * v->k + q->i * v->i) * 2.0f;
    c = q->w + q->w;

    out->i = a * v->i + b * q->i + (q->j * v->k - q->k * v->j) * c;
    out->j = (q->k * v->i - v->k * q->i) * c + a * v->j + b * q->j;
    out->k = (q->i * v->j - q->j * v->i) * c + a * v->k + b * q->k;
}

#if 0
Original Ghidra decompilation (0x4cdd40):

void quaternion_rotate_vector(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float *in_ECX;
  float *in_EDX;

  fVar1 = param_1[3] * param_1[3];
  fVar9 = (fVar1 + fVar1) - 1.0;
  fVar11 = param_1[1] * in_ECX[1] + param_1[2] * in_ECX[2] + *param_1 * *in_ECX;
  fVar11 = fVar11 + fVar11;
  fVar10 = param_1[3] + param_1[3];
  fVar1 = param_1[2];
  fVar2 = *in_ECX;
  fVar3 = in_ECX[2];
  fVar4 = *param_1;
  fVar5 = *param_1;
  fVar6 = in_ECX[1];
  fVar7 = param_1[1];
  fVar8 = *in_ECX;
  *in_EDX = fVar9 * *in_ECX +
            fVar11 * *param_1 + (param_1[1] * in_ECX[2] - param_1[2] * in_ECX[1]) * fVar10;
  in_EDX[1] = (fVar1 * fVar2 - fVar3 * fVar4) * fVar10 + fVar9 * in_ECX[1] + fVar11 * param_1[1];
  in_EDX[2] = (fVar5 * fVar6 - fVar7 * fVar8) * fVar10 + fVar9 * in_ECX[2] + fVar11 * param_1[2];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
