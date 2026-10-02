// ai_search_step  (Ghidra: ai_search_step, renamed)
// address 0x43bcb0, size 354 bytes
// name confidence: 0.45  rewrite confidence: 0.85
// REWRITTEN from objdump 0x43bcb0..0x43be11 (the draft lost every callee's arguments). EAX = context.
//   Pops the cheapest node (the last heap entry moves to the top and sifts down, ECX context, DX 0) and runs
//   ai_search_evaluate_edge_cost from it toward the origin: map +0x0c, ignore +0x04, obstacles +0x08, no
//   exclusion, the node's position and surface, radius +0x00, base cost = the node's length, the direct test
//   skipped for the root (no parent), apply_offset 1, require_unflagged +0x2a, direction = the node's direction.
//   A wall ends the step. With nothing in the way the origin is reached -- when the end surface is not the
//   target surface (+0x18) only if the heights there are close (0x43d910 at the origin) -- and a node on it
//   (position + direction * cost, base cost = the node's inherited cost + cost) becomes the result (+0x1e).
//   Hitting an obstacle point: a point whose link is the goal (+0x1c) records the node as best when its length
//   beats +0x24, then the point's neighbours are expanded. Returns 1 while there is no result and the heap is
//   not empty.
// blam-cc: EAX -> context

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h> // uintptr_t

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void ai_search_heap_sift_down(ai_search_context *context, int16_t index); // 0x43b4d0, ECX, DX
extern void ai_search_expand_point_neighbors(ai_search_context *context, int16_t node_index,
    int16_t start_point_id); // 0x43ba60, stack, DX
extern uint8_t path_find_heights_are_close(ScenarioStructureBSP *structure_bsp, real_point2d *point, int32_t surface_a,
    int32_t surface_b); // 0x43d910; EAX structure_bsp, EDX point, ECX surface_a, stack surface_b
extern int16_t ai_search_add_node(ai_search_context *context, int16_t parent, real_point2d *position,
    int32_t surface_index, int16_t point_id, uint8_t side, float base_cost); // 0x43b5a0, EDI, BX, stack
extern uint8_t ai_search_evaluate_edge_cost(void *context, uint8_t ignore_permission,
    ai_search_obstacle_list *obstacle_list, int16_t exclude_index, real_point2d *point, int32_t start_surface_index,
    float distance, float base_cost, uint8_t skip_direct, uint8_t apply_offset, uint8_t require_unflagged,
    ai_search_edge_result *out_result, real_vector2d *direction); // 0x43b830, EBX direction, stack

uint8_t ai_search_step(ai_search_context *context)
{
    if (context->heap_count > 0) {
        int16_t index;

        context->heap_count--;
        index = context->heap[0];
        context->heap[0] = context->heap[context->heap_count];
        ai_search_heap_sift_down(context, 0);
        if (index != -1) {
            ai_search_node *node = &context->nodes[index];
            ai_search_edge_result edge;

            ai_search_evaluate_edge_cost((void *)(uintptr_t)context->structure_bsp, context->unknown_04,
                (ai_search_obstacle_list *)(uintptr_t)context->obstacles, -1, &node->position, *(int32_t *)&node->z,
                *(float *)&context->search_radius, node->length, (uint8_t)(node->parent == -1), 1, context->ignore_flagged_obstacles,
                &edge, &node->direction);
            if (edge.edge_index == -1) {
                if (edge.point_id == -1) {
                    // 0x43bd5d: the origin is in reach
                    if (edge.surface_index == (int32_t)context->origin_surface_index ||
                        path_find_heights_are_close((ScenarioStructureBSP *)(uintptr_t)context->structure_bsp,
                            &context->origin, (int32_t)context->origin_surface_index, edge.surface_index)) {
                        real_point2d position;

                        position.x = edge.cost * node->direction.i + node->position.x;
                        position.y = edge.cost * node->direction.j + node->position.y;
                        context->result_node = ai_search_add_node(context, index, &position, edge.surface_index, -1, 0,
                            (node->cost - node->length) + edge.cost);
                    }
                } else {
                    if (edge.link == context->goal_point_id && node->length < context->best_cost) {
                        context->best_cost = node->length;
                        context->best_node = index;
                    }
                    ai_search_expand_point_neighbors(context, index, edge.point_id);
                }
            }
        }
    }
    return (uint8_t)(context->result_node == -1 && context->heap_count > 0);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
