// path_find_run  (Ghidra: path_find_run, already named)
// address 0x43a8b0, size 1716 bytes
// name confidence: 0.55  rewrite confidence: 0.2
// evidence: types/ai.h path_find_context (node_count +0x80, heap_count +0xd084, heap
//   +0xd086, vertex_hash +0xe08a, best_node +0x68, best_cost +0x6c, unknown_70, best_position
//   +0x74, have_goal +0x4c, goal_position +0x50, goal_vertex_id +0x5c, goal_cost +0x60,
//   bsp_generation +0x64) and path_find_node (parent, unknown_04, vertex_id, position, cost,
//   unknown_1c/unknown_20/unknown_24, distance, key, waypoint, heap_index) -- every offset
//   below is one of those fields, cross-checked against the "in_EAX[K]" float-array indexing
//   Ghidra used throughout via `node_byte_offset = 4*(K-0x21)` (confirmed independently at
//   several of the K values, e.g. K=0x23 is vertex_id, matched by its use as the argument to
//   path_find_gather_adjacent_edges). Calls path_find_push_start_node, path_find_heap_sift_up,
//   path_find_heap_sift_down, path_find_vertex_distance (path_find_vertex_distance),
//   path_find_gather_adjacent_edges (path_find_gather_adjacent_edges) and path_find_score_avoidance_penalty (this rewrite,
//   obstacle-avoidance cost penalty), all in this module.
//
// This is the module's main A*-style search loop and by far its largest function. Kept
// close to the Ghidra decompilation and at low confidence: the surface-permission lookup
// (DAT_006b8d78/DAT_0069e8d8, gating whether an edge can be crossed at all) belongs to a
// different, un-established subsystem and is preserved as raw offsets; `context->
// bsp_generation` is dereferenced here as a pointer into a structure_bsp-shaped record
// (matching the module header's own note that it is "the structure BSP pointer, used here
// as an opaque handle" despite its int32 declaration), not used as a counter.
//
// register convention: EAX -> context (the only register Ghidra's decompile shows; ESI/EDI
//   appear as `unaff_` registers in one call, forwarded to path_find_heap_sift_down, but the
//   values needed there -- the just-popped heap slot and its replacement -- are computed in
//   this function and passed explicitly here instead of literally forwarding unknown
//   registers).
//   // blam-cc: EAX -> context
//
// UNSURE, broadly:
//  - The permission-bitmap test (`DAT_006b8d78`/`DAT_0069e8d8`) is reproduced verbatim as
//    raw offsets; its real meaning (a per-material or per-team crossing permission, by the
//    shape of the lookup) is not established here.
//  - `__ftol()` is called with no visible operand; by analogy with every other user of this
//    pattern in this module, it almost certainly rounds `local_834` (the just-computed f-cost
//    estimate) to get the heap sort key, and is modeled that way here rather than as an
//    unresolved 0.0.
//  - path_find_gather_adjacent_edges and path_find_score_avoidance_penalty are called here with fewer arguments
//    than elsewhere (context not shown); called explicitly with this function's own context.
// reconciled: R06 path_find_context.bsp_generation -> structure_bsp (0x00746f9c, the resident ScenarioStructureBSP pointer)
// reconciled: R79 0x006b8d78 ai_path_permission_table -> physics.h breakable_surface_globals *breakable_surface_state (the code took the global's ADDRESS; the binary loads the pointer: mov edx,ds:0x6b8d78) and 0x0069e8d8 local_command_list_generation -> global_structure_bsp_index; the row is active[bsp index] (intact breakable surfaces)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "physics.h"

extern void path_find_heap_sift_up(path_find_context *context, int16_t index);   // 0x43af70
extern void path_find_heap_sift_down(path_find_context *context, int16_t index); // 0x43b010
extern uint8_t path_find_push_start_node(path_find_context *context);            // 0x43a760
extern float path_find_vertex_distance(real_point3d *point_a, real_point3d *point_b); // 0x43b130

