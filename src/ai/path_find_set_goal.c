// path_find_set_goal  (Ghidra: path_find_set_goal, renamed)
// address 0x43a730, size 40 bytes
// name confidence: 0.55  rewrite confidence: 0.9 (VERIFIED against the binary (register/stack mapping and struct offsets asserted))
// evidence: types/ai.h path_find_context.have_goal (+0x4c) / goal_position (+0x50) /
//   goal_vertex_id (+0x5c) / goal_cost (+0x60) -- this function writes exactly those four
//   fields and nothing else, which is the strongest possible match for their names.
// register convention: EAX -> context, ECX -> position (a real_point3d the caller owns).
//   // blam-cc: EAX -> context, ECX -> position, stack -> goal_vertex_id, goal_cost

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

// blam-cc: EAX -> context, ECX -> position, stack -> goal_vertex_id, goal_cost
void path_find_set_goal(path_find_context *context, const real_point3d *position,
                        uint32_t goal_vertex_id, float goal_cost)
{
    context->have_goal = 1;
    context->goal_position = *position;
    context->goal_vertex_id = goal_vertex_id;
    context->goal_cost = goal_cost;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043a730 @ 0x43a730) ----
void FUN_0043a730(undefined4 param_1,undefined4 param_2)

{
  int in_EAX;
  undefined4 *in_ECX;

  *(undefined1 *)(in_EAX + 0x4c) = 1;
  *(undefined4 *)(in_EAX + 0x50) = *in_ECX;
  *(undefined4 *)(in_EAX + 0x54) = in_ECX[1];
  *(undefined4 *)(in_EAX + 0x58) = in_ECX[2];
  *(undefined4 *)(in_EAX + 0x5c) = param_1;
  *(undefined4 *)(in_EAX + 0x60) = param_2;
  return;
}
#endif
