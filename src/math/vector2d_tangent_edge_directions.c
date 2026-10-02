// vector2d_tangent_edge_directions  (Ghidra: FUN_0043c400, renamed)
// address 0x43c400, size 163 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (verified against objdump)
// evidence: out/phase2/ai/08.md; caller FUN_0043c9a0 @0x43c9a0 (confirmed via objdump at the
//   0x43ca2c call site) first computes the distance and unit direction between two positions,
//   then calls this function as (distance, distance_to_edge + radii + epsilon, output_slot,
//   direction). Structurally this clamps a ratio extent/distance to a sine (asin domain, capped
//   at 1.0), derives the matching cosine, and rotates `direction` by +/- that angle to produce
//   two edge vectors, plus writes distance*cosine as an "adjacent" length. This is the standard
//   tangent-line-from-a-point-to-a-circle construction (sin(half_angle) = radius/distance):
//   `edge_pos`/`edge_neg` are the two tangent directions bracketing `direction`, and
//   *adjacent_out is the distance along `direction` to the tangent points' projection. Grouped
//   by out/phase4/ai_types_notes.md with the "plain math helpers that belong with types/math.h"
//   living inside the ai address range.
// register convention (confirmed via objdump at 0x43c400 and the 0x43c9a0/0x43ca2c call site):
//   direction vector in ECX (in_ECX, read-only), first output vector (rotated by +angle) in EDX
//   (in_EDX), second output vector (rotated by -angle) in ESI (unaff_ESI, a genuine register
//   parameter, never assigned inside the function); distance, extent and the adjacent-length
//   output pointer as the three recognized stack parameters (param_1, param_2, param_3).
//   // blam-cc: ECX -> direction, EDX -> edge_pos, ESI -> edge_neg, stack -> (distance, extent, adjacent_out)
//
// review fix (phase 4 math gate): outputs now computed into temporaries before storing, to
//   match the binary if an output pointer aliases `direction`.
// UNSURE: the exact purpose (this reads as a tangent-to-circle / cone-edge helper used by the
// AI-side caller for some kind of clearance or visibility check) is inferred from the shape of
// the arithmetic, not from any string or named struct field; param_2's "extent" role and the
// choice of names edge_pos/edge_neg (rather than e.g. left/right) reflect that uncertainty.
// UNSURE: only the positive-overflow branch clamps (extent/distance capped at 1.0); there is no
// symmetric clamp for a very negative ratio, preserved exactly as decompiled.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// Rotates `direction` by +/- asin(clamp(extent / distance, max 1.0)) to produce the two tangent
// edge directions (edge_pos = +angle, edge_neg = -angle), and writes distance * cos(angle) to
// *adjacent_out.
void vector2d_tangent_edge_directions(const real_vector2d *direction, real_vector2d *edge_pos,
                                       real_vector2d *edge_neg, real distance, real extent,
                                       real *adjacent_out)
{
    real sin_ratio = 1.0f;
    real cos_ratio;
    real i, j;

    if (distance != 0.0f) {
        sin_ratio = extent / distance;
        if (sin_ratio > 1.0f) {
            sin_ratio = 1.0f;
        }
    }
    cos_ratio = (real)sqrt((double)(1.0f - sin_ratio * sin_ratio));

    // Both components of each output are computed before either is stored (the FPU trace reads
    // [ecx]/[ecx+4] for a pair, then stores it), so an output aliasing `direction` behaves as in
    // the binary. edge_neg is written i then j; edge_pos is written j then i.
    i = cos_ratio * direction->i - (-sin_ratio) * direction->j;
    j = cos_ratio * direction->j + (-sin_ratio) * direction->i;
    edge_neg->i = i;
    edge_neg->j = j;

    j = cos_ratio * direction->j + sin_ratio * direction->i;
    i = cos_ratio * direction->i - sin_ratio * direction->j;
    edge_pos->j = j;
    edge_pos->i = i;

    *adjacent_out = cos_ratio * distance;
}

#if 0
Original Ghidra decompilation (0x43c400):

void FUN_0043c400(float param_1,float param_2,float *param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;

  fVar3 = 1.0;
  if ((param_1 != 0.0) && (fVar3 = param_2 / param_1, 1.0 < fVar3)) {
    fVar3 = 1.0;
  }
  fVar4 = SQRT(1.0 - fVar3 * fVar3);
  fVar1 = *in_ECX;
  fVar2 = in_ECX[1];
  *unaff_ESI = fVar4 * *in_ECX - -fVar3 * in_ECX[1];
  unaff_ESI[1] = fVar4 * fVar2 + -fVar3 * fVar1;
  fVar1 = *in_ECX;
  fVar2 = in_ECX[1];
  in_EDX[1] = fVar4 * in_ECX[1] + fVar3 * *in_ECX;
  *in_EDX = fVar4 * fVar1 - fVar3 * fVar2;
  *param_3 = fVar4 * param_1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
