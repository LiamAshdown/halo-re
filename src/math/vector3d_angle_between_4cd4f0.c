// vector3d_angle_between_4cd4f0  (Ghidra: vector3d_angle_between_4cd4f0, already named)
// address 0x4cd4f0, size 238 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: math_functions.md: "Returns the unsigned angle between two 3D vectors." Uses a
//   double-angle identity (cos(2*theta) = 2*cos(theta)^2 - 1, with cos(theta)^2 =
//   dot(a,b)^2 / (|a|^2*|b|^2)) to get the angle without a square root for the vector lengths,
//   then halves it and corrects the branch (acos(cos(2*theta)) is only 2*theta for theta <=
//   pi/2) when the dot product is negative. Zero-length input returns 0.
// register convention: first vector pointer in ECX (in_ECX), second vector pointer in EDX
//   (in_EDX), no stack arguments.
//   // blam-cc: ECX -> a, EDX -> b

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double acos(double x); // 0x00628140, CRT/compiler helper

real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b)
{
    real length_products;
    real dot;
    real cos_double_angle;
    real angle;

    angle = 0.0f;
    length_products = (a->k * a->k + a->j * a->j + a->i * a->i) *
                       (b->k * b->k + b->j * b->j + b->i * b->i);
    if (length_products != 0.0f) {
        dot = b->i * a->i + b->j * a->j + b->k * a->k;
        cos_double_angle = (dot / length_products) * dot;
        cos_double_angle = (cos_double_angle + cos_double_angle) - 1.0f;
        if (cos_double_angle < -1.0f) {
            cos_double_angle = -1.0f;
        } else if (1.0f < cos_double_angle) {
            cos_double_angle = 1.0f;
        }

        angle = (real)acos((double)cos_double_angle);
        angle = angle * 0.5f;
        if (dot < 0.0f) {
            angle = 3.1415927f - angle;
        }
    }
    return angle;
}

#if 0
Original Ghidra decompilation (0x4cd4f0):

float10 vector3d_angle_between_4cd4f0(void)

{
  float fVar1;
  float *in_ECX;
  float *in_EDX;
  float10 fVar2;
  float fVar3;

  fVar2 = (float10)0.0;
  fVar3 = (in_ECX[2] * in_ECX[2] + in_ECX[1] * in_ECX[1] + *in_ECX * *in_ECX) *
          (in_EDX[2] * in_EDX[2] + in_EDX[1] * in_EDX[1] + *in_EDX * *in_EDX);
  if (fVar3 != 0.0) {
    fVar1 = *in_EDX * *in_ECX + in_EDX[1] * in_ECX[1] + in_EDX[2] * in_ECX[2];
    fVar3 = (fVar1 / fVar3) * fVar1;
    fVar3 = (fVar3 + fVar3) - 1.0;
    if (-1.0 <= fVar3) {
      if (1.0 < fVar3) {
        fVar3 = 1.0;
      }
    }
    else {
      fVar3 = -1.0;
    }
    fVar2 = (float10)FUN_00628140(fVar3);
    fVar2 = fVar2 * (float10)0.5;
    if (fVar1 < 0.0) {
      fVar2 = (float10)3.1415927 - fVar2;
    }
  }
  return fVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
