// vector3d_rotate_about_axis_perpendicular  (Ghidra: FUN_004cd700; renamed, no established name)
// address 0x4cd700, size 137 bytes
// VERIFIED against disassembly 0x4cd700..0x4cd789 (2026-09-30)
// name confidence: 0.4   rewrite confidence: 0.65
// evidence: math_functions.md: "Rotates the vector at EAX around the axis at ECX by a given
//   sin/cos pair, using the simplified formula valid only when the vector is already
//   perpendicular to the axis." v' = cos*v + sin*(axis x v); this is
//   vector3d_rotate_about_axis @0x4cd820's full Rodrigues formula with the
//   (1-cos)*dot(v,axis)*axis term dropped, which is exactly zero when v is already
//   perpendicular to axis.
// register convention: vector pointer in EAX (in_EAX, rotated in place), axis pointer in ECX
//   (in_ECX); sin/cos as the recognized stack parameters (param_1, param_2).
//   // blam-cc: EAX -> v, ECX -> axis, stack -> (sin_angle, cos_angle)

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

void vector3d_rotate_about_axis_perpendicular(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle)
{
    real old_i;
    real old_j;
    real old_k;

    old_i = v->i;
    old_j = v->j;
    old_k = v->k;

    v->i = (axis->j * old_k - old_j * axis->k) * sin_angle + cos_angle * old_i;
    v->j = (old_i * axis->k - axis->i * old_k) * sin_angle + cos_angle * old_j;
    v->k = cos_angle * old_k + (old_j * axis->i - old_i * axis->j) * sin_angle;
}

#if 0
Original Ghidra decompilation (0x4cd700):

void FUN_004cd700(float param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float *in_EAX;
  float *in_ECX;

  fVar1 = *in_EAX;
  fVar2 = in_ECX[2];
  fVar3 = *in_ECX;
  fVar4 = in_EAX[1];
  fVar5 = *in_ECX;
  fVar6 = *in_EAX;
  fVar7 = in_ECX[1];
  *in_EAX = (in_ECX[1] * in_EAX[2] - in_EAX[1] * in_ECX[2]) * param_1 + param_2 * *in_EAX;
  in_EAX[1] = (fVar1 * fVar2 - fVar3 * in_EAX[2]) * param_1 + param_2 * in_EAX[1];
  in_EAX[2] = param_2 * in_EAX[2] + (fVar4 * fVar5 - fVar6 * fVar7) * param_1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
