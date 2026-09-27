// path_find_run  (Ghidra: path_find_run, already named)
// address 0x43a8b0, size 1716 bytes
// name confidence: 0.55  rewrite confidence: 0.85
// REWRITTEN from objdump 0x43a8b0..0x43af63: 0x43a8b0 resets the search (node count 0, heap count 1, the vertex
//   hash to -1, best node -1, best cost and +0x70 FLT_MAX) and, when path_find_push_start_node (ESI) accepts the
//   start, tail-jumps into the search loop at 0x43a900 (path_find_search below). The draft handed the context
//   itself to path_find_gather_adjacent_edges instead of the map at +0x64 (crash reading its tables).
// The search: the radius is max(request radius +0x00, 0.2). Pop heap[1] (the last entry moves to the top and
//   sifts down, ECX 1); with a goal, reaching the goal vertex (+0x5c) finishes with the goal position as best, and a
//   node whose f (+0x28) exceeds 10 * max(best cost, 5) + best f (+0x70) ends the search. For each adjacent
//   edge (gathered from the map, up to 64): skip the surface the node came from (+0x04), edges without flag 0x40,
//   and (unless the request ignores glass, +0x04) glass edges (0x80) whose breakable surface (map bsp +0x40 record
//   +8 bit 8, index +9) is still intact. The candidate is the edge midpoint or, with a goal on a long edge
//   (length^2 > 16 and > (2r)^2), the projection of the goal onto the edge clamped to [r/len, 1 - r/len].
//   step = |candidate - node|; travelled = step + node +0x20; cost = step, times (1 + avoid penalty) with the
//   avoid sphere (+0x24; the running minimum avoid distance is kept at +0x1c); g = cost + node +0x24; f = g +
//   |goal - candidate| with a goal; key = (int)(f * 10) must stay below 0x7fff; a travel limit (+0x40/+0x44)
//   applies. The vertex hash (512 buckets of 8, probed modulo 0x1000) finds an existing node (only improved while
//   open and when the key drops) or a new one (at most 0x400). The node gets parent, came-from surface, vertex,
//   position, step, avoid distance, travelled, g, f, key and waypoint + 1, and is pushed or re-keyed in the heap
//   (sift up, EAX ctx, DX slot). With a goal, candidates within 4 of it measure the goal distance on their surface
//   (0x43b130), and a closer one becomes the best (cost, position, node, f).
//   Returns best cost <= goal cost with a goal, else 1.
// blam-cc: EAX -> context

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h> // uintptr_t
#include "physics.h"

extern void path_find_heap_sift_up(path_find_context *context, int16_t index);   // 0x43af70, EAX, DX
extern void path_find_heap_sift_down(path_find_context *context, int16_t index); // 0x43b010, EAX, ECX
extern uint8_t path_find_push_start_node(path_find_context *context);            // 0x43a760, ESI
extern float path_find_vertex_distance(ScenarioStructureBSP *structure_bsp, int32_t surface, real_point3d *point_a,
    real_point3d *out_point); // 0x43b130; blam-cc: EAX structure_bsp, ECX surface, stack point_a, out_point
extern int16_t path_find_gather_adjacent_edges(void *map, int32_t vertex_id,
    path_find_adjacent_edge *out_edges); // 0x43b1c0, EAX map, stack
extern float path_find_score_avoidance_penalty(path_find_context *context, const real_point3d *segment_start,
    const real_point3d *segment_end, float *out_distance); // 0x43b3b0, EBX, ECX, EDX, stack
extern double sqrt(double x); // FSQRT
extern uint8_t *breakable_surface_state; // 0x006b8d78
extern int16_t global_structure_bsp_index; // 0x0069e8d8

