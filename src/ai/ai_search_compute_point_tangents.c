// ai_search_compute_point_tangents  (Ghidra: ai_search_compute_point_tangents, renamed)
// address 0x43c9a0, size 149 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: types/ai.h ai_search_obstacle_list.obstacles(+0x08, stride 0x14) and
// ai_search_obstacle.position(+0x08)/radius(+0x10). phase-4 summary "computes tangent offset
// directions for steering around a specific point-array entry, used while scoring candidate
// path edges." Calls vector2d_tangent_edge_directions (a math helper this task's skip list excludes from
// rewriting, "computes the two tangent directions from an offset to a circle...").
// register convention: ECX -> list, AX -> point_index, EDX -> position; stack -> radius,
//   out_a, out_b.
//   // blam-cc: ECX -> list, EAX(low16) -> point_index, EDX -> position, stack -> radius,
//   //   out_a, out_b

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern double sqrt(double x); // FSQRT
extern double fabs(double x); // ABS
extern void vector2d_tangent_edge_directions(float combined_radius, float total_radius, void *out_b, float distance,
                         float dir_x, float dir_y); // 0x43c400, math helper, not rewritten here

// blam-cc: ECX -> list, EAX(low16) -> point_index, EDX -> position, stack -> radius, out_a, out_b
void ai_search_compute_point_tangents(ai_search_obstacle_list *list, int16_t point_index, real_point2d *position,
                                      float radius, void *out_a, void *out_b)
{
    ai_search_obstacle *point = &list->obstacles[point_index];
    float dx = point->position.x - position->x;
    float dy = point->position.y - position->y;
    float distance = (float)sqrt(dx * dx + dy * dy);

    if (fabs(distance) < 0.0001) {
        distance = 0.0f;
    } else {
        dx = dx * (1.0f / distance);
        dy = dy * (1.0f / distance);
    }

    vector2d_tangent_edge_directions(distance, radius + point->radius + 0.00390625f, out_b, distance, dx, dy);
    (void)out_a;
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
