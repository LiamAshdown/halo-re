// path_find_reconstruct_path  (Ghidra: path_find_reconstruct_path, renamed)
// address 0x43a4d0, size 551 bytes
// name confidence: 0.4   rewrite confidence: 0.15
// evidence: types/ai.h path_find_context.have_goal(+0x4c)/goal_position(+0x50)/
//   goal_vertex_id(+0x5c)/goal_cost(+0x60)/best_node(+0x68)/best_cost(+0x6c)/unknown_70/
//   best_position(+0x74)/path_find_node.waypoint(+0x2e)/parent(+0x02)/vertex_id(+0x08).
//   Calls path_find_hash_lookup_vertex @0x43b2b0, path_find_simplify_waypoints (this rewrite's
//   path_find_simplify_waypoints) and ai_navigate_around_obstacles (this rewrite's
//   path_find_search_point_graph), both called here with no visible arguments at all.
//   phase-4 summary "reconstructs and smooths the final waypoint path from a completed
//   pathfinding search's node chain."
//
// Kept deliberately close to the Ghidra decompilation, and at low confidence, for two
// reasons documented in detail below: two stack-copy loops in the original are provably
// dead code (their trip count is a decompiled literal `0`, so they can never execute), and
// path_find_simplify_waypoints/ai_navigate_around_obstacles are called with zero visible arguments even though the rest of
// this function is clearly assembling a caller-owned output buffer (`param_1`) that must be
// getting written some other way -- almost certainly through a hidden pointer argument to
// one or both of those calls that this decompilation lost entirely. This rewrite reproduces
// the observable arithmetic exactly; it does not invent a fix for the dead loops or guess
// what pointer those two calls really take.
//
// register convention: EBX -> context; stack -> out_result.
//   // blam-cc: EBX -> context, stack -> out_result
//
// UNSURE (see the two paragraphs above for the main ones):
//  - path_find_hash_lookup_vertex is called here with no visible arguments (context/
//    goal_vertex_id both implicit); declared and called locally with zero arguments to
//    match this call site.
//  - The `for (iVar4 = 0; iVar4 != 0; ...)` copy loops (both a dword-wise and a byte-wise
//    version) can never run: Ghidra could not recover their real trip count (almost
//    certainly path_find_simplify_waypoints's return value, a waypoint count, which this decompilation also
//    fails to capture from that call). Reproduced as dead code exactly as shown.
//  - The final block re-reads `out_result` at `+0x10 + index*0x10` as a 0x10-stride
//    waypoint array, which is a different base than the `+0x1c` the (dead) copy loops target
//    -- consistent with the waypoint array being populated by path_find_simplify_waypoints/ai_navigate_around_obstacles
//    directly rather than by anything visible in this function.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void vector3d_distance(void); // 0x4088b0, established elsewhere; called here with no visible arguments (see header)
extern int16_t path_find_hash_lookup_vertex(void); // 0x43b2b0, see header UNSURE
extern void path_find_simplify_waypoints(void); // 0x43cc00 = path_find_simplify_waypoints (this rewrite's own file), called with no visible arguments here
extern uint8_t ai_navigate_around_obstacles(void); // 0x43be90 = path_find_search_point_graph (this rewrite's own file), called with no visible arguments here

// blam-cc: EBX -> context, stack -> out_result
uint8_t path_find_reconstruct_path(path_find_context *context, uint8_t *out_result)
{
    int16_t node_index;
    uint8_t *node_ptr;
    int16_t waypoint;
    int16_t prev_node;
    uint8_t all_waypoints_valid;
    uint8_t *prev_slot;
    uint32_t local_400[256];
    uint32_t local_440[16];

    out_result[0] = 0;
    if (context->have_goal == 0) {
        return out_result[0];
    }

    node_index = path_find_hash_lookup_vertex();
    if (node_index == -1) {
        if (context->goal_cost <= context->best_cost) {
            return out_result[0];
        }
        node_index = context->best_node;
        *(real_point3d *)(out_result + 4) = context->best_position;
        *(uint32_t *)(out_result + 0x10) = context->nodes[node_index].vertex_id;
        *(float *)(out_result + 0x14) = context->best_cost;
    } else {
        *(real_point3d *)(out_result + 4) = context->goal_position;
        *(uint32_t *)(out_result + 0x10) = context->goal_vertex_id;
        *(float *)(out_result + 0x14) = 0.0f; // goal_cost written then immediately overwritten with 0 in the original
    }

    if (node_index != -1) {
        prev_node = -1;
        all_waypoints_valid = 1;
        prev_slot = 0;

        do {
            int16_t cur = node_index;
            node_ptr = (uint8_t *)&context->nodes[cur];
            waypoint = context->nodes[cur].waypoint;

            if (waypoint < 0x40) {
                local_400[waypoint * 4] = context->nodes[cur].vertex_id;
                {
                    uint32_t *dst = local_400 + context->nodes[cur].waypoint * 4 + 1;
                    uint32_t *src;
                    if (prev_node == -1) {
                        src = (uint32_t *)(out_result + 4);
                    } else {
                        src = (uint32_t *)(prev_slot + 0xc);
                    }
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                }
            } else {
                all_waypoints_valid = 0;
            }

            prev_slot = node_ptr;
            node_index = context->nodes[cur].parent;
            prev_node = cur;
        } while (context->nodes[prev_node].parent != -1);

        path_find_simplify_waypoints();
        if (ai_navigate_around_obstacles() != 0) {
            out_result[0x18] = all_waypoints_valid;
            out_result[0x19] = 0;
            out_result[0] = 1;
            out_result[0x1a] = 0;

            // UNSURE: dead in the original (trip count 0); see header.
            {
                uint32_t *src = local_440;
                uint32_t *dst = (uint32_t *)(out_result + 0x1c);
                int32_t n;
                for (n = 0; n != 0; n = n - 1) {
                    *dst = *src;
                    src = src + 1;
                    dst = dst + 1;
                }
                {
                    uint8_t *bsrc = (uint8_t *)src;
                    uint8_t *bdst = (uint8_t *)dst;
                    for (n = 0; n != 0; n = n - 1) {
                        *bdst = *bsrc;
                        bsrc = bsrc + 1;
                        bdst = bdst + 1;
                    }
                }
            }

            if (out_result[0x18] != 0) {
                uint8_t index = out_result[0x19];
                *(uint32_t *)(out_result + 4) = *(uint32_t *)(out_result + index * 0x10 + 0x10);
                *(uint32_t *)(out_result + 8) = *(uint32_t *)(out_result + index * 0x10 + 0x14);
                *(uint32_t *)(out_result + 0xc) = *(uint32_t *)(out_result + index * 0x10 + 0x18);
                *(uint32_t *)(out_result + 0x10) = *(uint32_t *)(out_result + index * 0x10 + 0xc);
                vector3d_distance();
                *(float *)(out_result + 0x14) = 0.0f; // UNSURE: extraout_ST0, see vector3d_distance's own hidden result
            }
        }
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
