// ray_intersects_sphere  (Ghidra: ray_intersects_sphere, already named)
// address 0x4ce3a0, size 320 bytes
// name confidence: 0.7   rewrite confidence: 0.55
// evidence: math_functions.md: "Tests a ray (origin in_EAX, direction in_EDX) against a sphere
//   (center param_1, radius param_2), returning the hit t (via unaff_EDI) and surface normal
//   (in_ECX) on success." Origin-inside-sphere is a special case (t=0, normal = direction from
//   center to origin); otherwise the standard quadratic ray/sphere test, with `direction`
//   apparently not required to be unit length: the near-root numerator is compared against
//   |direction|^2 (i.e. t <= 1) rather than being unconditionally accepted, and the accepted t
//   is that numerator divided by |direction|^2.
// register convention: origin pointer in EAX (in_EAX), normal-output pointer in ECX (in_ECX),
//   direction pointer in EDX (in_EDX), t-output pointer in EDI (unaff_EDI); center pointer and
//   radius as the recognized stack parameters (param_1, param_2).
//   // blam-cc: EAX -> origin, ECX -> normal_out, EDX -> direction, EDI -> t_out, stack ->
//   (center, radius)

// RETURN WIDTH (verified in the disassembly): the success/failure result is written with
// `mov al,1` / `xor al,al` and never zero-extended, so only AL carries the result and the
// return type is a byte, not an int. Declared uint8_t below; reading it as a 32-bit value
// would pick up whatever the upper 24 bits of EAX happened to hold.

#include "tags.h"
#include "math.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction
extern void vector3d_normalize(real_vector3d *v); // 0x4cd320

uint8_t ray_intersects_sphere(real_point3d *origin, real_vector3d *normal_out, real_vector3d *direction, real *t_out, real_point3d *center, real radius)
{
    real oc_i;
    real oc_j;
    real oc_k;
    real dot_oc_dir;

    oc_i = origin->x - center->x;
    oc_j = origin->y - center->y;
    oc_k = origin->z - center->z;

    dot_oc_dir = oc_i * direction->i + oc_k * direction->k + oc_j * direction->j;
    if (dot_oc_dir < 0.0f) {
        real oc_len2 = oc_k * oc_k + oc_i * oc_i + oc_j * oc_j;
        real c = oc_len2 - radius * radius;

        if (c < 0.0f) {
            real inv_len = 1.0f / (real)sqrt((double)oc_len2);
            *t_out = 0.0f;
            normal_out->i = oc_i * inv_len;
            normal_out->j = oc_j * inv_len;
            normal_out->k = inv_len * oc_k;
            return 1;
        }

        {
            real dir_len2 = direction->i * direction->i + direction->k * direction->k + direction->j * direction->j;
            real discriminant = dot_oc_dir * dot_oc_dir - dir_len2 * c;

            if (0.0f <= discriminant) {
                real t_numerator = -((real)sqrt((double)discriminant) + dot_oc_dir);
                if (t_numerator <= dir_len2) {
                    real t = t_numerator / dir_len2;
                    *t_out = t;
                    normal_out->i = t * direction->i + oc_i;
                    normal_out->j = t * direction->j + oc_j;
                    normal_out->k = t * direction->k + oc_k;
                    vector3d_normalize(normal_out);
                    return 1;
                }
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ce3a0):

undefined2 ray_intersects_sphere(float *param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;
  float *unaff_EDI;

  fVar1 = *in_EAX - *param_1;
  fVar2 = in_EAX[1] - param_1[1];
  fVar3 = in_EAX[2] - param_1[2];
  fVar4 = fVar1 * *in_EDX + fVar3 * in_EDX[2] + fVar2 * in_EDX[1];
  if (fVar4 < 0.0) {
    fVar5 = fVar3 * fVar3 + fVar1 * fVar1 + fVar2 * fVar2;
    fVar6 = fVar5 - param_2 * param_2;
    if (fVar6 < 0.0 != (fVar6 == 0.0)) {
      *unaff_EDI = 0.0;
      fVar4 = 1.0 / SQRT(fVar5);
      *in_ECX = fVar1 * fVar4;
      in_ECX[1] = fVar2 * fVar4;
      in_ECX[2] = fVar4 * fVar3;
      return 1;
    }
    fVar5 = *in_EDX * *in_EDX + in_EDX[2] * in_EDX[2] + in_EDX[1] * in_EDX[1];
    fVar6 = fVar4 * fVar4 - fVar5 * fVar6;
    if (0.0 <= fVar6) {
      fVar4 = -(SQRT(fVar6) + fVar4);
      if (fVar4 < fVar5 != (fVar4 == fVar5)) {
        fVar4 = fVar4 / fVar5;
        *unaff_EDI = fVar4;
        *in_ECX = fVar4 * *in_EDX + fVar1;
        in_ECX[1] = fVar4 * in_EDX[1] + fVar2;
        in_ECX[2] = fVar4 * in_EDX[2] + fVar3;
        vector3d_normalize();
        return 1;
      }
    }
  }
  return 0;
}
#endif
