// ai_search_choose_shorter_corner  (Ghidra: ai_search_choose_shorter_corner, renamed)
// address 0x43d240, size 614 bytes
// name confidence: 0.35  rewrite confidence: 0.85
// VERIFIED against disassembly 0x43d240..0x43d4a5 (2026-09-30); rewritten from it (the draft had lost ECX, EDX and the fifth argument). ECX = point p,
//   EBX = corner A, EDX = point q; stack: corner B, point r, out. The unit directions (normalised only when longer
//   than 0.0001) from each corner to p, q and r give the turn around that corner as the sum of the signed angles
//   (vector2d_angle_between, ESI a, EDI b) r->q and q->p. Corner A is taken (copied to out, returns 1) when
//   -turn(B) < turn(A); otherwise corner B (returns 0).
// blam-cc: ECX -> p, EBX -> corner_a, EDX -> q, stack -> corner_b, r, out_point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double sqrt(double x); // FSQRT
extern double fabs(double x); // FABS
extern real vector2d_angle_between(real_vector2d *a, real_vector2d *b); // 0x4cd480, ESI a, EDI b

static void ai_search_corner_direction(const real_point2d *point, const real_point2d *corner, real_vector2d *out)
{
    float length;

    out->i = point->x - corner->x;
    out->j = point->y - corner->y;
    length = (float)sqrt(out->j * out->j + out->i * out->i);
    if (!((float)fabs(length) < 0.0001f)) {
        float scale = 1.0f / length;

        out->i *= scale;
        out->j *= scale;
    }
}

uint8_t ai_search_choose_shorter_corner(real_point2d *p, real_point2d *corner_a, real_point2d *q,
    real_point2d *corner_b, real_point2d *r, real_point2d *out_point)
{
    real_vector2d a_p;
    real_vector2d a_q;
    real_vector2d a_r;
    real_vector2d b_p;
    real_vector2d b_q;
    real_vector2d b_r;
    float turn_a;
    float turn_b;

    ai_search_corner_direction(p, corner_a, &a_p);
    ai_search_corner_direction(q, corner_a, &a_q);
    ai_search_corner_direction(r, corner_a, &a_r);
    ai_search_corner_direction(p, corner_b, &b_p);
    ai_search_corner_direction(q, corner_b, &b_q);
    ai_search_corner_direction(r, corner_b, &b_r);
    turn_a = vector2d_angle_between(&a_r, &a_q);
    turn_a = vector2d_angle_between(&a_q, &a_p) + turn_a;
    turn_b = vector2d_angle_between(&b_r, &b_q);
    turn_b = vector2d_angle_between(&b_q, &b_p) + turn_b;
    if (-turn_b < turn_a) {
        *out_point = *corner_a;
        return 1;
    }
    *out_point = *corner_b;
    return 0;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043d240 @ 0x43d240) ----
undefined4 FUN_0043d240(undefined4 *param_1,undefined4 param_2,undefined4 *param_3)

{
  undefined4 *unaff_EBX;
  float10 fVar1;
  float10 fVar2;
  float10 fVar3;
  float10 fVar4;

  fVar1 = (float10)vector2d_angle_between();
  fVar2 = (float10)vector2d_angle_between();
  fVar3 = (float10)vector2d_angle_between();
  fVar4 = (float10)vector2d_angle_between();
  if (-(fVar4 + (float10)(float)fVar3) < (float10)(float)(fVar2 + (float10)(float)fVar1)) {
    *param_3 = *unaff_EBX;
    param_3[1] = unaff_EBX[1];
    return 1;
  }
  *param_3 = *param_1;
  param_3[1] = param_1[1];
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
