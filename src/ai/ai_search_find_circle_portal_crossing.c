// ai_search_find_circle_portal_crossing  (Ghidra: ai_search_find_circle_portal_crossing, renamed)
// address 0x43d100, size 308 bytes
// name confidence: 0.35  rewrite confidence: 0.85 (verified against objdump 0x43d100..0x43d233)
// evidence: phase-4 summary "computes where a given-radius circle crosses a portal edge,
// falling back to a direction-based offset if the edge geometry is degenerate."
// register convention: ECX -> center, EDX -> portal (two 2D points), ESI -> out_point, EDI ->
//   fallback_reference; stack -> radius.
//   // blam-cc: ECX -> center, EDX -> portal, ESI -> out_point, EDI -> fallback_reference,
//   //   stack -> radius

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern real_point2d *ai_default_2d_direction; // 0x006966ec, UNSURE: same fallback constant as ai_search_find_circle_tangent_point.c

extern double sqrt(double x); // FSQRT
extern double fabs(double x); // ABS

// blam-cc: ECX -> center, EDX -> portal, ESI -> out_point, EDI -> fallback_reference, stack -> radius
void ai_search_find_circle_portal_crossing(real_point2d *center, real_point2d *portal, real_point2d *out_point,
                                           real_point2d *fallback_reference, float radius)
{
    float denom = (portal[1].y - center->y) * (portal[0].x - center->x) -
                  (portal[0].y - center->y) * (portal[1].x - center->x);

    if (0.0001 <= fabs(denom)) {
        float scale = (radius * radius) / denom;
        float y = ((portal[0].x - center->x) - (portal[1].x - center->x)) * scale + center->y;
        out_point->x = center->x - ((portal[0].y - center->y) - (portal[1].y - center->y)) * scale;
        out_point->y = y;

        {
            float dy = y - center->y;
            if ((out_point->x - center->x) * (out_point->x - center->x) + dy * dy <= radius * radius * 4.0f) {
                return;
            }
        }
    }

    {
        float dx = portal[0].x - fallback_reference->x;
        float dy = portal[0].y - fallback_reference->y;
        float len = (float)sqrt(dx * dx + dy * dy);
        if ((0.0001 <= fabs(len)) && (len != 0.0f)) {
            dx = dx * (1.0f / len);
            dy = dy * (1.0f / len);
        } else {
            dx = ai_default_2d_direction->x;
            dy = ai_default_2d_direction->y;
        }
        out_point->x = dx * radius + portal[0].x;
        out_point->y = dy * radius + portal[0].y;
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043d100 @ 0x43d100) ----
void FUN_0043d100(float param_1)

{
  float fVar1;
  float fVar2;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;
  float *unaff_EDI;
  float local_10;
  float local_c;

  fVar1 = (in_EDX[3] - in_ECX[1]) * (*in_EDX - *in_ECX) -
          (in_EDX[1] - in_ECX[1]) * (in_EDX[2] - *in_ECX);
  if (0.0001 <= ABS(fVar1)) {
    fVar1 = (param_1 * param_1) / fVar1;
    fVar2 = ((*in_EDX - *in_ECX) - (in_EDX[2] - *in_ECX)) * fVar1 + in_ECX[1];
    *unaff_ESI = *in_ECX - ((in_EDX[1] - in_ECX[1]) - (in_EDX[3] - in_ECX[1])) * fVar1;
    unaff_ESI[1] = fVar2;
    fVar2 = fVar2 - in_ECX[1];
    if ((*unaff_ESI - *in_ECX) * (*unaff_ESI - *in_ECX) + fVar2 * fVar2 <= param_1 * param_1 * 4.0)
    {
      return;
    }
  }
  local_10 = *in_EDX - *unaff_EDI;
  local_c = in_EDX[1] - unaff_EDI[1];
  fVar1 = SQRT(local_10 * local_10 + local_c * local_c);
  if (0.0001 <= ABS(fVar1)) {
    local_10 = local_10 * (1.0 / fVar1);
    local_c = local_c * (1.0 / fVar1);
    if (fVar1 != 0.0) goto LAB_0043d216;
  }
  local_10 = *(float *)PTR_DAT_006966ec;
  local_c = *(float *)(PTR_DAT_006966ec + 4);
LAB_0043d216:
  *unaff_ESI = local_10 * param_1 + *in_EDX;
  unaff_ESI[1] = local_c * param_1 + in_EDX[1];
  return;
}
#endif
