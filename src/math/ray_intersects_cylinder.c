// ray_intersects_cylinder  (Ghidra: ray_intersects_cylinder, already named)
// address 0x4ce4e0, size 469 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: math_functions.md: "Tests a ray against a finite cylinder (with a spherical cap
//   fallback), writing the hit fraction and point on success." The cylinder is Z-axis-aligned:
//   base at `center`, side test uses only the ray's XY components against `radius`, and axial
//   position (fVar3) selects between a side hit, a top-cap sphere test (center + (0,0,height))
//   and a bottom-cap sphere test (center itself, unchanged so passed through silently); a final
//   dot(direction, normal) <= 0 check rejects back-face hits regardless of which case fired.
// register convention: t-output pointer in EAX (in_EAX), base-center pointer in ECX (in_ECX),
//   ray origin pointer in EBX (unaff_EBX), ray direction pointer in ESI (unaff_ESI); height,
//   radius and hit-point-output pointer as the recognized stack parameters (param_1, param_2,
//   param_3).
//   // blam-cc: EAX -> t_out, ECX -> center, EBX -> origin, ESI -> direction, stack -> (height,
//   radius, hit_out)
//
// UNSURE, significantly:
// - `param_1` (height) is used both added directly to center.z (top-cap position) and squared
//   as a divisor for the axial parameter; the squared use is not understood and is preserved
//   literally rather than renamed to something more specific.
// - Both `ray_intersects_sphere` calls show fewer visible arguments than that function's
//   6-parameter signature (0x4ce3a0, defined in this same batch). Reconstructed here on the
//   theory that Ghidra only prints an argument when a *new* value was loaded for it: origin,
//   direction and t_out are this function's own (unchanged, so silently forwarded); center is
//   this function's own `center` for the bottom-cap call (silent) or a freshly-built top-cap
//   point for the other (`&local_c`, the one pointer Ghidra did print); `hit_out` (param_3) is
//   reused as `ray_intersects_sphere`'s normal-output parameter, matching how it doubles as a
//   hit-point-then-normal buffer in the side-hit case below.

// RETURN WIDTH (verified in the disassembly): the success/failure result is written with
// `mov al,1` / `xor al,al` and never zero-extended, so only AL carries the result and the
// return type is a byte, not an int. Declared uint8_t below; reading it as a 32-bit value
// would pick up whatever the upper 24 bits of EAX happened to hold.

#include "tags.h"
#include "math.h"
#include "fn_math.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction


uint8_t ray_intersects_cylinder(real height, real radius, real_vector3d *hit_out, real *t_out, real_point3d *center, real_point3d *origin, real_vector3d *direction)
{
    real dx; // origin.x - center.x
    real dy; // origin.y - center.y
    real b;  // dot((dx,dy), (direction.i,direction.j))
    real c;  // dx^2+dy^2 - radius^2
    real t;  // circle/side parameter
    uint8_t hit;
    real axial_t;

    dx = origin->x - center->x;
    dy = origin->y - center->y;
    b = dy * direction->j + dx * direction->i;
    c = (dx * dx + dy * dy) - radius * radius;

    if (c < 0.0f) {
        // origin's XY position is already inside the cylinder's radius
        t = 0.0f;
    } else {
        real a = direction->i * direction->i + direction->j * direction->j;
        real discriminant = b * b - a * c;
        real t_numerator;

        if (discriminant < 0.0f) {
            return 0;
        }
        t_numerator = -((real)sqrt((double)discriminant) + b);
        if (a < t_numerator) { // t_numerator > a  =>  t (after dividing by a) would exceed 1
            return 0;
        }
        t = t_numerator / a;
    }

    hit = 1;
    axial_t = (t * direction->k + (origin->z - center->z)) / (height * height);

    if (0.0f <= axial_t) {
        if (axial_t <= 1.0f) {
            // side hit
            if (0.0f <= b) {
                return 0;
            }
            *t_out = t;
            hit_out->i = t * direction->i + dx;
            hit_out->j = t * direction->j + dy;
            vector2d_normalize((real_vector2d *)hit_out);
            hit_out->k = 0.0f;
        } else {
            // top cap: sphere test against center + (0, 0, height)
            real_point3d top_center;
            top_center.x = center->x;
            top_center.y = center->y;
            top_center.z = height + center->z;
            hit = ray_intersects_sphere(origin, hit_out, direction, t_out, &top_center, radius);
        }
    } else {
        // bottom cap: sphere test against center itself
        hit = ray_intersects_sphere(origin, hit_out, direction, t_out, center, radius);
    }

    if (hit == 0) {
        return 0;
    }
    if (direction->i * hit_out->i + hit_out->j * direction->j + hit_out->k * direction->k <= 0.0f) {
        return hit;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ce4e0):

char ray_intersects_cylinder(float param_1,float param_2,float *param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  float *in_ECX;
  float *unaff_EBX;
  float *unaff_ESI;
  char local_15;
  float local_c;
  float local_8;
  float local_4;

  local_c = *unaff_EBX - *in_ECX;
  local_8 = unaff_EBX[1] - in_ECX[1];
  fVar1 = local_8 * unaff_ESI[1] + local_c * *unaff_ESI;
  fVar2 = (local_c * local_c + local_8 * local_8) - param_2 * param_2;
  if (fVar2 < 0.0 == (fVar2 == 0.0)) {
    fVar3 = *unaff_ESI * *unaff_ESI + unaff_ESI[1] * unaff_ESI[1];
    fVar2 = fVar1 * fVar1 - fVar3 * fVar2;
    if (fVar2 < 0.0) {
      return '\0';
    }
    fVar2 = -(SQRT(fVar2) + fVar1);
    if (fVar2 < fVar3 == (fVar2 == fVar3)) {
      return '\0';
    }
    fVar2 = fVar2 / fVar3;
  }
  else {
    fVar2 = 0.0;
  }
  local_15 = '\x01';
  fVar3 = (fVar2 * unaff_ESI[2] + (unaff_EBX[2] - in_ECX[2])) / (param_1 * param_1);
  if (0.0 <= fVar3) {
    if (fVar3 <= 1.0) {
      if (0.0 <= fVar1) {
        return '\0';
      }
      *in_EAX = fVar2;
      *param_3 = fVar2 * *unaff_ESI + local_c;
      param_3[1] = fVar2 * unaff_ESI[1] + local_8;
      vector2d_normalize();
      param_3[2] = 0.0;
      goto LAB_004ce5e5;
    }
    local_c = *in_ECX;
    local_8 = in_ECX[1];
    local_4 = param_1 + in_ECX[2];
    local_15 = ray_intersects_sphere(&local_c,param_2);
  }
  else {
    local_15 = ray_intersects_sphere();
  }
  if (local_15 == '\0') {
    return '\0';
  }
LAB_004ce5e5:
  if (*unaff_ESI * *param_3 + param_3[1] * unaff_ESI[1] + param_3[2] * unaff_ESI[2] <= 0.0) {
    return local_15;
  }
  return '\0';
}
#endif
