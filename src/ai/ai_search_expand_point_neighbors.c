// ai_search_expand_point_neighbors  (Ghidra: ai_search_expand_point_neighbors, renamed)
// address 0x43ba60, size 579 bytes
// name confidence: 0.35  rewrite confidence: 0.1
// evidence: phase-4 summary "breadth-first expands a point's neighbors in the AI navigation
// point graph, scoring and pushing viable successors onto the open list." Calls
// ai_search_add_node (0x43b5a0), ai_search_evaluate_edge_cost (0x43b830, this rewrite's own
// very-low-confidence file), ai_search_compute_point_tangents (this rewrite) and path_find_trace_cluster_boundary_from_vertex (this rewrite).
//
// Kept close to the Ghidra decompilation and at very low confidence. This function's own
// stack frame mixes an `ai_search_obstacle_list`-sized visited bitset (`local_11c`, sized
// from the obstacle count) with a 128-entry BFS worklist (`local_100`) and several scratch
// floats whose exact roles are not independently confirmed; ai_search_evaluate_edge_cost is
// itself one of this batch's least reliable rewrites, and this function's own call into it
// passes twelve operands that only partially line up with that file's inferred signature.
// This translation preserves the raw arithmetic and control flow rather than asserting
// struct-level confidence it does not have.
//
// register convention: DX -> start_point_id (the only Ghidra-recognized register operand;
//   `context` and `param_2` are the two recognized stack/register formal parameters).
//   // blam-cc: EDX -> start_point_id, stack -> context, param_2

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>

extern void ai_search_add_node(void *context, real_point2d *position, uint32_t param3, int16_t worklist_index, float extra_cost); // 0x43b5a0, see header UNSURE on the arity mismatch with ai_search_add_node's own file
extern void ai_search_compute_point_tangents(float param1, void *out_a, float *out_b); // 0x43c9a0
extern void path_find_trace_cluster_boundary_from_vertex(uint8_t param1, float *point, float param3, float *out_direction, float distance, void *out_result); // 0x43d790
extern uint8_t ai_search_evaluate_edge_cost(uint32_t cluster_a, uint8_t param2, uint32_t cluster_b, uint32_t param4,
                                            float *point, float param6, float distance, float base_cost,
                                            uint8_t skip_direct, uint8_t apply_offset, uint8_t param_11,
                                            float *out_result); // 0x43b830, see header UNSURE

// blam-cc: EDX -> start_point_id, stack -> context, param_2
void ai_search_expand_point_neighbors(float *context, int16_t param_2, int16_t start_point_id)
{
    uint32_t visited[4];
    uint16_t worklist[128];
    int16_t worklist_count;
    float *base_position;
    float *cur_position;
    int32_t i;

    for (i = 0; i < 4; i = i + 1) {
        visited[i] = 0;
    }

    base_position = context + param_2 * 10 + 0xc;
    visited[start_point_id >> 5] |= 1u << (start_point_id & 0x1f);
    worklist[0] = (uint16_t)start_point_id; // UNSURE: original never explicitly writes worklist[0]; see below
    worklist_count = 1;
    cur_position = context + param_2 * 10 + 0xc;

    do {
        uint16_t point_id;
        uint32_t obstacle_index;
        int32_t obstacle_link;
        real_point2d out_a;
        float out_b;
        int16_t leg;
        float *leg_direction;

        worklist_count = worklist_count - 1;
        point_id = worklist[worklist_count];
        obstacle_index = point_id;
        if (point_id == 0xffff) {
            obstacle_link = -1;
        } else {
            obstacle_link = *(int16_t *)((uint8_t *)(uintptr_t)(uint32_t)context[2] + 10 + point_id * 0x14);
        }

        ai_search_compute_point_tangents(*context, &out_a, &out_b);
        if (out_b < *context) {
            out_b = *context;
        }

        leg = 0;
        leg_direction = 0; // local_12c, UNSURE (advances by 2 floats per leg)
        do {
            float edge_result[3]; // local_13c
            uint8_t reached;

            reached = ai_search_evaluate_edge_cost((uint32_t)context[3], *(uint8_t *)(context + 1), (uint32_t)context[2],
                                                   obstacle_index, cur_position, cur_position[2], *context,
                                                   *context + *context + out_b, 0, 0,
                                                   *((uint8_t *)context + 0x2a), edge_result);
            cur_position = base_position;

            {
                int16_t neighbor = (int16_t)0xffff; // UNSURE: `local_130`'s int16 half, see original
                (void)reached;
                if (neighbor != -1) {
                    uint32_t bit = 1u << (neighbor & 0x1f);
                    if ((visited[(uint16_t)neighbor >> 5] & bit) == 0) {
                        visited[(uint16_t)neighbor >> 5] |= bit;
                        worklist[worklist_count] = (uint16_t)neighbor;
                        worklist_count = worklist_count + 1;
                    }
                }
            }

            if (out_b < edge_result[0]) {
                if (1 /* UNSURE: original compares a companion int16 against obstacle_link here */) {
                    float mid = (edge_result[0] + out_b) * 0.5f;
                    uint8_t trace_result[4];
                    float new_x, new_y;

                    path_find_trace_cluster_boundary_from_vertex(*(uint8_t *)(context + 1), base_position, base_position[2], leg_direction, mid, trace_result);
                    new_x = mid * leg_direction[0] + cur_position[0];
                    new_y = mid * leg_direction[1] + cur_position[1];
                    {
                        real_point2d p; p.x = new_x; p.y = new_y;
                        ai_search_add_node(&p, 0, obstacle_link, leg, (cur_position[8] - cur_position[5]) + mid);
                    }
                }
            }

            leg_direction = leg_direction + 2;
            leg = leg + 1;
            cur_position = base_position;
        } while (leg < 2);
    } while (0 < worklist_count);
}