static uint8_t path_find_search(path_find_context *context)
{
    path_find_request *request = (path_find_request *)context;
    float radius = (0.2f > request->pathfinding_radius) ? 0.2f : request->pathfinding_radius;
    path_find_adjacent_edge edges[64];

    while (context->heap_count > 1) {
        int16_t current = context->heap[1].node;
        path_find_node *node = &context->nodes[current];
        int16_t edge_count;
        int16_t e;

        node->heap_index = -1;
        context->heap_count--;
        if (context->heap_count > 1) {
            context->heap[1] = context->heap[context->heap_count];
            path_find_heap_sift_down(context, 1);
        }
        if (current == -1) {
            break;
        }
        if (context->have_goal) {
            float limit;

            if (node->vertex_id == context->goal_vertex_id) {
                context->best_position = context->goal_position;
                context->best_node = current;
                context->best_cost = 0.0f;
                break;
            }
            limit = (5.0f > context->best_cost) ? 5.0f : context->best_cost;
            if (node->distance > limit * 10.0f + context->unknown_70) {
                break;
            }
        }
        edge_count = path_find_gather_adjacent_edges((void *)(uintptr_t)context->structure_bsp, node->vertex_id, edges);
        for (e = 0; e < edge_count; e++) {
            path_find_adjacent_edge *edge = &edges[e];
            uint8_t passable = (uint8_t)((uint32_t)edge->edge_id != (uint32_t)node->unknown_04);
            real_point3d candidate;
            float step;
            float travelled;
            float cost;
            float avoid_distance;
            float g;
            float f;
            float goal_distance = 0.0f;
            int32_t key;
            int16_t slot;
            int16_t index = -1;
            path_find_node *next;

            if ((edge->flag & 0x40) == 0) {
                passable = 0;
            }
            if (request->ignores_glass == 0 && (edge->flag & 0x80) != 0) {
                uint8_t *map = (uint8_t *)(uintptr_t)context->structure_bsp;
                uint8_t *record = *(uint8_t **)(*(uint8_t **)(map + 0xb4) + 0x40) + edge->edge_id * 12;

                if ((record[8] & 8) != 0) {
                    uint32_t bit = record[9];
                    uint32_t word = *(uint32_t *)(breakable_surface_state + 1 +
                        ((bit >> 5) + global_structure_bsp_index * 8) * 4);

                    if ((word & (1u << (bit & 0x1f))) == 0) {
                        continue; // 0x43aa82: the glass is still intact
                    }
                }
            }
            if (!passable) {
                continue;
            }
            candidate.x = edge->direction_x * 0.5f + edge->start_x;
            candidate.y = edge->direction_y * 0.5f + edge->start_y;
            candidate.z = edge->direction_z * 0.5f + edge->start_z;
            if (context->have_goal) {
                float dx2 = edge->direction_x * edge->direction_x;
                float length2 = edge->direction_z * edge->direction_z + edge->direction_y * edge->direction_y + dx2;

                if (length2 > 16.0f && length2 > (radius + radius) * (radius + radius)) {
                    float length = (float)sqrt(length2);
                    float t = ((context->goal_position.x - edge->start_x) * edge->direction_x +
                        (context->goal_position.z - edge->start_z) * edge->direction_z +
                        (context->goal_position.y - edge->start_y) * edge->direction_y) /
                        (edge->direction_z * edge->direction_z + edge->direction_y * edge->direction_y + dx2);
                    float margin = radius / length;

                    if (t < margin) {
                        t = margin;
                    } else if (t > 1.0f - margin) {
                        t = 1.0f - margin;
                    }
                    candidate.x = t * edge->direction_x + edge->start_x;
                    candidate.y = t * edge->direction_y + edge->start_y;
                    candidate.z = t * edge->direction_z + edge->start_z;
                }
            }
            {
                float dx = candidate.x - node->position.x;
                float dy = candidate.y - node->position.y;
                float dz = candidate.z - node->position.z;

                step = (float)sqrt(dz * dz + dy * dy + dx * dx);
            }
            travelled = step + node->unknown_20;
            if (request->have_avoid_sphere) {
                cost = (path_find_score_avoidance_penalty(context, &node->position, &candidate, &avoid_distance) + 1.0f) *
                    step;
                if (!(node->unknown_1c > avoid_distance)) {
                    avoid_distance = node->unknown_1c;
                }
            } else {
                cost = step;
                avoid_distance = 0.0f;
            }
            g = cost + node->unknown_24;
            f = g;
            if (context->have_goal) {
                float dx = context->goal_position.x - candidate.x;
                float dy = context->goal_position.y - candidate.y;
                float dz = context->goal_position.z - candidate.z;

                goal_distance = (float)sqrt(dz * dz + dy * dy + dx * dx);
                f = goal_distance + g;
            }
            key = (int32_t)(f * 10.0f);
            if (key >= 0x7fff) {
                continue;
            }
            if (request->have_limit && travelled > request->limit_distance) {
                continue;
            }

            // 0x43aced: the vertex hash
            slot = (int16_t)((edge->edge_id & 0x1ff) << 3);
            while (context->vertex_hash[slot] != -1) {
                path_find_node *existing = &context->nodes[context->vertex_hash[slot]];

                if (existing->vertex_id == (uint32_t)edge->edge_id) {
                    if (key >= existing->key || existing->heap_index == -1) {
                        index = -2; // no improvement, or already closed
                    } else {
                        index = context->vertex_hash[slot];
                    }
                    break;
                }
                slot = (int16_t)((slot + 1) & 0xfff);
            }
            if (index == -2) {
                continue;
            }
            if (index == -1) {
                if (context->node_count >= 0x400) {
                    continue;
                }
                index = context->node_count++;
                context->vertex_hash[slot] = index;
                context->nodes[index].heap_index = -1;
            }

            next = &context->nodes[index];
            next->parent = current;
            next->unknown_04 = (int32_t)node->vertex_id;
            next->vertex_id = (uint32_t)edge->edge_id;
            next->position = candidate;
            next->cost = step;
            next->unknown_1c = avoid_distance;
            next->unknown_20 = travelled;
            next->unknown_24 = g;
            next->distance = f;
            next->key = (int16_t)key;
            next->waypoint = (int16_t)(node->waypoint + 1);
            if (next->heap_index != -1) {
                context->heap[next->heap_index].key = (int16_t)key;
                path_find_heap_sift_up(context, next->heap_index);
            } else if (context->heap_count < 0x400) {
                int16_t heap_slot = context->heap_count++;

                context->heap[heap_slot].key = (int16_t)key;
                context->heap[heap_slot].node = index;
                path_find_heap_sift_up(context, heap_slot);
            }

            if (context->have_goal) {
                real_point3d best_point = next->position;
                float distance = goal_distance;

                if (distance < 4.0f) {
                    distance = path_find_vertex_distance((ScenarioStructureBSP *)(uintptr_t)context->structure_bsp,
                        edge->edge_id, &context->goal_position, &best_point);
                }
                if (distance < context->best_cost) {
                    context->best_cost = distance;
                    context->best_position = best_point;
                    context->best_node = index;
                    context->unknown_70 = f;
                }
            }
        }
    }

    if (context->have_goal) {
        return (uint8_t)(context->best_cost <= context->goal_cost);
    }
    return 1;
}

uint8_t path_find_run(path_find_context *context)
{
    int32_t i;

    context->node_count = 0;
    context->heap_count = 1;
    for (i = 0; i < 0x1000; i++) {
        context->vertex_hash[i] = -1;
    }
    context->best_node = -1;
    context->best_cost = 3.4028235e+38f;
    context->unknown_70 = 3.4028235e+38f;
    if (!path_find_push_start_node(context)) {
        return 0;
    }
    return path_find_search(context);
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
