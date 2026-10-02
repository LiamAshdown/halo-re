// ray_intersect_sphere_distance  (Ghidra: ray_intersect_sphere_distance, already named)
// address 0x4ce7d0, size 240 bytes
// name confidence: 0.65   rewrite confidence: 0.6
// evidence: math_functions.md: "Computes the ray-parameter distance to the nearest intersection
//   with a sphere of radius param_1, or FLT_MAX if the ray misses." Same structure as
//   ray_intersects_sphere_test @0x4ce6c0 but cleanly decompiled (no CONCAT/NAN artifacts) and
//   returning the actual near-root distance instead of a boolean.
// register convention: origin pointer in EAX (in_EAX), sphere-center pointer in ECX (in_ECX),
//   direction pointer in EDX (in_EDX); radius as the recognized stack parameter (param_1).
//   // blam-cc: EAX -> origin, ECX -> center, EDX -> direction, stack -> radius

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

real ray_intersect_sphere_distance(real_point3d *origin, real_point3d *center, real_vector3d *direction, real radius)
{
    real to_center_i;
    real to_center_j;
    real to_center_k;
    real c;

    to_center_i = center->x - origin->x;
    to_center_j = center->y - origin->y;
    to_center_k = center->z - origin->z;
    c = (to_center_i * to_center_i + to_center_j * to_center_j + to_center_k * to_center_k) - radius * radius;

    if (0.0f <= c) {
        real b = direction->i * to_center_i + direction->j * to_center_j + direction->k * to_center_k;
        if (b < 0.0f) {
            real a = direction->i * direction->i + direction->j * direction->j + direction->k * direction->k;
            real discriminant = b * b - a * c;
            if (0.0f < discriminant) {
                return (-b - (real)sqrt((double)discriminant)) / a;
            }
        }
    } else {
        return 0.0f;
    }
    return 3.4028235e+38f;
}

#if 0
Original Ghidra decompilation (0x4ce7d0):

float10 ray_intersect_sphere_distance(float param_1)

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
  float *in_EDX;
  float10 fVar8;

  fVar4 = *in_ECX - *in_EAX;
  fVar6 = in_ECX[1] - in_EAX[1];
  fVar7 = in_ECX[2] - in_EAX[2];
  fVar5 = (fVar4 * fVar4 + fVar6 * fVar6 + fVar7 * fVar7) - param_1 * param_1;
  if (fVar5 < 0.0) {
    return (float10)0.0;
  }
  fVar1 = *in_EDX;
  fVar2 = in_EDX[1];
  fVar3 = in_EDX[2];
  fVar4 = fVar1 * fVar4 + fVar2 * fVar6 + fVar3 * fVar7;
  if (fVar4 < 0.0) {
    fVar6 = fVar1 * fVar1 + fVar2 * fVar2 + fVar3 * fVar3;
    fVar8 = (float10)fVar4 * (float10)fVar4 - (float10)fVar6 * (float10)fVar5;
    if (fVar8 < (float10)0.0 == (fVar8 == (float10)0.0)) {
      return (-(float10)fVar4 - SQRT(fVar8)) / (float10)fVar6;
    }
  }
  return (float10)3.4028235e+38;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