// TYPES-GAP: duplicated from path_find_gather_adjacent_edges.c (each rewritten file is
// compiled independently, so the shared shape is repeated here rather than shared through a
// header this task's rules do not let this rewrite add).
extern int16_t path_find_gather_adjacent_edges(path_find_context *context, int32_t vertex_id,
                                               path_find_adjacent_edge *out_edges); // 0x43b1c0
extern float path_find_score_avoidance_penalty(path_find_context *context, float *out_distance); // 0x43b3b0, called here with context forwarded explicitly instead of Ghidra's no-argument call

extern int32_t __ftol(double x); // FISTP-based float-to-int truncation
extern double sqrt(double x); // FSQRT
extern breakable_surface_globals *breakable_surface_state; // 0x006b8d78, physics.h
extern int16_t global_structure_bsp_index; // 0x0069e8d8, physics.h (the structure BSP index)

// blam-cc: EAX -> context
// Runs the A*-style search to completion: repeatedly pops the cheapest open node, expands
// its adjacent navmesh edges into new or improved candidate nodes, and re-heapifies, until
// either the exact goal vertex is reached, the open list empties, or the node/heap budget is
// exhausted. Returns 1 if a usable path (exact or best-effort) was found, 0 otherwise.
uint8_t path_find_run(path_find_context *context)
{
    float step_radius;
    path_find_adjacent_edge edges[64];

    context->node_count = 0;
    context->heap_count = 1;
    {
        int16_t *hash = context->vertex_hash;
        int32_t i;
        for (i = 0; i < 4096; i = i + 1) {
            hash[i] = -1;
        }
    }
    context->best_node = -1;
    context->best_cost = 3.4028235e+38f;
    context->unknown_70 = 3.4028235e+38f;

    if (path_find_push_start_node(context) == 0) {
        return 0;
    }

    step_radius = (0.2f <= *(float *)context) ? *(float *)context : 0.2f;

    for (;;) {
        int16_t popped_node_index;
        int16_t edge_count;
        int32_t current_index; // local_804

        if (context->heap_count < 2) {
            if (context->have_goal == 0) {
                return 1;
            }
            return (context->best_cost < context->goal_cost) != (context->best_cost == context->goal_cost);
        }

        popped_node_index = context->heap[1].node;
        current_index = popped_node_index;
        context->nodes[popped_node_index].heap_index = -1;
        context->heap_count = context->heap_count - 1;

        if (1 < context->heap_count) {
            context->heap[1] = context->heap[context->heap_count];
            path_find_heap_sift_down(context, 1);
        }

        if (popped_node_index == -1) {
            if (context->have_goal == 0) {
                return 1;
            }
            return (context->best_cost < context->goal_cost) != (context->best_cost == context->goal_cost);
        }

        if (context->have_goal != 0) {
            if (context->nodes[popped_node_index].vertex_id == context->goal_vertex_id) {
                context->best_position.x = context->goal_position.x;
                context->best_position.y = context->goal_position.y;
                context->best_node = popped_node_index;
                context->best_cost = 0.0f;
                context->best_position.z = context->goal_position.z;
                if (context->have_goal == 0) {
                    return 1;
                }
                return (context->best_cost < context->goal_cost) != (context->best_cost == context->goal_cost);
            }

            {
                float leash = (5.0f <= context->best_cost) ? context->best_cost : 5.0f;
                if (leash * 10.0f + context->unknown_70 < context->nodes[popped_node_index].distance) {
                    if (context->have_goal == 0) {
                        return 1;
                    }
                    return (context->best_cost < context->goal_cost) != (context->best_cost == context->goal_cost);
                }
            }
        }

        edge_count = path_find_gather_adjacent_edges(context, context->nodes[popped_node_index].vertex_id, edges);
        if (0 < edge_count) {
            int16_t i;
            for (i = 0; i < edge_count; i = i + 1) {
                path_find_adjacent_edge *edge = &edges[i];
                uint8_t crosses_edge;

                crosses_edge = 0;
                if ((*(uint8_t *)((uint8_t *)context + 4) != 0) || (0 <= (int8_t)edge->flag)) {
                    crosses_edge = 1;
                } else {
                    int32_t table_entry = *(int32_t *)(*(int32_t *)(context->structure_bsp + 0xb4) + 0x40) + edge->edge_id * 0xc;
                    if ((*(uint8_t *)(table_entry + 8) & 8) == 0) {
                        crosses_edge = 1;
                    } else {
                        uint8_t permission_index = *(uint8_t *)(table_entry + 9);
                        uint32_t bits = breakable_surface_state->active[global_structure_bsp_index][permission_index >> 5];
                        uint8_t denied = 1 - (uint8_t)((bits & (1u << (permission_index & 0x1f))) != 0);
                        if (denied == 0) {
                            crosses_edge = 1;
                        }
                    }
                }

                if (crosses_edge && ((edge->flag & 0x40) != 0) &&
                    (edge->edge_id != context->nodes[popped_node_index].unknown_04)) {
                    float new_x = edge->direction_x * 0.5f + edge->start_x;
                    float new_y = edge->direction_y * 0.5f + edge->start_y;
                    float new_z = edge->direction_z * 0.5f + edge->start_z;

                    if (context->have_goal != 0) {
                        float dir_len2 = edge->direction_x * edge->direction_x + edge->direction_y * edge->direction_y +
                                        edge->direction_z * edge->direction_z;
                        if ((16.0f < dir_len2) && ((step_radius + step_radius) * (step_radius + step_radius) < dir_len2)) {
                            float dir_len = (float)sqrt(dir_len2);
                            float t = ((context->goal_position.y - edge->start_y) * edge->direction_y +
                                      (context->goal_position.z - edge->start_z) * edge->direction_z +
                                      (context->goal_position.x - edge->start_x) * edge->direction_x) / dir_len2;
                            float min_t = step_radius / dir_len;
                            float max_t = 1.0f - min_t;
                            if (min_t <= t) {
                                t = (t < max_t) ? t : max_t;
                            } else {
                                t = min_t;
                            }
                            new_x = t * edge->direction_x + edge->start_x;
                            new_y = t * edge->direction_y + edge->start_y;
                            new_z = t * edge->direction_z + edge->start_z;
                        }
                    }

                    {
                        float step_cost = (float)sqrt(
                            (new_x - context->nodes[popped_node_index].position.x) * (new_x - context->nodes[popped_node_index].position.x) +
                            (new_y - context->nodes[popped_node_index].position.y) * (new_y - context->nodes[popped_node_index].position.y) +
                            (new_z - context->nodes[popped_node_index].position.z) * (new_z - context->nodes[popped_node_index].position.z));
                        float g_cost = step_cost + context->nodes[popped_node_index].unknown_20;
                        float avoid_distance;
                        float adjusted_cost;
                        float f_cost;

                        if (((path_find_request *)context)->have_avoid_sphere == 0) {
                            adjusted_cost = step_cost;
                            avoid_distance = 0.0f;
                        } else {
                            adjusted_cost = (path_find_score_avoidance_penalty(context, &avoid_distance) + 1.0f) * step_cost;
                            if (context->nodes[popped_node_index].unknown_1c <= avoid_distance) {
                                avoid_distance = context->nodes[popped_node_index].unknown_1c;
                            }
                        }
                        f_cost = adjusted_cost + context->nodes[popped_node_index].unknown_24;

                        {
                            float total_estimate = f_cost;
                            float dist_to_goal = 0.0f;
                            if (context->have_goal != 0) {
                                dist_to_goal = (float)sqrt(
                                    (context->goal_position.x - new_x) * (context->goal_position.x - new_x) +
                                    (context->goal_position.y - new_y) * (context->goal_position.y - new_y) +
                                    (context->goal_position.z - new_z) * (context->goal_position.z - new_z));
                                total_estimate = dist_to_goal + f_cost;
                            }

                            {
                                int32_t rounded_key = __ftol((double)total_estimate);
                                if ((rounded_key < 0x7fff) &&
                                    ((*(uint8_t *)((uint8_t *)context + 0x40) == 0) ||
                                     (g_cost <= *(float *)((uint8_t *)context + 0x44)))) {
                                    uint32_t bucket = (uint32_t)(edge->edge_id & 0x1ff) << 3;
                                    int16_t found = context->vertex_hash[bucket];
                                    int16_t target_index = -1;
                                    uint8_t skip = 0;
                                    uint8_t update_in_place = 0;

                                    while (found != -1) {
                                        if (context->nodes[found].vertex_id == edge->edge_id) {
                                            if ((context->nodes[found].key <= (int16_t)rounded_key) ||
                                                (context->nodes[found].heap_index == -1)) {
                                                skip = 1;
                                            } else {
                                                target_index = found;
                                                update_in_place = 1;
                                            }
                                            break;
                                        }
                                        bucket = (bucket + 1) & 0xfff;
                                        found = context->vertex_hash[bucket];
                                    }

                                    if (!skip) {
                                        if (!update_in_place) {
                                            int16_t new_slot = context->node_count;
                                            if (new_slot < k_path_find_maximum_nodes) {
                                                context->node_count = new_slot + 1;
                                                context->vertex_hash[bucket] = new_slot;
                                                context->nodes[new_slot].heap_index = -1;
                                                target_index = new_slot;
                                            } else {
                                                target_index = -1;
                                            }
                                        }

                                        if (target_index != -1) {
                                            path_find_node *node = &context->nodes[target_index];
                                            node->parent = (int16_t)current_index;
                                            node->unknown_04 = context->nodes[popped_node_index].vertex_id;
                                            node->vertex_id = edge->edge_id;
                                            node->position.x = new_x;
                                            node->position.y = new_y;
                                            node->position.z = new_z;
                                            node->cost = step_cost;
                                            node->unknown_1c = avoid_distance;
                                            node->unknown_20 = g_cost;
                                            node->unknown_24 = adjusted_cost;
                                            node->distance = total_estimate;
                                            node->key = (int16_t)rounded_key;
                                            node->waypoint = context->nodes[popped_node_index].waypoint + 1;

                                            if (node->heap_index == -1) {
                                                if (context->heap_count < k_path_find_maximum_heap) {
                                                    int16_t heap_slot = context->heap_count;
                                                    context->heap_count = heap_slot + 1;
                                                    context->heap[heap_slot].key = (int16_t)rounded_key;
                                                    context->heap[heap_slot].node = target_index;
                                                    path_find_heap_sift_up(context, heap_slot);
                                                }
                                            } else {
                                                context->heap[node->heap_index].key = (int16_t)rounded_key;
                                                path_find_heap_sift_up(context, node->heap_index);
                                            }

                                            if (context->have_goal != 0) {
                                                float refined_estimate = dist_to_goal;
                                                real_point3d refined_position = node->position;
                                                if (dist_to_goal < 4.0f) {
                                                    refined_estimate = path_find_vertex_distance(&context->goal_position, &refined_position);
                                                }
                                                if (refined_estimate < context->best_cost) {
                                                    context->best_cost = refined_estimate;
                                                    context->best_position = refined_position;
                                                    context->best_node = target_index;
                                                    context->unknown_70 = total_estimate;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

#if 0
// ---- original Ghidra decompilation (path_find_run @ 0x43a8b0) ----
undefined4 path_find_run(void)

{
  float fVar1;
  float fVar2;
  byte bVar3;
  short sVar4;
  float fVar5;
  char cVar6;
  ushort uVar7;
  float *in_EAX;
  int iVar8;
  undefined2 uVar9;
  float *pfVar10;
  short sVar11;
  undefined4 unaff_ESI;
  undefined4 unaff_EDI;
  undefined4 *puVar12;
  float *pfVar13;
  float10 fVar14;
  float local_840;
  float local_83c;
  float local_838;
  float local_834;
  float local_830;
  float *local_82c;
  float local_828;
  float local_824;
  char local_81d;
  float local_81c;
  uint local_818;
  float local_814;
  float local_810;
  float local_80c;
  float local_808;
  uint local_804;
  float local_800;
  undefined1 local_7fc [4];
  float afStack_7f8 [506];
  undefined4 uStack_10;

  *(undefined2 *)(in_EAX + 0x20) = 0;
  *(undefined2 *)(in_EAX + 0x3421) = 1;
  puVar12 = (undefined4 *)((int)in_EAX + 0xe08a);
  for (iVar8 = 0x800; iVar8 != 0; iVar8 = iVar8 + -1) {
    *puVar12 = 0xffffffff;
    puVar12 = puVar12 + 1;
  }
  *(undefined2 *)(in_EAX + 0x1a) = 0xffff;
  in_EAX[0x1b] = 3.4028235e+38;
  in_EAX[0x1c] = 3.4028235e+38;
  uStack_10 = 0x43a8eb;
  cVar6 = path_find_push_start_node();
  if (cVar6 == '\0') {
    return 0;
  }
  if (0.2 <= *in_EAX) {
    local_81c = *in_EAX;
  }
  else {
    local_81c = 0.2;
  }
  do {
    if (*(short *)(in_EAX + 0x3421) < 2) {
LAB_0043af2f:
      if (*(char *)(in_EAX + 0x13) == '\0') {
        return 1;
      }
      if (in_EAX[0x1b] < in_EAX[0x18] != (in_EAX[0x1b] == in_EAX[0x18])) {
        return 1;
      }
      return 0;
    }
    uVar7 = *(ushort *)((int)in_EAX + 0xd08a);
    local_804 = (uint)uVar7;
    pfVar13 = in_EAX + (short)uVar7 * 0xd;
    *(undefined2 *)(pfVar13 + 0x2d) = 0xffff;
    *(short *)(in_EAX + 0x3421) = *(short *)(in_EAX + 0x3421) + -1;
    local_82c = pfVar13;
    if (1 < *(short *)(in_EAX + 0x3421)) {
      *(undefined4 *)((int)in_EAX + 0xd08a) =
           *(undefined4 *)((int)in_EAX + *(short *)(in_EAX + 0x3421) * 4 + 0xd086);
      path_find_heap_sift_down(unaff_EDI,unaff_ESI);
    }
    if (uVar7 == 0xffff) goto LAB_0043af2f;
    if (*(char *)(in_EAX + 0x13) != '\0') {
      if (pfVar13[0x23] == in_EAX[0x17]) {
        in_EAX[0x1d] = in_EAX[0x14];
        in_EAX[0x1e] = in_EAX[0x15];
        *(ushort *)(in_EAX + 0x1a) = uVar7;
        in_EAX[0x1b] = 0.0;
        in_EAX[0x1f] = in_EAX[0x16];
        goto LAB_0043af2f;
      }
      if (5.0 <= in_EAX[0x1b]) {
        fVar1 = in_EAX[0x1b];
      }
      else {
        fVar1 = 5.0;
      }
      if (fVar1 * 10.0 + in_EAX[0x1c] < pfVar13[0x2b]) goto LAB_0043af2f;
    }
    uVar7 = FUN_0043b1c0(pfVar13[0x23],&local_800);
    if (0 < (short)uVar7) {
      local_818 = (uint)uVar7;
      pfVar10 = afStack_7f8 + 4;
      do {
        if (((((*(char *)(in_EAX + 1) != '\0') || (-1 < (char)*(byte *)(pfVar10 + -5))) ||
             (iVar8 = *(int *)(*(int *)((int)in_EAX[0x19] + 0xb4) + 0x40) + (int)pfVar10[-6] * 0xc,
             (*(byte *)(iVar8 + 8) & 8) == 0)) ||
            (bVar3 = *(byte *)(iVar8 + 9),
            local_81d = '\x01' - ((*(uint *)(DAT_006b8d78 + 1 +
                                            ((uint)(bVar3 >> 5) + DAT_0069e8d8 * 8) * 4) &
                                  1 << (bVar3 & 0x1f)) != 0), local_81d == '\0')) &&
           ((*(byte *)(pfVar10 + -5) & 0x40) != 0 && pfVar10[-6] != pfVar13[0x22])) {
          local_840 = pfVar10[-1] * 0.5 + pfVar10[-4];
          local_83c = *pfVar10 * 0.5 + pfVar10[-3];
          local_838 = pfVar10[1] * 0.5 + pfVar10[-2];
          if (*(char *)(in_EAX + 0x13) != '\0') {
            fVar2 = pfVar10[-1] * pfVar10[-1];
            fVar1 = *pfVar10 * *pfVar10 + pfVar10[1] * pfVar10[1] + fVar2;
            if ((16.0 < fVar1) && ((local_81c + local_81c) * (local_81c + local_81c) < fVar1)) {
              local_828 = SQRT(fVar1);
              fVar1 = ((in_EAX[0x15] - pfVar10[-3]) * *pfVar10 +
                      (in_EAX[0x16] - pfVar10[-2]) * pfVar10[1] +
                      (in_EAX[0x14] - pfVar10[-4]) * pfVar10[-1]) /
                      (*pfVar10 * *pfVar10 + pfVar10[1] * pfVar10[1] + fVar2);
              fVar2 = local_81c / local_828;
              if ((fVar2 <= fVar1) && (fVar5 = 1.0 - fVar2, fVar2 = fVar1, fVar5 < fVar1)) {
                fVar2 = fVar5;
              }
              local_840 = fVar2 * pfVar10[-1] + pfVar10[-4];
              local_83c = fVar2 * *pfVar10 + pfVar10[-3];
              local_838 = fVar2 * pfVar10[1] + pfVar10[-2];
            }
          }
          fVar1 = SQRT((local_840 - pfVar13[0x24]) * (local_840 - pfVar13[0x24]) +
                       (local_83c - pfVar13[0x25]) * (local_83c - pfVar13[0x25]) +
                       (local_838 - pfVar13[0x26]) * (local_838 - pfVar13[0x26]));
          local_814 = fVar1 + pfVar13[0x29];
          if (*(char *)(in_EAX + 9) == '\0') {
            fVar14 = (float10)fVar1;
            local_830 = 0.0;
          }
          else {
            fVar14 = (float10)FUN_0043b3b0(&local_830);
            fVar14 = (fVar14 + (float10)1.0) * (float10)fVar1;
            if (pfVar13[0x28] <= local_830) {
              local_830 = pfVar13[0x28];
            }
          }
          fVar2 = (float)(fVar14 + (float10)pfVar13[0x2a]);
          local_834 = fVar2;
          if (*(char *)(in_EAX + 0x13) != '\0') {
            local_824 = SQRT((in_EAX[0x14] - local_840) * (in_EAX[0x14] - local_840) +
                             (in_EAX[0x15] - local_83c) * (in_EAX[0x15] - local_83c) +
                             (in_EAX[0x16] - local_838) * (in_EAX[0x16] - local_838));
            local_834 = local_824 + fVar2;
          }
          local_828 = (float)__ftol();
          if (((int)local_828 < 0x7fff) &&
             ((*(char *)(in_EAX + 0x10) == '\0' || (local_814 <= in_EAX[0x11])))) {
            uVar7 = (*(ushort *)(pfVar10 + -6) & 0x1ff) << 3;
            sVar11 = *(short *)((int)in_EAX + (short)uVar7 * 2 + 0xe08a);
            while (sVar11 != -1) {
              iVar8 = (int)sVar11;
              if (in_EAX[iVar8 * 0xd + 0x23] == pfVar10[-6]) {
                if (((int)*(short *)(in_EAX + iVar8 * 0xd + 0x2c) <= (int)local_828) ||
                   (*(short *)(in_EAX + iVar8 * 0xd + 0x2d) == -1)) goto LAB_0043aef7;
                goto LAB_0043ad7d;
              }
              uVar7 = uVar7 + 1 & 0xfff;
              sVar11 = *(short *)((int)in_EAX + (short)uVar7 * 2 + 0xe08a);
            }
            sVar11 = *(short *)(in_EAX + 0x20);
            if (sVar11 < 0x400) {
              *(short *)(in_EAX + 0x20) = sVar11 + 1;
              *(short *)((int)in_EAX + (short)uVar7 * 2 + 0xe08a) = sVar11;
              *(undefined2 *)(in_EAX + sVar11 * 0xd + 0x2d) = 0xffff;
LAB_0043ad7d:
              if (sVar11 != -1) {
                iVar8 = (int)sVar11;
                *(undefined2 *)((int)in_EAX + iVar8 * 0x34 + 0x86) = (undefined2)local_804;
                in_EAX[iVar8 * 0xd + 0x22] = pfVar13[0x23];
                in_EAX[iVar8 * 0xd + 0x23] = pfVar10[-6];
                in_EAX[iVar8 * 0xd + 0x24] = local_840;
                in_EAX[iVar8 * 0xd + 0x25] = local_83c;
                in_EAX[iVar8 * 0xd + 0x26] = local_838;
                in_EAX[iVar8 * 0xd + 0x27] = fVar1;
                in_EAX[iVar8 * 0xd + 0x28] = local_830;
                in_EAX[iVar8 * 0xd + 0x29] = local_814;
                in_EAX[iVar8 * 0xd + 0x2a] = fVar2;
                in_EAX[iVar8 * 0xd + 0x2b] = local_834;
                uVar9 = SUB42(local_828,0);
                *(undefined2 *)(in_EAX + iVar8 * 0xd + 0x2c) = uVar9;
                *(short *)((int)in_EAX + iVar8 * 0x34 + 0xb2) =
                     *(short *)((int)local_82c + 0xb2) + 1;
                if (*(short *)(in_EAX + iVar8 * 0xd + 0x2d) == -1) {
                  sVar4 = *(short *)(in_EAX + 0x3421);
                  if (sVar4 < 0x400) {
                    *(short *)(in_EAX + 0x3421) = sVar4 + 1;
                    *(undefined2 *)(in_EAX + sVar4 + 0x3422) = uVar9;
                    *(short *)((int)in_EAX + sVar4 * 4 + 0xd086) = sVar11;
                    goto LAB_0043ae6a;
                  }
                }
                else {
                  *(undefined2 *)(in_EAX + *(short *)(in_EAX + iVar8 * 0xd + 0x2d) + 0x3422) = uVar9
                  ;
LAB_0043ae6a:
                  path_find_heap_sift_up();
                }
                pfVar13 = local_82c;
                if (*(char *)(in_EAX + 0x13) != '\0') {
                  fVar14 = (float10)local_824;
                  local_80c = in_EAX[iVar8 * 0xd + 0x25];
                  local_810 = in_EAX[iVar8 * 0xd + 0x24];
                  local_808 = in_EAX[iVar8 * 0xd + 0x26];
                  if (local_824 < 4.0) {
                    fVar14 = (float10)FUN_0043b130(in_EAX + 0x14,&local_810);
                  }
                  pfVar13 = local_82c;
                  if (fVar14 < (float10)in_EAX[0x1b]) {
                    in_EAX[0x1b] = (float)fVar14;
                    in_EAX[0x1d] = local_810;
                    in_EAX[0x1e] = local_80c;
                    *(short *)(in_EAX + 0x1a) = sVar11;
                    in_EAX[0x1f] = local_808;
                    in_EAX[0x1c] = local_834;
                  }
                }
              }
            }
          }
        }
LAB_0043aef7:
        pfVar10 = pfVar10 + 8;
        local_818 = local_818 - 1;
      } while (local_818 != 0);
      local_818 = 0;
    }
  } while( true );
}
#endif
