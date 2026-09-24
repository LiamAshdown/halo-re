// ai_search_step  (Ghidra: ai_search_step, renamed)
// address 0x43bcb0, size 354 bytes
// name confidence: 0.4   rewrite confidence: 0.15
// evidence: types/ai.h ai_search_context (heap_count via the +0x1430/dword-0x50c
// cross-check, heap +0x1432, nodes +0x30 stride 0x28, goal_point_id +0x1c, best_node +0x20,
// best_cost +0x24, result_node +0x1e) and ai_search_node (position/z/direction/length/
// point_id/cost/parent), all confirmed by this function's own dword-scaled indexing
// matching the header's byte offsets exactly (e.g. `in_EAX + sVar2*10 + 0xc` is
// `0x30 + sVar2*0x28`). Calls ai_search_heap_sift_down (0x43b4d0), ai_search_evaluate_edge_cost
// (0x43b830, itself very low confidence), ai_search_add_node (0x43b5a0), path_find_heights_are_close
// (0x43d910, this rewrite) and ai_search_expand_point_neighbors (0x43ba60, this rewrite,
// called here with no visible arguments -- see its own file for the same gap). phase-4
// summary "performs one iteration of the AI point search: pop the cheapest open node and
// either connect it to the goal or expand its neighbors."
//
// Kept close to the Ghidra decompilation and at low confidence given how much of it forwards
// into ai_search_evaluate_edge_cost, which is itself one of this batch's least reliable
// rewrites.
//
// register convention: EAX -> context (the only recognized operand).
//   // blam-cc: EAX -> context
// reconciled: R53 ai_search_evaluate_edge_cost declared with its real signature (13 params, EBX = direction = node+0xc);
//   the uninitialised locals local_c/local_8/local_4/sStack_2 were the fields of its 0x10-byte result record (edge.*)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h> // uintptr_t

extern void ai_search_heap_sift_down(void *context, int16_t index); // 0x43b4d0
extern void ai_search_expand_point_neighbors(void); // 0x43ba60, called here with no visible arguments; see that file
extern uint8_t path_find_heights_are_close(ScenarioStructureBSP *structure_bsp, real_point2d *point, int32_t surface_a,
    int32_t surface_b); // 0x43d910; blam-cc: EAX structure_bsp, EDX point, ECX surface_a, stack surface_b
extern int32_t ai_search_add_node(real_point2d *position, int32_t point_id, uint32_t param3, int32_t param4, float extra_cost); // 0x43b5a0, see header UNSURE
extern uint8_t ai_search_evaluate_edge_cost(void *context, uint8_t ignore_permission,
                                            ai_search_obstacle_list *obstacle_list, int16_t exclude_index,
                                            real_point2d *point, int32_t start_surface_index, float distance,
                                            float base_cost, uint8_t skip_direct, uint8_t apply_offset,
                                            uint8_t require_unflagged, ai_search_edge_result *out_result,
                                            real_vector2d *direction);
    // 0x43b830; EBX -> direction (see src/ai/ai_search_evaluate_edge_cost.c)

