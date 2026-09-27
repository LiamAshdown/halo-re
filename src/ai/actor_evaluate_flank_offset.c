// actor_evaluate_flank_offset  (Ghidra: actor_evaluate_flank_offset; named from out/phase2/results/ai_02.json)
// address 0x420b10, size 376 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x420b10..0x420c87 (all seven constants).)
// evidence: out/phase2/results/ai_02.json -- pure geometry helper operating on four
//   register-passed vectors: computes a 2D perpendicular basis from cover_direction, projects
//   the threat-to-candidate offset onto it, and tests the result against cos(30 deg) =
//   0.8660254 and squared-distance bands (0.36 = 0.6^2, 1.21 = 1.1^2) to classify a candidate
//   flank position, returning 0 (invalid), 1 or 2 (valid, two quality grades).
// register convention: ECX -> cover_direction, EBX -> out_offset (nullable), ESI ->
//   threat_position, EDI -> candidate_position; no stack parameters.
//   // blam-cc: ECX -> cover_direction, EBX -> out_offset, ESI -> threat_position, EDI -> candidate_position
//
// UNSURE: every return path packs the upper 16 bits of EAX with leftover bits from an
// unrelated float-to-int reinterpretation (`(ushort)((uint)fVar2 >> 0x10)`); only the low
// 16 bits (0, 1 or 2) are ever tested by callers, and that is what this rewrite returns.
// UNSURE: field/parameter names (cover_direction, out_offset, threat_position,
// candidate_position) describe the geometry, not confirmed semantics; no caller in this batch
// is included to cross-check them against.

#include "tags.h"
#include "memory.h"
#include "math.h"

// sqrt/fabs are single x87 instructions (FSQRT/FABS) in the original code, which
// Ghidra renders as the pseudo-functions SQRT()/ABS(); declared locally instead of via
// <math.h> because -I types shadows that header name with types/math.h.
extern double sqrt(double x); // FSQRT
static float sqrt_f(float x) { return (float)sqrt((double)x); }
extern double fabs(double x); // FABS
static float fabs_f(float x) { return (float)fabs((double)x); }

// blam-cc: ECX -> cover_direction, EBX -> out_offset, ESI -> threat_position, EDI -> candidate_position
// Classifies whether a candidate side/flank position relative to a threat and cover direction
// is usable, returning a small validity code (0 invalid, 1 or 2 valid).
int16_t actor_evaluate_flank_offset(real_vector3d *cover_direction, real_vector3d *out_offset,
                                    real_point3d *threat_position, real_point3d *candidate_position)
{
    float cover_len;
    float dx, dy;
    float projection;
    float offset_x, offset_y, offset_z;
    float horizontal_sq;
    int16_t grade;

    cover_len = sqrt_f(cover_direction->j * cover_direction->j + cover_direction->i * cover_direction->i);

    if (0.0001f <= fabs_f(cover_len) && 0.0f < cover_len) {
        dx = threat_position->x - candidate_position->x;
        dy = threat_position->y - candidate_position->y;
        projection = dy * (1.0f / cover_len) * cover_direction->j + dx * cover_direction->i * (1.0f / cover_len);

        if (sqrt_f(dx * dx + dy * dy) * 0.8660254f < projection) {
            projection = -projection;
            offset_x = projection * cover_direction->i + dx;
            offset_y = projection * cover_direction->j + dy;
            offset_z = projection * cover_direction->k + (threat_position->z - candidate_position->z);

            if (out_offset != (real_vector3d *)0) {
                out_offset->i = -offset_x;
                out_offset->j = -offset_y;
                out_offset->k = -offset_z;
            }

            if (offset_z <= -0.5f || 0.9f <= offset_z) {
                if (offset_z <= -0.8f) {
                    return 0;
                }
                if (1.2f <= offset_z) {
                    return 0;
                }
                grade = 1;
            } else {
                grade = 2;
            }

            horizontal_sq = offset_x * offset_x + offset_y * offset_y;
            if (horizontal_sq < 0.36f) {
                return grade;
            }
            if (1.21f <= horizontal_sq) {
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x420b10):

int FUN_00420b10(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  ushort uVar6;
  undefined2 uVar7;
  float *in_ECX;
  float *unaff_EBX;
  float *unaff_ESI;
  float *unaff_EDI;

  fVar1 = *in_ECX;
  fVar2 = in_ECX[1];
  fVar3 = SQRT(fVar2 * fVar2 + fVar1 * fVar1);
  uVar6 = (ushort)((uint)fVar2 >> 0x10);
  if (0.0001 <= ABS(fVar3)) {
    if (0.0 < fVar3) {
      fVar4 = *unaff_ESI - *unaff_EDI;
      fVar5 = unaff_ESI[1] - unaff_EDI[1];
      fVar1 = fVar5 * (1.0 / fVar3) * fVar2 + fVar4 * fVar1 * (1.0 / fVar3);
      if (SQRT(fVar4 * fVar4 + fVar5 * fVar5) * 0.8660254 < fVar1) {
        fVar1 = -fVar1;
        fVar4 = fVar1 * *in_ECX + fVar4;
        fVar5 = fVar1 * in_ECX[1] + fVar5;
        fVar1 = fVar1 * in_ECX[2] + (unaff_ESI[2] - unaff_EDI[2]);
        if (unaff_EBX != (float *)0x0) {
          *unaff_EBX = -fVar4;
          unaff_EBX[1] = -fVar5;
          unaff_EBX[2] = -fVar1;
        }
        if ((fVar1 <= -0.5) || (0.9 <= fVar1)) {
          if (fVar1 <= -0.8) {
            return 0;
          }
          if (1.2 <= fVar1) {
            return 0;
          }
          uVar7 = 1;
        }
        else {
          uVar7 = 2;
        }
        fVar1 = fVar4 * fVar4 + fVar5 * fVar5;
        if (fVar1 < 0.36) {
          return CONCAT22(uVar6,uVar7);
        }
        if (1.21 <= fVar1) {
          return 0;
        }
        return 1;
      }
    }
  }
  return (uint)uVar6 << 0x10;
}
#endif
