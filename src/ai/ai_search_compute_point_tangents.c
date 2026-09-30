// ai_search_compute_point_tangents  (Ghidra: ai_search_compute_point_tangents, renamed)
// address 0x43c9a0, size 149 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (verified against objdump)
// evidence: types/ai.h ai_search_obstacle_list.obstacles(+0x08, stride 0x14) and
// ai_search_obstacle.position(+0x08)/radius(+0x10). phase-4 summary "computes tangent offset
// directions for steering around a specific point-array entry, used while scoring candidate
// path edges." Calls vector2d_tangent_edge_directions (a math helper this task's skip list excludes from
// rewriting, "computes the two tangent directions from an offset to a circle...").
// register convention: ECX -> list, AX -> point_index, EDX -> position; stack -> radius,
//   out_a, out_b.
//   // blam-cc: ECX -> list, EAX(low16) -> point_index, EDX -> position, ESI -> edge_neg,
//   //   stack -> radius, out_a, out_b
// FIXED (register inputs, objdump): ESI carries edge_neg, the second output pointer that this
//   function forwards unchanged to vector2d_tangent_edge_directions (its own blam-cc: ECX ->
//   direction, EDX -> edge_pos, ESI -> edge_neg). The old extern for that callee also had the
//   wrong parameter count/order/types (a leftover guess); fixed to match
//   src/math/vector2d_tangent_edge_directions.c's real signature, and out_a (EDX at the call,
//   previously unused) is now threaded through as edge_pos.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"
#include "fn_math.h"

extern double sqrt(double x); // FSQRT
extern double fabs(double x); // ABS


// blam-cc: ECX -> list, EAX(low16) -> point_index, EDX -> position, ESI -> edge_neg,
//   stack -> radius, out_a, out_b
void ai_search_compute_point_tangents(ai_search_obstacle_list *list, int16_t point_index, real_point2d *position,
                                      real_vector2d *edge_neg, float radius, real_vector2d *out_a, real *out_b)
{
    ai_search_obstacle *point = &list->obstacles[point_index];
    real_vector2d direction;
    float dx = point->position.x - position->x;
    float dy = point->position.y - position->y;
    float distance = (float)sqrt(dx * dx + dy * dy);

    if (fabs(distance) < 0.0001) {
        distance = 0.0f;
    } else {
        dx = dx * (1.0f / distance);
        dy = dy * (1.0f / distance);
    }
    direction.i = dx;
    direction.j = dy;

    vector2d_tangent_edge_directions(&direction, out_a, edge_neg, distance, radius + point->radius + 0.00390625f, out_b);
}

#if 0
// ---- original Ghidra decompilation (FUN_0043c9a0 @ 0x43c9a0) ----
void FUN_0043c9a0(float param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  short in_AX;
  int in_ECX;
  float *in_EDX;
  float fVar2;
  float fVar3;
  float fVar4;

  iVar1 = in_ECX + 8 + in_AX * 0x14;
  fVar3 = *(float *)(iVar1 + 8) - *in_EDX;
  fVar4 = *(float *)(iVar1 + 0xc) - in_EDX[1];
  fVar2 = SQRT(fVar3 * fVar3 + fVar4 * fVar4);
  if (ABS(fVar2) < 0.0001) {
    fVar2 = 0.0;
  }
  else {
    fVar3 = fVar3 * (1.0 / fVar2);
    fVar4 = fVar4 * (1.0 / fVar2);
  }
  FUN_0043c400(fVar2,param_1 + *(float *)(iVar1 + 0x10) + 0.00390625,param_3,fVar2,fVar3,fVar4);
  return;
}
#endif
