// ai_search_context_init  (Ghidra: ai_search_context_init, renamed)
// address 0x43b790, size 157 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (verified against objdump)
// evidence: types/ai.h ai_search_context, every field written here matches the header
// exactly (obstacles, origin, goal_point_id via obstacle[goal].link, result_node, best_cost,
// best_node, node_count, heap_count via the +0x1430/dword-0x50c cross-check the header
// itself documents). Calls ai_search_find_covering_point (ai_search_find_covering_point, this rewrite) and
// ai_search_add_node @0x43b5a0 (this rewrite) to push the initial node.
// register convention: stack -> context, unknown_04, unknown_00, position, z, weighted_actor_count,
//   unknown_29, weighted_actor_count; ECX -> unknown_0c, EAX -> obstacles, EDX -> origin (a
//   real_point2d).
//   // blam-cc: ECX -> unknown_0c, EAX -> obstacles, EDX -> origin, stack -> context,
//   //   unknown_04, unknown_00, position, z, weighted_actor_count, unknown_29, weighted_actor_count

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>

extern int16_t ai_search_find_covering_point(ai_search_obstacle_list *list, real_point2d *position,
                                             int16_t exclude_index, float extra_radius); // 0x43c890
extern int16_t ai_search_add_node(ai_search_context *context, int16_t chain_head, real_point2d *position,
                                  int32_t surface_index, int16_t point_id, uint8_t side, float extra_cost); // 0x43b5a0

// blam-cc: ECX -> unknown_0c, EAX -> obstacles, EDX -> origin, stack -> context, unknown_04,
//   unknown_00, position, z, weighted_actor_count, unknown_29, weighted_actor_count
void ai_search_context_init(ai_search_context *context, uint8_t unknown_04, uint32_t unknown_00,
                            ai_search_obstacle_list *obstacles, real_point2d *origin, uint32_t unknown_0c,
                            real_point2d *position, int32_t surface_index, uint32_t weighted_actor_count,
                            uint8_t unknown_29, uint8_t weighted_actor_count)
{
    int16_t covering_point;

    context->unknown_00 = unknown_00;
    context->unknown_0c = unknown_0c;
    context->unknown_04 = unknown_04;
    context->obstacles = (uint32_t)(uintptr_t)obstacles;
    context->complete = 0;
    context->origin = *origin;
    context->weighted_actor_count = weighted_actor_count;

    // UNSURE: ai_search_obstacle_list.unknown_00 is declared uint32_t in types/ai.h, but
    // ai_search_find_covering_point.c reads this same value back as a float radius; another
    // same-offset type disagreement between functions (see path_find_compute_heuristic.c for
    // the same situation on a different struct).
    covering_point = ai_search_find_covering_point(obstacles, origin, -1, *(float *)&unknown_00);
    context->goal_point_id = (covering_point == -1) ? -1 : obstacles->obstacles[covering_point].link;

    context->unknown_29 = unknown_29;
    context->weighted_actor_count = weighted_actor_count;
    context->result_node = -1;
    context->best_cost = 3.4028235e+38f;
    context->best_node = -1;
    context->node_count = 0;
    context->heap_count = 0;

    ai_search_add_node(context, -1, position, surface_index, -1, 0, 0.0f); // "z" is the start surface
}

#if 0
// ---- original Ghidra decompilation (FUN_0043b790 @ 0x43b790) ----
void FUN_0043b790(undefined4 *param_1,undefined1 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5,undefined4 param_6,undefined1 param_7,undefined1 param_8)

{
  short sVar1;
  undefined2 uVar2;
  int in_EAX;
  undefined4 in_ECX;
  undefined4 *in_EDX;

  *param_1 = param_3;
  param_1[3] = in_ECX;
  *(undefined1 *)(param_1 + 1) = param_2;
  param_1[2] = in_EAX;
  *(undefined1 *)(param_1 + 10) = 0;
  param_1[4] = *in_EDX;
  param_1[5] = in_EDX[1];
  param_1[6] = param_6;
  sVar1 = FUN_0043c890(param_3);
  if (sVar1 == -1) {
    uVar2 = 0xffff;
  }
  else {
    uVar2 = *(undefined2 *)(in_EAX + 10 + sVar1 * 0x14);
  }
  *(undefined2 *)(param_1 + 7) = uVar2;
  *(undefined1 *)((int)param_1 + 0x2a) = param_8;
  *(undefined1 *)((int)param_1 + 0x29) = param_7;
  *(undefined2 *)((int)param_1 + 0x1e) = 0xffff;
  param_1[9] = 0x7f7fffff;
  *(undefined2 *)(param_1 + 8) = 0xffff;
  *(undefined2 *)(param_1 + 0xb) = 0;
  *(undefined2 *)(param_1 + 0x50c) = 0;
  FUN_0043b5a0(param_4,param_5,0xffffffff,0,0);
  return;
}
#endif
