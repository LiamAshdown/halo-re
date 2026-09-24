// ai_search_find_circle_tangent_point  (Ghidra: ai_search_find_circle_tangent_point, renamed)
// address 0x43cf60, size 415 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: phase-4 summary "finds the tangent point where a path must bend around a
// circular navmesh obstacle vertex, on the requested side." PTR_DAT_006966ec is a constant
// 2D direction the fallback path uses when the two input points coincide.
// register convention: ECX -> center, EDX -> target, ESI -> out_point; stack -> radius, side.
//   // blam-cc: ECX -> center, EDX -> target, ESI -> out_point, stack -> radius, side

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern real_point2d *ai_default_2d_direction; // 0x006966ec, UNSURE: fallback direction constant

extern double sqrt(double x); // FSQRT
extern double fabs(double x); // ABS

// blam-cc: ECX -> center, EDX -> target, ESI -> out_point, stack -> radius, side
void ai_search_find_circle_tangent_point(real_point2d *center, real_point2d *target, real_point2d *out_point,
                                         float radius, uint8_t side)
{
    float dx = target->x - center->x;
    float dy = target->y - center->y;
    float dist2 = dy * dy + dx * dx;
    float inv = radius / dist2;
    float discriminant = dist2 - radius * radius;

    if (0.0f < discriminant) {
        float root = (float)sqrt(discriminant);
        float tangent_pts[4];
        float cross_dy = dy * root;
        float cross_dx = dx * root;
        uint32_t which;

        tangent_pts[0] = (dx * radius + cross_dy) * inv + center->x;
        tangent_pts[1] = (dy * radius - cross_dx) * inv + center->y;
        tangent_pts[2] = (dx * radius - cross_dy) * inv + center->x;
        tangent_pts[3] = (cross_dx + dy * radius) * inv + center->y;

        which = (0.0f < (tangent_pts[3] - target->y) * (tangent_pts[0] - target->x) -
                         (tangent_pts[2] - target->x) * (tangent_pts[1] - target->y)) != (side != 0);

        out_point->x = tangent_pts[which * 2];
        out_point->y = tangent_pts[which * 2 + 1];
        return;
    }

    {
        float len = (float)sqrt(dx * dx + dy * dy);
        if ((0.0001 <= fabs(len)) && (len != 0.0f)) {
            dx = (1.0f / len) * dx;
            dy = dy * (1.0f / len);
        } else {
            dx = ai_default_2d_direction->x;
            dy = ai_default_2d_direction->y;
        }
        out_point->x = dx * radius + center->x;
        out_point->y = dy * radius + center->y;
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043cf60 @ 0x43cf60) ----
void FUN_0043cf60(float param_1,char param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_ECX;
  float *in_EDX;
  uint uVar4;
  float *unaff_ESI;
  float local_18;
  float local_14;
  float local_10 [4];

  local_18 = *in_EDX - *in_ECX;
  local_14 = in_EDX[1] - in_ECX[1];
  fVar1 = local_14 * local_14 + local_18 * local_18;
  fVar2 = param_1 / fVar1;
  fVar1 = fVar1 - param_1 * param_1;
  if (0.0 < fVar1) {
    fVar1 = SQRT(fVar1);
    fVar3 = local_14 * fVar1;
    local_10[0] = (local_18 * param_1 + fVar3) * fVar2 + *in_ECX;
    fVar1 = local_18 * fVar1;
    local_10[1] = (local_14 * param_1 - fVar1) * fVar2 + in_ECX[1];
    local_10[2] = (local_18 * param_1 - fVar3) * fVar2 + *in_ECX;
    local_10[3] = (fVar1 + local_14 * param_1) * fVar2 + in_ECX[1];
    uVar4 = (uint)(0.0 < (local_10[3] - in_EDX[1]) * (local_10[0] - *in_EDX) -
                         (local_10[2] - *in_EDX) * (local_10[1] - in_EDX[1]) != (bool)param_2);
    fVar1 = local_10[uVar4 * 2 + 1];
    *unaff_ESI = local_10[uVar4 * 2];
    unaff_ESI[1] = fVar1;
    return;
  }
  fVar1 = SQRT(local_18 * local_18 + local_14 * local_14);
  if (0.0001 <= ABS(fVar1)) {
    local_18 = (1.0 / fVar1) * local_18;
    local_14 = local_14 * (1.0 / fVar1);
    if (fVar1 != 0.0) goto LAB_0043d0e1;
  }
  local_18 = *(float *)PTR_DAT_006966ec;
  local_14 = *(float *)(PTR_DAT_006966ec + 4);
LAB_0043d0e1:
  *unaff_ESI = local_18 * param_1 + *in_ECX;
  unaff_ESI[1] = local_14 * param_1 + in_ECX[1];
  return;
}
#endif
