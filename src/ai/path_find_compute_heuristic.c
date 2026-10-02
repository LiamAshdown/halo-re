// path_find_compute_heuristic  (Ghidra: path_find_compute_heuristic, renamed)
// address 0x43a310, size 439 bytes
// name confidence: 0.45  rewrite confidence: 0.9 (VERIFIED against 0x43a310 (hash lookup, distance + leash, avoid-sphere secondary min, reversed-chain 0.8 look-ahead, not-found defaults))
// evidence: types/ai.h path_find_node.position(+0x0c)/cost(+0x18)/unknown_1c(FLT_MAX
//   sentinel)/unknown_20; path_find_context's unnamed +0x24 flag and +0x28 position (see
//   path_find_set_avoid_sphere.c / path_find_request.have_avoid_sphere/avoid_position, whose fields this function reads back, confirming
//   that guess). Calls path_find_hash_lookup_vertex @0x43b2b0 (this rewrite),
//   path_find_closest_point_on_segment (0x43b2f0, a math helper this task's skip list
//   excludes from rewriting) and vector3d_normalize_with_length (established elsewhere).
//   phase-4 summary "computes the pathfinding heuristic distance from a search node to its
//   goal, including an optional leash radius and look-ahead direction."
// register convention: EDI -> context; stack -> point, out_distance, out_secondary,
//   out_direction.
//   // blam-cc: EDI -> context, stack -> point, out_distance, out_secondary, out_direction
//
// VERIFIED against disassembly 0x43a310..0x43a4c5 (2026-09-30): path_find_hash_lookup_vertex takes ESI = vertex_id, EDX = context;
// the look-ahead walk stores each node's child in node.unknown_00 while retracing the parent chain and accumulates node.cost until
// 0.8 (the node position that pushes it over becomes the aim point, else the query point); the squared-distance terms are summed
// in the x87 order shown.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real_point3d *global_origin3d_pointer; // 0x00696714, a pointer to a constant vector (UNSURE: likely but not confirmed identical to the {1,0,0} constant at 0x00696718 referenced elsewhere in this module)

extern double sqrt(double x); // FSQRT
extern int16_t path_find_hash_lookup_vertex(path_find_context *context, uint32_t vertex_id); // 0x43b2b0, EDX, ESI
extern void path_find_closest_point_on_segment(const real_point3d *point, const real_point3d *segment_start,
    const real_point3d *segment_end, real_point3d *out); // 0x43b2f0, EAX, ECX, EDX, ESI
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, see header UNSURE on its own argument

// blam-cc: EDI -> context, EAX -> vertex_id (the surface the point lies on), stack -> point, out_distance,
//   out_secondary, out_direction
uint8_t path_find_compute_heuristic(path_find_context *context, uint32_t vertex_id, real_point3d *point, float *out_distance,
                                    float *out_secondary, real_vector3d *out_direction)
{
    int16_t node_index;
    path_find_node *node;
    float dx, dy, dz;
    float leash;
    float secondary;

    node_index = path_find_hash_lookup_vertex(context, vertex_id); // 0x43a314: ESI = EAX
    if (node_index == -1) {
        if (out_secondary != 0) {
            *out_secondary = 3.4028235e+38f;
        }
        if (out_direction != 0) {
            *out_direction = *(real_vector3d *)global_origin3d_pointer;
        }
        *out_distance = 3.4028235e+38f;
        return 0;
    }

    node = &context->nodes[node_index];
    dx = point->x - node->position.x;
    dy = point->y - node->position.y;
    dz = point->z - node->position.z;
    leash = node->travelled_distance;

    secondary = 0.0f;
    if (((path_find_request *)context)->have_avoid_sphere != 0) {
        float closest_x, closest_y, closest_z; // the closest point, [esp+0x14]
        real_point3d closest;

        // 0x43a377: EAX = the avoid sphere centre (context +0x28), ECX = the node position, EDX = the point
        path_find_closest_point_on_segment((real_point3d *)((uint8_t *)context + 0x28), &node->position, point,
            &closest);
        closest_x = closest.x;
        closest_y = closest.y;
        closest_z = closest.z;
        {
            float fx = closest_x - ((path_find_request *)context)->avoid_position.x;
            float fy = closest_y - ((path_find_request *)context)->avoid_position.y;
            float fz = closest_z - ((path_find_request *)context)->avoid_position.z;
            secondary = (float)sqrt(fz * fz + fy * fy + fx * fx); // x87 term order (0x43a399)
        }
        if (node->avoid_distance < secondary) {
            secondary = node->avoid_distance;
        }
    }

    if (out_secondary != 0) {
        *out_secondary = secondary;
    }
    *out_distance = (float)sqrt(dz * dz + dx * dx + dy * dy) + leash; // x87 term order (0x43a356)

    if (out_direction != 0) {
        int16_t prev = -1;
        int16_t cur = node_index;
        path_find_node *cur_node;
        float accumulated = 0.0f;
        real_point3d *lookahead_position = point;

        do {
            cur_node = &context->nodes[cur];
            cur_node->unknown_00 = prev;
            prev = cur;
            cur = cur_node->parent;
        } while (cur_node->parent != -1);

        cur = prev;
        cur_node = &context->nodes[cur];
        while (cur != -1) {
            if (0.8f <= accumulated) {
                lookahead_position = &cur_node->position;
                break;
            }
            accumulated = accumulated + cur_node->cost;
            cur_node = &context->nodes[cur];
            cur = cur_node->unknown_00;
        }

        out_direction->i = lookahead_position->x - context->start_position.x;
        out_direction->j = lookahead_position->y - context->start_position.y;
        out_direction->k = lookahead_position->z - context->start_position.z;
        vector3d_normalize_with_length(out_direction);
    }
    return 1;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043a310 @ 0x43a310) ----