// blam-cc: EAX -> context
uint8_t ai_search_step(uint32_t *context)
{
    float local_18, local_14;
    ai_search_edge_result edge; // [esp+0x18]: Ghidra's local_10/local_c/local_8/local_4/sStack_2

    if (0 < *(int16_t *)(context + 0x50c)) {
        int16_t heap_count = *(int16_t *)(context + 0x50c) - 1;
        int16_t popped;
        *(int16_t *)(context + 0x50c) = heap_count;

        popped = *(int16_t *)((uint8_t *)context + 0x1432);
        *(int16_t *)((uint8_t *)context + 0x1432) = *(int16_t *)((uint8_t *)context + heap_count * 2 + 0x1432);
        ai_search_heap_sift_down(context, 0);

        if (popped != -1) {
            float *node = (float *)(context + popped * 10 + 0xc);
            uint8_t reached;

            // 0x43bd0b..0x43bd3e: EBX = node+0xc (direction), 12 stack arguments, add esp,0x30
            reached = ai_search_evaluate_edge_cost((void *)context[3], *(uint8_t *)(context + 1),
                                                   (ai_search_obstacle_list *)context[2], -1,
                                                   (real_point2d *)node, *(int32_t *)&node[2], *(float *)context,
                                                   node[5], (*(int16_t *)(node + 9) == -1) ? 1 : 0, 1,
                                                   *((uint8_t *)context + 0x2a), &edge,
                                                   (real_vector2d *)(node + 3));
            (void)reached;

            if (edge.edge_index == -1) {
                if (edge.point_id == -1) {
                    if ((edge.surface_index == (int32_t)context[6]) || 
                        // 0x43bd5d..0x43bd6f: EAX = context[3], EDX = &context[4], ECX = context[6]
                        (path_find_heights_are_close((ScenarioStructureBSP *)(uintptr_t)context[3],
                             (real_point2d *)(context + 4), (int32_t)context[6], edge.surface_index) != 0)) {
                        int32_t new_node;
                        local_18 = edge.cost * node[3] + *node;
                        local_14 = edge.cost * node[4] + node[1];
                        {
                            real_point2d p; p.x = local_18; p.y = local_14;
                            new_node = ai_search_add_node(&p, edge.surface_index, 0xffffffff, 0, (node[8] - node[5]) + edge.cost);
                        }
                        *(int16_t *)((uint8_t *)context + 0x1e) = (int16_t)new_node;
                    }
                } else {
                    if ((edge.link == *(int16_t *)(context + 7)) && (node[5] < *(float *)(context + 9))) {
                        *(float *)(context + 9) = node[5];
                        *(int16_t *)(context + 8) = popped;
                    }
                    ai_search_expand_point_neighbors();
                }
            }
        }
    }

    return (*(int16_t *)((uint8_t *)context + 0x1e) == -1) && (0 < *(int16_t *)(context + 0x50c));
}

#if 0
// ---- original Ghidra decompilation (FUN_0043bcb0 @ 0x43bcb0) ----
undefined4 FUN_0043bcb0(void)

{
  float *pfVar1;
  short sVar2;
  char cVar3;
  short sVar4;
  undefined2 uVar5;
  undefined4 *in_EAX;
  float local_18;
  float local_14;
  float local_10;
  int local_c;
  int local_8;
  undefined4 local_4;
  short sStack_2;

  if (0 < *(short *)(in_EAX + 0x50c)) {
    sVar4 = *(short *)(in_EAX + 0x50c) + -1;
    *(short *)(in_EAX + 0x50c) = sVar4;
    sVar2 = *(short *)((int)in_EAX + 0x1432);
    *(undefined2 *)((int)in_EAX + 0x1432) = *(undefined2 *)((int)in_EAX + sVar4 * 2 + 0x1432);
    ai_search_heap_sift_down();
    if (sVar2 != -1) {
      pfVar1 = (float *)(in_EAX + sVar2 * 10 + 0xc);
      FUN_0043b830(in_EAX[3],*(undefined1 *)(in_EAX + 1),in_EAX[2],0xffffffff,pfVar1,pfVar1[2],
                   *in_EAX,pfVar1[5],
                   CONCAT31((int3)((uint)(sVar2 * 5) >> 8),*(short *)(pfVar1 + 9) == -1),1,
                   *(undefined1 *)((int)in_EAX + 0x2a),&local_10);
      if (local_8 == -1) {
        if ((short)local_4 == -1) {
          if ((local_c == in_EAX[6]) || (cVar3 = FUN_0043d910(local_c), cVar3 != '\0')) {
            local_18 = local_10 * pfVar1[3] + *pfVar1;
            local_14 = local_10 * pfVar1[4] + pfVar1[1];
            uVar5 = FUN_0043b5a0(&local_18,local_c,0xffffffff,0,(pfVar1[8] - pfVar1[5]) + local_10);
            *(undefined2 *)((int)in_EAX + 0x1e) = uVar5;
          }
        }
        else {
          if ((sStack_2 == *(short *)(in_EAX + 7)) && (pfVar1[5] < (float)in_EAX[9])) {
            in_EAX[9] = pfVar1[5];
            *(short *)(in_EAX + 8) = sVar2;
          }
          FUN_0043ba60();
        }
      }
    }
  }
  if ((*(short *)((int)in_EAX + 0x1e) == -1) && (0 < *(short *)(in_EAX + 0x50c))) {
    return 1;
  }
  return 0;
}
#endif
