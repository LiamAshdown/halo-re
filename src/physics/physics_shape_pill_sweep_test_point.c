// physics_shape_pill_sweep_test_point  (Ghidra: FUN_00503050, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503050, size 569 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase2/results/physics_00.json: "Solves the standard moving-sphere-vs-point
//   quadratic ... to find the sweep fraction and closest-point ratio, falling back to
//   FUN_00503290 for the segment-endpoint cases." Closest-approach-of-a-moving-sphere-to-a-line
//   quadratic: solves for the sweep fraction (out_t) and, when the closest point falls within
//   the segment, the parametric position along it (out_edge_fraction); otherwise falls back to
//   physics_shape_sphere_sweep_test_ray against whichever endpoint the closest point fell past.
//   The final edge fraction is divided by |edge_dir|^2 and the sweep-time recurrence
//   (edge_param(t) = rel.edge_dir + t * edge_dir.delta) only comes out dimensionally consistent
//   if the register carrying the "first" cross-product operand is edge_dir and the other is the
//   motion vector -- see the register convention note below.
// register convention: in_ECX -> near_vertex, unaff_EDI -> edge_dir, unaff_EBX -> origin,
//   in_EDX -> delta. param_1..param_3 are Ghidra-recognized stack parameters (radius, out_t,
//   out_edge_fraction).
//   // blam-cc: ECX -> near_vertex, EDX -> delta, EBX -> origin, EDI -> edge_dir,
//   //           stack -> radius, out_t, out_edge_fraction
// UNSURE: both calls to physics_shape_sphere_sweep_test_ray show only the radius argument in
// Ghidra's decompile; `point` is reconstructed as the far vertex (near_vertex + edge_dir) for
// the "swept past the end of the segment" branch and the near vertex for the "swept before the
// start" branch, with origin/delta/out_t carried over from this function's own parameters
// (which is what those registers still hold, unchanged, at each call site).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction
extern uint8_t physics_shape_sphere_sweep_test_ray(real_point3d *point, real_point3d *origin,
                                                    real_vector3d *delta, float *out_t,
                                                    float radius); // 0x503290, this batch

