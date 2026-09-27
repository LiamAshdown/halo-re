// path_find_reconstruct_path  (Ghidra: path_find_reconstruct_path, renamed)
// address 0x43a4d0, size 551 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x43a4d0..0x43a6f6 (the draft called path_find_simplify_waypoints and
//   ai_navigate_around_obstacles without arguments). EBX = context; stack: out_result.
//   out_result: +0x00 success, +0x04 end point, +0x10 end surface, +0x14 remaining distance, +0x18 path valid,
//   +0x19 waypoint count, +0x1a 0, +0x1c waypoints (path_find_waypoint).
//   Without a goal nothing is done (0). The goal's node (0x43b2b0) ends at the goal (+0x50 point, surface,
//   cost, then remaining 0); otherwise the best node (+0x68) ends at the best position (+0x74) with its surface
//   and cost, but only when the best cost is below the goal cost. The node chain is laid out by waypoint number
//   (at most 0x40; a deeper node marks the path invalid): each entry is the node's surface and the position of
//   the node after it (the end point for the last). path_find_simplify_waypoints then
//   ai_navigate_around_obstacles turn it into at most four waypoints (failure = 0); they are copied out and, for
//   a valid path, the end point / surface become the last waypoint's and the remaining distance its distance to
//   the goal.
// blam-cc: EBX -> context, stack -> out_result

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>

extern real vector3d_distance(real_point3d *a, real_point3d *b); // 0x4088b0, EAX a, ECX b
extern int16_t path_find_hash_lookup_vertex(path_find_context *context, uint32_t vertex_id); // 0x43b2b0, EDX, ESI
extern void path_find_simplify_waypoints(path_find_context *context, int16_t count, path_find_waypoint *waypoints,
    int16_t *out_count, path_find_waypoint *out_waypoints, uint8_t *out_valid); // 0x43cc00, stack
extern uint8_t ai_navigate_around_obstacles(path_find_context *context, int16_t count, path_find_waypoint *waypoints,
    int16_t *out_count, path_find_waypoint *out_waypoints, uint8_t *out_valid); // 0x43be90, stack

uint8_t path_find_reconstruct_path(path_find_context *context, uint8_t *out_result)
{
    path_find_waypoint chain[0x40];
    path_find_waypoint simplified[4];
    path_find_waypoint final_waypoints[4];
    int16_t simplified_count = 0;
    int16_t final_count = 0;
    uint8_t valid = 1;
    int16_t node_index;
    int16_t count;
    int16_t previous = -1;
    path_find_node *previous_node = 0;
    real_point3d *end_point = (real_point3d *)(out_result + 4);

    out_result[0] = 0;
    if (!context->have_goal) {
        return 0;
    }
    node_index = path_find_hash_lookup_vertex(context, context->goal_vertex_id);
    if (node_index != -1) {
        *end_point = context->goal_position;
        *(uint32_t *)(out_result + 0x10) = context->goal_vertex_id;
        *(float *)(out_result + 0x14) = 0.0f;
    } else {
        if (!(context->best_cost < context->goal_cost)) {
            return 0;
        }
        node_index = context->best_node;
        *end_point = context->best_position;
        *(uint32_t *)(out_result + 0x10) = context->nodes[node_index].vertex_id;
        *(float *)(out_result + 0x14) = context->best_cost;
    }
    if (node_index == -1) {
        return 0;
    }
    count = (int16_t)(context->nodes[node_index].waypoint + 1);
    if (count > 0x40) {
        count = 0x40;
    }
    do {
        path_find_node *node = &context->nodes[node_index];
        int16_t waypoint = node->waypoint;

        if (waypoint >= 0x40) {
            valid = 0;
        } else {
            chain[waypoint].surface_index = (int32_t)node->vertex_id;
            chain[waypoint].position = (previous == -1) ? *end_point : previous_node->position;
        }
        previous = node_index;
        previous_node = node;
        node_index = node->parent;
    } while (node_index != -1);

    path_find_simplify_waypoints(context, count, chain, &simplified_count, simplified, &valid);
    if (!ai_navigate_around_obstacles(context, simplified_count, simplified, &final_count, final_waypoints, &valid)) {
        return 0;
    }
    out_result[0x18] = valid;
    out_result[0x19] = (uint8_t)final_count;
    out_result[0] = 1;
    out_result[0x1a] = 0;
    memcpy(out_result + 0x1c, final_waypoints, (size_t)final_count * sizeof(path_find_waypoint));
    if (out_result[0x18]) {
        path_find_waypoint *last = (path_find_waypoint *)(out_result + 0x1c) + ((int8_t)out_result[0x19] - 1);

        *end_point = last->position;
        *(int32_t *)(out_result + 0x10) = last->surface_index;
        *(float *)(out_result + 0x14) = vector3d_distance(&context->goal_position, end_point);
    }
    return out_result[0];
}

#if 0
// ---- original Ghidra decompilation (FUN_0043a4d0 @ 0x43a4d0) ----
undefined1 FUN_0043a4d0(undefined1 *param_1)