undefined4 FUN_0043a310(float *param_1,float *param_2,float *param_3,float *param_4)

{
  int iVar1;
  short *psVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  undefined *puVar10;
  short sVar11;
  short sVar12;
  short sVar13;
  int unaff_EDI;
  float local_c;
  float local_8;
  float local_4;

  sVar11 = FUN_0043b2b0();
  if (sVar11 == -1) {
    if (param_3 != (float *)0x0) {
      *param_3 = 3.4028235e+38;
    }
    puVar10 = PTR_DAT_00696714;
    if (param_4 != (float *)0x0) {
      *param_4 = *(float *)PTR_DAT_00696714;
      param_4[1] = *(float *)(puVar10 + 4);
      param_4[2] = *(float *)(puVar10 + 8);
    }
    *param_2 = 3.4028235e+38;
    return 0;
  }
  iVar1 = sVar11 * 0x34 + 0x84 + unaff_EDI;
  fVar5 = *param_1 - *(float *)(iVar1 + 0xc);
  fVar6 = param_1[1] - *(float *)(iVar1 + 0x10);
  fVar7 = param_1[2] - *(float *)(iVar1 + 0x14);
  fVar3 = *(float *)(iVar1 + 0x20);
  if (*(char *)(unaff_EDI + 0x24) == '\0') {
    fVar4 = 0.0;
  }
  else {
    path_find_closest_point_on_segment();
    fVar4 = local_c - *(float *)(unaff_EDI + 0x28);
    fVar9 = local_8 - *(float *)(unaff_EDI + 0x2c);
    fVar8 = local_4 - *(float *)(unaff_EDI + 0x30);
    fVar4 = SQRT(fVar4 * fVar4 + fVar9 * fVar9 + fVar8 * fVar8);
    if (*(float *)(iVar1 + 0x1c) < fVar4) {
      fVar4 = *(float *)(iVar1 + 0x1c);
    }
  }
  if (param_3 != (float *)0x0) {
    *param_3 = fVar4;
  }
  *param_2 = SQRT(fVar6 * fVar6 + fVar5 * fVar5 + fVar7 * fVar7) + fVar3;
  if (param_4 != (float *)0x0) {
    fVar3 = 0.0;
    sVar13 = -1;
    do {
      sVar12 = sVar11;
      psVar2 = (short *)(sVar12 * 0x34 + 0x84 + unaff_EDI);
      *psVar2 = sVar13;
      sVar11 = psVar2[1];
      sVar13 = sVar12;
    } while (psVar2[1] != -1);
    while (sVar12 != -1) {
      if (0.8 <= fVar3) {
        if (sVar12 != -1) {
          param_1 = (float *)(psVar2 + 6);
        }
        break;
      }
      fVar3 = fVar3 + *(float *)(sVar12 * 0x34 + 0x9c + unaff_EDI);
      psVar2 = (short *)(sVar12 * 0x34 + 0x84 + unaff_EDI);
      sVar12 = *psVar2;
    }
    *param_4 = *param_1 - *(float *)(unaff_EDI + 0x14);
    param_4[1] = param_1[1] - *(float *)(unaff_EDI + 0x18);
    param_4[2] = param_1[2] - *(float *)(unaff_EDI + 0x1c);
    vector3d_normalize_with_length();
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
