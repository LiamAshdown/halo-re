// ai_search_expand_point_neighbors  (Ghidra: ai_search_expand_point_neighbors, renamed)
// address 0x43ba60, size 579 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// REWRITTEN from objdump 0x43ba60..0x43bca2. Stack: context, node index; DX = the obstacle point to start from.
//   A worklist (visited bits over the obstacle list) spreads from that point: for each point (its link, or -1),
//   ai_search_compute_point_tangents gives the two tangent directions from the node's position (radius
//   context +0x00) and the distance along them (at least the radius). Along each direction
//   ai_search_evaluate_edge_cost(map +0x0c, ignore +0x04, obstacles +0x08, exclude the point, the node's
//   position and surface (+0x08), radius, limit 2 * radius + distance, require_unflagged +0x2a) reports what stops
//   it; an obstacle point it runs into joins the worklist once. When it gets past the tangent distance and does not
//   end at the point's own link, a node is added halfway: position = node + direction * (cost + distance) / 2,
//   its surface from a boundary trace to there, point id = the point's link, side = the direction's index and
//   base cost = the node's inherited cost (cost - length) + that half distance.
// blam-cc: stack -> context, node_index; DX -> start_point_id

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void ai_search_compute_point_tangents(ai_search_obstacle_list *list, int16_t point_index, real_point2d *position,
    real_vector2d *edge_neg, float radius, real_vector2d *out_a, real *out_b); // 0x43c9a0, ECX, AX, EDX, ESI, stack
extern uint8_t ai_search_evaluate_edge_cost(void *context, uint8_t ignore_permission,
    ai_search_obstacle_list *obstacle_list, int16_t exclude_index, real_point2d *point, int32_t start_surface_index,
    float distance, float base_cost, uint8_t skip_direct, uint8_t apply_offset, uint8_t require_unflagged,
    ai_search_edge_result *out_result, real_vector2d *direction); // 0x43b830, EBX direction, stack
extern uint8_t path_find_trace_cluster_boundary_from_vertex(void *context, uint8_t ignore_permission,
    real_point2d *point, int32_t start_index, real_vector2d *direction, float max_distance,
    path_find_boundary_trace_result *out); // 0x43d790, EAX map, stack
extern int16_t ai_search_add_node(ai_search_context *context, int16_t parent, real_point2d *position, int32_t surface_index,
    int16_t point_id, uint8_t side, float base_cost); // 0x43b5a0, EDI, BX, stack

void ai_search_expand_point_neighbors(ai_search_context *context, int16_t node_index, int16_t start_point_id)
{
    ai_search_obstacle_list *list = (ai_search_obstacle_list *)(uintptr_t)context->obstacles;
    void *map = (void *)(uintptr_t)context->structure_bsp;
    float radius = *(float *)&context->search_radius;
    ai_search_node *node = &context->nodes[node_index];
    uint32_t visited[8];
    int16_t worklist[0x78];
    int16_t pending = 1;

    memset(visited, 0, ((list->count + 0x1f) >> 5) * 4);
    visited[start_point_id >> 5] |= 1u << (start_point_id & 0x1f);
    worklist[0] = start_point_id;
    do {
        int16_t point = worklist[--pending];
        int16_t link = (point != -1) ? list->obstacles[point].link : -1;
        real_vector2d directions[2];
        float tangent_distance;
        int16_t side;

        ai_search_compute_point_tangents(list, point, &node->position, &directions[0], radius, &directions[1],
            &tangent_distance);
        if (tangent_distance < radius) {
            tangent_distance = radius;
        }
        for (side = 0; side < 2; side++) {
            ai_search_edge_result edge;

            ai_search_evaluate_edge_cost(map, context->unknown_04, list, point, &node->position,
                *(int32_t *)&node->z, radius, radius + radius + tangent_distance, 0, 0, context->ignore_flagged_obstacles, &edge,
                &directions[side]);
            if (edge.point_id != -1 && (visited[edge.point_id >> 5] & (1u << (edge.point_id & 0x1f))) == 0) {
                visited[edge.point_id >> 5] |= 1u << (edge.point_id & 0x1f);
                worklist[pending++] = edge.point_id;
            }
            if (edge.cost > tangent_distance && edge.link != link) {
                float half = (edge.cost + tangent_distance) * 0.5f;
                path_find_boundary_trace_result trace;
                real_point2d position;

                path_find_trace_cluster_boundary_from_vertex(map, context->unknown_04, &node->position,
                    *(int32_t *)&node->z, &directions[side], half, &trace);
                position.x = half * directions[side].i + node->position.x;
                position.y = half * directions[side].j + node->position.y;
                ai_search_add_node(context, node_index, &position, trace.surface_index, link, (uint8_t)side,
                    (node->cost - node->length) + half);
            }
        }
    } while (pending > 0);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
