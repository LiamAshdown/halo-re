// physics_shape_sphere_test_point  (Ghidra: FUN_00503ec0, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503ec0, size 199 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: physics_model_sphere.center_x/y/z (0x0c/0x10/0x14) and .radius (0x18) match
//   in_ECX+0xc/0x10/0x14/0x18 exactly (out/phase4/physics_types_notes.md).
// register convention: in_EAX -> point, in_ECX -> sphere (physics_model_sphere *), in_EDX ->
//   out_normal (real_plane3d *). param_1 is the Ghidra-recognized stack parameter (out_depth).
//   // blam-cc: EAX -> point, ECX -> sphere, EDX -> out_normal, stack -> out_depth

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// blam-cc: EAX -> point, ECX -> sphere, EDX -> out_normal, stack -> out_depth
uint8_t physics_shape_sphere_test_point(real_point3d *point, physics_model_sphere *sphere,
                                          real_plane3d *out_normal, float *out_depth)
{
    float dx = point->x - sphere->center_x;
    float dy = point->y - sphere->center_y;
    float dz = point->z - sphere->center_z;
    float dist_sq = dx * dx + dy * dy + dz * dz;

    if (dist_sq < sphere->radius * sphere->radius) {
        float dist = (float)sqrt((double)dist_sq);

        if (dist <= 0.0f) {
            out_normal->normal.i = 0.0f;
            out_normal->normal.j = 0.0f;
            out_normal->normal.k = 1.0f;
        } else {
            float inv = 1.0f / dist;
            out_normal->normal.i = dx * inv;
            out_normal->normal.j = dy * inv;
            out_normal->normal.k = dz * inv;
        }
        out_normal->d = sphere->center_x * out_normal->normal.i +
                         sphere->center_y * out_normal->normal.j +
                         sphere->center_z * out_normal->normal.k + sphere->radius;
        *out_depth = sphere->radius - dist;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x503ec0):

undefined4 FUN_00503ec0(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float *in_EAX;
  int in_ECX;
  float *in_EDX;

  fVar1 = *in_EAX - *(float *)(in_ECX + 0xc);
  fVar2 = in_EAX[1] - *(float *)(in_ECX + 0x10);
  fVar3 = in_EAX[2] - *(float *)(in_ECX + 0x14);
  fVar4 = fVar1 * fVar1 + fVar2 * fVar2 + fVar3 * fVar3;
  if (fVar4 < *(float *)(in_ECX + 0x18) * *(float *)(in_ECX + 0x18)) {
    fVar4 = SQRT(fVar4);
    if (fVar4 <= 0.0) {
      *in_EDX = 0.0;
      in_EDX[1] = 0.0;
      in_EDX[2] = 1.0;
    }
    else {
      fVar5 = 1.0 / fVar4;
      *in_EDX = fVar1 * fVar5;
      in_EDX[1] = fVar2 * fVar5;
      in_EDX[2] = fVar3 * fVar5;
    }
    in_EDX[3] = *(float *)(in_ECX + 0xc) * *in_EDX +
                *(float *)(in_ECX + 0x10) * in_EDX[1] + *(float *)(in_ECX + 0x14) * in_EDX[2] +
                *(float *)(in_ECX + 0x18);
    *param_1 = *(float *)(in_ECX + 0x18) - fVar4;
    return 1;
  }
  return 0;
}
#endif