{
  char cVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  undefined4 *puVar5;
  int unaff_EBX;
  short sVar6;
  undefined4 *puVar7;
  float10 extraout_ST0;
  undefined1 local_491;
  int local_488;
  undefined4 local_440 [16];
  undefined4 local_400 [256];

  *param_1 = 0;
  if (*(char *)(unaff_EBX + 0x4c) != '\0') {
    sVar2 = FUN_0043b2b0();
    if (sVar2 == -1) {
      if (*(float *)(unaff_EBX + 0x60) <= *(float *)(unaff_EBX + 0x6c)) goto LAB_0043a6ec;
      sVar2 = *(short *)(unaff_EBX + 0x68);
      *(undefined4 *)(param_1 + 4) = *(undefined4 *)(unaff_EBX + 0x74);
      *(undefined4 *)(param_1 + 8) = *(undefined4 *)(unaff_EBX + 0x78);
      *(undefined4 *)(param_1 + 0xc) = *(undefined4 *)(unaff_EBX + 0x7c);
      *(undefined4 *)(param_1 + 0x10) = *(undefined4 *)(sVar2 * 0x34 + unaff_EBX + 0x8c);
      *(undefined4 *)(param_1 + 0x14) = *(undefined4 *)(unaff_EBX + 0x6c);
    }
    else {
      *(undefined4 *)(param_1 + 4) = *(undefined4 *)(unaff_EBX + 0x50);
      *(undefined4 *)(param_1 + 8) = *(undefined4 *)(unaff_EBX + 0x54);
      *(undefined4 *)(param_1 + 0xc) = *(undefined4 *)(unaff_EBX + 0x58);
      *(undefined4 *)(param_1 + 0x10) = *(undefined4 *)(unaff_EBX + 0x5c);
      *(undefined4 *)(param_1 + 0x14) = *(undefined4 *)(unaff_EBX + 0x60);
      *(undefined4 *)(param_1 + 0x14) = 0;
    }
    if (sVar2 != -1) {
      sVar6 = -1;
      local_491 = 1;
      local_488 = 0;
      do {
        sVar3 = sVar2;
        iVar4 = sVar3 * 0x34;
        sVar2 = *(short *)(iVar4 + 0xb2 + unaff_EBX);
        iVar4 = iVar4 + 0x84 + unaff_EBX;
        if (sVar2 < 0x40) {
          local_400[sVar2 * 4] = *(undefined4 *)(iVar4 + 8);
          if (sVar6 == -1) {
            puVar7 = local_400 + *(short *)(iVar4 + 0x2e) * 4 + 1;
            puVar5 = (undefined4 *)(param_1 + 4);
          }
          else {
            puVar5 = (undefined4 *)(local_488 + 0xc);
            puVar7 = local_400 + *(short *)(iVar4 + 0x2e) * 4 + 1;
          }
          *puVar7 = *puVar5;
          puVar7[1] = puVar5[1];
          puVar7[2] = puVar5[2];
        }
        else {
          local_491 = 0;
        }
        local_488 = iVar4;
        sVar2 = *(short *)(iVar4 + 2);
        sVar6 = sVar3;
      } while (*(short *)(iVar4 + 2) != -1);
      FUN_0043cc00();
      cVar1 = FUN_0043be90();
      if (cVar1 != '\0') {
        param_1[0x18] = local_491;
        param_1[0x19] = 0;
        *param_1 = 1;
        param_1[0x1a] = 0;
        puVar5 = local_440;
        puVar7 = (undefined4 *)(param_1 + 0x1c);
        for (iVar4 = 0; iVar4 != 0; iVar4 = iVar4 + -1) {
          *puVar7 = *puVar5;
          puVar5 = puVar5 + 1;
          puVar7 = puVar7 + 1;
        }
        for (iVar4 = 0; iVar4 != 0; iVar4 = iVar4 + -1) {
          *(undefined1 *)puVar7 = *(undefined1 *)puVar5;
          puVar5 = (undefined4 *)((int)puVar5 + 1);
          puVar7 = (undefined4 *)((int)puVar7 + 1);
        }
        if (param_1[0x18] != '\0') {
          cVar1 = param_1[0x19];
          *(undefined4 *)(param_1 + 4) = *(undefined4 *)(param_1 + cVar1 * 0x10 + 0x10);
          *(undefined4 *)(param_1 + 8) = *(undefined4 *)(param_1 + cVar1 * 0x10 + 0x14);
          *(undefined4 *)(param_1 + 0xc) = *(undefined4 *)(param_1 + cVar1 * 0x10 + 0x18);
          *(undefined4 *)(param_1 + 0x10) = *(undefined4 *)(param_1 + cVar1 * 0x10 + 0xc);
          vector3d_distance();
          *(float *)(param_1 + 0x14) = (float)extraout_ST0;
        }
      }
    }
  }
LAB_0043a6ec:
  return *param_1;
}
#endif
