// physics_shape_sphere_test_ray  (Ghidra: FUN_00504430, still unnamed there; name from
// out/phase2/results/physics_00.json: "Ray-vs-sphere quadratic ... analogous to FUN_00503ec0
// but parameterized along a direction, used by the ray dispatcher FUN_00504bb0.")
// address 0x504430, size 396 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: physics_model_sphere.center_x/y/z (0x0c/0x10/0x14) and .radius (0x18) match
//   unaff_ESI+0xc/0x10/0x14/0x18 exactly (out/phase4/physics_types_notes.md), the same fields
//   physics_shape_sphere_test_point (0x503ec0, this module) reads off in_ECX; the "a<b !=
//   (a==b)" idiom throughout resolves to plain <= (established by physics_shape_pill_test_point,
//   this module, which documents the same pattern); the caller FUN_00504bb0 (this batch) reads
//   this function's out_plane back as a real_plane3d and out_t as a float, matching
//   physics_model_contact's own plane/t fields.
// register convention: in_EAX -> origin, in_ECX -> delta, unaff_ESI -> sphere
//   (physics_model_sphere *), unaff_EDI -> out_plane (real_plane3d *). param_1 is the
//   Ghidra-recognized stack parameter (out_t).
//   // blam-cc: EAX -> origin, ECX -> delta, ESI -> sphere, EDI -> out_plane, stack -> out_t

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// Tests a world-space ray (origin, delta) against sphere, the ray counterpart of
// physics_shape_sphere_test_point. When origin already starts inside (or exactly on) the
// sphere, reports an immediate hit at t = 0; otherwise solves the ray/sphere quadratic and
// reports the near root when it falls within [0, 1] of delta. On a hit, out_plane is the
// outward-facing separating plane at the hit point.
// blam-cc: EAX -> origin, ECX -> delta, ESI -> sphere, EDI -> out_plane, stack -> out_t
uint8_t physics_shape_sphere_test_ray(real_point3d *origin, real_vector3d *delta,
                                        physics_model_sphere *sphere, real_plane3d *out_plane,
                                        float *out_t)
{
    float dx = sphere->center_x - origin->x;
    float dy = sphere->center_y - origin->y;
    float dz = sphere->center_z - origin->z;
    float c = (dx * dx + dz * dz + dy * dy) - sphere->radius * sphere->radius;
    float t;
    real length;

    if (c <= 0.0f) {
        // origin starts inside (or exactly on) the sphere: hits immediately
        t = 0.0f;
    } else {
        float b = dz * delta->k + dy * delta->j + dx * delta->i;

        if (b <= 0.0f) {
            return 0;
        }

        {
            float delta_len_sq = delta->i * delta->i + delta->k * delta->k + delta->j * delta->j;
            float disc = b * b - delta_len_sq * c;

            if (disc < 0.0f) {
                return 0;
            }
            t = b - (float)sqrt((double)disc);
            if (t > delta_len_sq) {
                return 0;
            }
            t = t / delta_len_sq;
        }
    }

    *out_t = t;
    out_plane->normal.i = t * delta->i - dx;
    out_plane->normal.j = t * delta->j - dy;
    out_plane->normal.k = t * delta->k - dz;
    length = vector3d_normalize_with_length(&out_plane->normal);
    if (length == 0.0f) {
        out_plane->normal.i = 0.0f;
        out_plane->normal.j = 0.0f;
        out_plane->normal.k = 1.0f;
    }
    out_plane->d = sphere->center_x * out_plane->normal.i + sphere->center_y * out_plane->normal.j +
                    sphere->center_z * out_plane->normal.k + sphere->radius;
    return 1;
}

#if 0
Original Ghidra decompilation (0x504430):

uint FUN_00504430(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float *in_EAX;
  float *in_ECX;
  int unaff_ESI;
  float *unaff_EDI;
  ushort uVar7;
  float10 fVar8;
  float10 fVar9;

  fVar4 = *(float *)(unaff_ESI + 0xc) - *in_EAX;
  fVar5 = *(float *)(unaff_ESI + 0x10) - in_EAX[1];
  fVar6 = *(float *)(unaff_ESI + 0x14) - in_EAX[2];
  fVar1 = (fVar4 * fVar4 + fVar6 * fVar6 + fVar5 * fVar5) -
          *(float *)(unaff_ESI + 0x18) * *(float *)(unaff_ESI + 0x18);
  if (fVar1 < 0.0 != (fVar1 == 0.0)) {
    *param_1 = 0.0;
LAB_0050452e:
    fVar1 = *param_1;
    fVar2 = in_ECX[1];
    fVar3 = in_ECX[2];
    *unaff_EDI = fVar1 * *in_ECX - fVar4;
    unaff_EDI[1] = fVar1 * fVar2 - fVar5;
    unaff_EDI[2] = fVar1 * fVar3 - fVar6;
    fVar8 = (float10)vector3d_normalize_with_length();
    fVar9 = (float10)0.0;
    if (fVar9 == fVar8) {
      *unaff_EDI = 0.0;
      unaff_EDI[1] = 0.0;
      unaff_EDI[2] = 1.0;
    }
    unaff_EDI[3] = *(float *)(unaff_ESI + 0xc) * *unaff_EDI +
                   *(float *)(unaff_ESI + 0x10) * unaff_EDI[1] +
                   *(float *)(unaff_ESI + 0x14) * unaff_EDI[2] + *(float *)(unaff_ESI + 0x18);
    return CONCAT31((uint3)(byte)(fVar9 < fVar8 |
                                  (byte)((ushort)((ushort)(NAN(fVar9) || NAN(fVar8)) << 10) >> 8) |
                                 (byte)((ushort)((ushort)(fVar9 == fVar8) << 0xe) >> 8)),1);
  }
  fVar2 = fVar6 * in_ECX[2] + fVar5 * in_ECX[1] + fVar4 * *in_ECX;
  uVar7 = (ushort)(fVar2 < 0.0) << 8 | (ushort)NAN(fVar2) << 10 | (ushort)(fVar2 == 0.0) << 0xe;
  if (fVar2 >= 0.0 && (fVar2 == 0.0) == 0) {
    fVar3 = *in_ECX * *in_ECX + in_ECX[2] * in_ECX[2] + in_ECX[1] * in_ECX[1];
    fVar1 = fVar2 * fVar2 - fVar3 * fVar1;
    uVar7 = (ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 | (ushort)(fVar1 == 0.0) << 0xe;
    if (fVar1 >= 0.0) {
      fVar2 = fVar2 - SQRT(fVar1);
      uVar7 = (ushort)(fVar2 < fVar3) << 8 | (ushort)(NAN(fVar2) || NAN(fVar3)) << 10 |
              (ushort)(fVar2 == fVar3) << 0xe;
      if (fVar2 < fVar3 != (fVar2 == fVar3)) {
        *param_1 = fVar2 / fVar3;
        goto LAB_0050452e;
      }
    }
  }
  return (uint)uVar7;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