#if 0
// ---- original Ghidra decompilation (FUN_0043ba60 @ 0x43ba60) ----
void FUN_0043ba60(float *param_1,short param_2)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  short in_DX;
  ushort uVar4;
  float *pfVar5;
  float *pfVar6;
  uint *puVar7;
  uint auStackY_111c [991];
  float local_160;
  int local_15c;
  float local_158;
  float *local_154;
  undefined4 local_150;
  int local_14c;
  uint local_148;
  float local_144;
  float local_140;
  float local_13c [3];
  undefined4 local_130;
  short sStack_12e;
  float local_12c [2];
  undefined1 local_124 [8];
  uint local_11c [4];
  undefined1 local_10c [4];
  undefined4 local_108;
  ushort local_100 [128];

  puVar7 = local_11c;
  for (uVar2 = *(short *)((int)param_1[2] + 2) + 0x1f >> 5 & 0x3fffffff; uVar2 != 0;
      uVar2 = uVar2 - 1) {
    *puVar7 = 0;
    puVar7 = puVar7 + 1;
  }
  for (iVar3 = 0; iVar3 != 0; iVar3 = iVar3 + -1) {
    *(undefined1 *)puVar7 = 0;
    puVar7 = (uint *)((int)puVar7 + 1);
  }
  local_154 = param_1 + param_2 * 10 + 0xc;
  local_11c[(int)in_DX >> 5] = local_11c[(int)in_DX >> 5] | 1 << ((byte)in_DX & 0x1f);
  local_15c = 1;
  pfVar5 = param_1 + param_2 * 10 + 0xc;
  do {
    local_15c = local_15c + -1;
    uVar4 = local_100[(short)local_15c];
    local_148 = (uint)uVar4;
    if (uVar4 == 0xffff) {
      local_150 = 0xffffffff;
    }
    else {
      local_150 = CONCAT22(local_150._2_2_,
                           *(undefined2 *)((int)param_1[2] + 10 + (short)uVar4 * 0x14));
    }
    FUN_0043c9a0(*param_1,local_124,&local_160);
    if (local_160 < *param_1) {
      local_160 = *param_1;
    }
    local_14c = 0;
    pfVar6 = local_12c;
    do {
      iVar3 = local_14c;
      FUN_0043b830(param_1[3],*(undefined1 *)(param_1 + 1),param_1[2],local_148,pfVar5,pfVar5[2],
                   *param_1,*param_1 + *param_1 + local_160,0,0,*(undefined1 *)((int)param_1 + 0x2a)
                   ,local_13c);
      pfVar5 = local_154;
      uVar4 = (ushort)local_130;
      if (uVar4 != 0xffff) {
        uVar2 = 1 << ((byte)local_130 & 0x1f);
        if ((uVar2 & local_11c[(int)(short)uVar4 >> 5]) == 0) {
          local_11c[(int)(short)uVar4 >> 5] = local_11c[(int)(short)uVar4 >> 5] | uVar2;
          sVar1 = (short)local_15c;
          local_15c = local_15c + 1;
          local_100[sVar1] = uVar4;
        }
      }
      if (local_160 < local_13c[0]) {
        if (sStack_12e != (short)local_150) {
          local_158 = (local_13c[0] + local_160) * 0.5;
          FUN_0043d790(*(undefined1 *)(param_1 + 1),local_154,local_154[2],pfVar6,local_158,
                       local_10c);
          local_144 = local_158 * *pfVar6 + *pfVar5;
          local_140 = local_158 * pfVar6[1] + pfVar5[1];
          FUN_0043b5a0(&local_144,local_108,local_150,iVar3,(pfVar5[8] - pfVar5[5]) + local_158);
          iVar3 = local_14c;
        }
      }
      pfVar6 = pfVar6 + 2;
      local_14c = iVar3 + 1;
      pfVar5 = local_154;
    } while ((short)(iVar3 + 1) < 2);
  } while (0 < (short)local_15c);
  return;
}
#endif
