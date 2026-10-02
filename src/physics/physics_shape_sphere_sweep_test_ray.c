// physics_shape_sphere_sweep_test_ray  (Ghidra: FUN_00503290, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503290, size 203 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: out/phase2/results/physics_00.json: "Second quadratic solve (distance-to-point vs
//   radius, then along-ray discriminant) used as a fallback from FUN_00503050 and FUN_00502460
//   when the primary closest-approach case doesn't resolve." Standard moving-sphere-vs-point
//   test: immediate hit if already inside the sphere, else a quadratic in the sweep parameter.
// register convention: in_EAX -> point, in_ECX -> origin, in_EDX -> delta, unaff_ESI -> out_t.
//   param_1 is the Ghidra-recognized stack parameter (radius).
//   // blam-cc: EAX -> point, ECX -> origin, EDX -> delta, ESI -> out_t, stack -> radius
// UNSURE: the two call sites in physics_shape_pill_sweep_test_point (0x503050) show only the
// radius argument; `point` is reconstructed there as the edge's near or far vertex (whichever
// end the swept parameter fell outside of), origin/delta carried over unchanged from that
// function's own parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// blam-cc: EAX -> point, ECX -> origin, EDX -> delta, ESI -> out_t, stack -> radius
uint8_t physics_shape_sphere_sweep_test_ray(real_point3d *point, real_point3d *origin,
                                             real_vector3d *delta, float *out_t, float radius)
{
    float dx = origin->x - point->x;
    float dy = origin->y - point->y;
    float dz = origin->z - point->z;
    float c = (dz * dz + dy * dy + dx * dx) - radius * radius;

    // equivalent to (!isnan(c) && c <= 0.0f): the origin is already inside (or touching) the
    // sphere around point, so the hit is immediate.
    if (c == c && c <= 0.0f) {
        *out_t = 0.0f;
        return 1;
    }

    {
        float b = dz * delta->k + dy * delta->j + dx * delta->i;
        // equivalent to (b > 0.0f)
        if (b >= 0.0f && b != 0.0f) {
            float a = delta->i * delta->i + delta->k * delta->k + delta->j * delta->j;
            float disc = b * b - a * c;
            if (0.0f <= disc) {
                float t = (b - (float)sqrt((double)disc)) / a;
                // equivalent to (t <= 1.0f); see the XOR idiom in
                // breakable_surface_apply_damage.c
                if (t <= 1.0f) {
                    *out_t = t;
                    return 1;
                }
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x503290):

uint FUN_00503290(float param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;
  ushort uVar5;

  fVar1 = *in_ECX - *in_EAX;
  fVar3 = in_ECX[1] - in_EAX[1];
  fVar4 = in_ECX[2] - in_EAX[2];
  fVar2 = (fVar4 * fVar4 + fVar3 * fVar3 + fVar1 * fVar1) - param_1 * param_1;
  if (!NAN(fVar2) && fVar2 < 0.0 != (fVar2 == 0.0)) {
    *unaff_ESI = 0.0;
    return CONCAT31((uint3)(byte)(fVar2 < 0.0 | (byte)((ushort)((ushort)NAN(fVar2) << 10) >> 8) |
                                 (byte)((ushort)((ushort)(fVar2 == 0.0) << 0xe) >> 8)),1);
  }
  fVar1 = fVar4 * in_EDX[2] + fVar3 * in_EDX[1] + fVar1 * *in_EDX;
  uVar5 = (ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 | (ushort)(fVar1 == 0.0) << 0xe;
  if (fVar1 >= 0.0 && (fVar1 == 0.0) == 0) {
    fVar3 = *in_EDX * *in_EDX + in_EDX[2] * in_EDX[2] + in_EDX[1] * in_EDX[1];
    fVar2 = fVar1 * fVar1 - fVar3 * fVar2;
    uVar5 = (ushort)(fVar2 < 0.0) << 8 | (ushort)NAN(fVar2) << 10 | (ushort)(fVar2 == 0.0) << 0xe;
    if (fVar2 >= 0.0) {
      fVar3 = (fVar1 - SQRT(fVar2)) / fVar3;
      uVar5 = (ushort)(fVar3 < 1.0) << 8 | (ushort)NAN(fVar3) << 10 | (ushort)(fVar3 == 1.0) << 0xe;
      if (fVar3 < 1.0 != (fVar3 == 1.0)) {
        *unaff_ESI = fVar3;
        return CONCAT31((uint3)(byte)(uVar5 >> 8),1);
      }
    }
  }
  return (uint)uVar5;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