// blam-cc: ECX -> near_vertex, EDX -> delta, EBX -> origin, EDI -> edge_dir,
//          stack -> radius, out_t, out_edge_fraction
uint8_t physics_shape_pill_sweep_test_point(real_point3d *near_vertex, real_vector3d *delta,
                                             real_point3d *origin, real_vector3d *edge_dir,
                                             float radius, float *out_t,
                                             float *out_edge_fraction)
{
    float rel_x = origin->x - near_vertex->x;
    float rel_y = origin->y - near_vertex->y;
    float rel_z = origin->z - near_vertex->z;
    float edge_len_sq = edge_dir->i * edge_dir->i + edge_dir->j * edge_dir->j +
                         edge_dir->k * edge_dir->k;
    float edge_dot_delta = edge_dir->i * delta->i + edge_dir->j * delta->j + edge_dir->k * delta->k;
    float delta_len_sq = delta->i * delta->i + delta->j * delta->j + delta->k * delta->k;
    float disc_scale = delta_len_sq * edge_len_sq - edge_dot_delta * edge_dot_delta;

    if (disc_scale != 0.0f) {
        float rel_dot_edge = rel_z * edge_dir->k + rel_y * edge_dir->j + rel_x * edge_dir->i;
        float rel_dot_delta = rel_z * delta->k + rel_y * delta->j + rel_x * delta->i;
        float b = rel_dot_edge * edge_dot_delta - rel_dot_delta * edge_len_sq;
        float rel_len_sq = rel_z * rel_z + rel_y * rel_y + rel_x * rel_x;
        float disc = b * b - ((rel_len_sq - radius * radius) * edge_len_sq - rel_dot_edge * rel_dot_edge) *
                              disc_scale;

        if (0.0f <= disc) {
            float sqrt_disc = (float)sqrt((double)disc);
            float t = -((sqrt_disc + b) * (1.0f / disc_scale));
            // equivalent to (t <= 1.0f)
            if ((t <= 1.0f) && (0.0f <= -((b - sqrt_disc) * (1.0f / disc_scale)))) {
                float edge_param;

                if (t < 0.0f) {
                    t = 0.0f;
                }
                edge_param = edge_dot_delta * t + rel_dot_edge;
                if (0.0f <= edge_param) {
                    if (edge_param <= edge_len_sq) {
                        *out_t = t;
                        *out_edge_fraction = edge_param / edge_len_sq;
                        return 1;
                    }
                    {
                        real_point3d far_vertex;
                        far_vertex.x = near_vertex->x + edge_dir->i;
                        far_vertex.y = near_vertex->y + edge_dir->j;
                        far_vertex.z = near_vertex->z + edge_dir->k;
                        if (physics_shape_sphere_sweep_test_ray(&far_vertex, origin, delta, out_t,
                                                                 radius)) {
                            *out_edge_fraction = 1.0f;
                            return 1;
                        }
                    }
                } else {
                    if (physics_shape_sphere_sweep_test_ray(near_vertex, origin, delta, out_t,
                                                             radius)) {
                        *out_edge_fraction = 0.0f;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x503050):

undefined4 FUN_00503050(float param_1,float *param_2,float *param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  char cVar9;
  float *in_ECX;
  float *in_EDX;
  float *unaff_EBX;
  float *unaff_EDI;

  fVar1 = *unaff_EBX - *in_ECX;
  fVar5 = unaff_EBX[1] - in_ECX[1];
  fVar2 = unaff_EBX[2] - in_ECX[2];
  fVar6 = unaff_EDI[1] * unaff_EDI[1] + *unaff_EDI * *unaff_EDI + unaff_EDI[2] * unaff_EDI[2];
  fVar7 = *unaff_EDI * *in_EDX + unaff_EDI[1] * in_EDX[1] + unaff_EDI[2] * in_EDX[2];
  fVar8 = (in_EDX[2] * in_EDX[2] + in_EDX[1] * in_EDX[1] + *in_EDX * *in_EDX) * fVar6 -
          fVar7 * fVar7;
  if (fVar8 != 0.0) {
    fVar4 = fVar2 * unaff_EDI[2] + fVar5 * unaff_EDI[1] + fVar1 * *unaff_EDI;
    fVar3 = fVar4 * fVar7 - (fVar2 * in_EDX[2] + fVar5 * in_EDX[1] + fVar1 * *in_EDX) * fVar6;
    fVar1 = fVar3 * fVar3 -
            (((fVar2 * fVar2 + fVar5 * fVar5 + fVar1 * fVar1) - param_1 * param_1) * fVar6 -
            fVar4 * fVar4) * fVar8;
    if (0.0 <= fVar1) {
      fVar1 = SQRT(fVar1);
      fVar2 = -((fVar1 + fVar3) * (1.0 / fVar8));
      if ((fVar2 < 1.0 != (fVar2 == 1.0)) && (0.0 <= -((fVar3 - fVar1) * (1.0 / fVar8)))) {
        if (fVar2 < 0.0) {
          fVar2 = 0.0;
        }
        fVar4 = fVar7 * fVar2 + fVar4;
        if (0.0 <= fVar4) {
          if (fVar4 <= fVar6) {
            *param_2 = fVar2;
            *param_3 = fVar4 / fVar6;
            return 1;
          }
          cVar9 = FUN_00503290(param_1);
          if (cVar9 != '\0') {
            *param_3 = 1.0;
            return 1;
          }
        }
        else {
          cVar9 = FUN_00503290(param_1);
          if (cVar9 != '\0') {
            *param_3 = 0.0;
            return 1;
          }
        }
      }
    }
  }
  return 0;
}
#endif
