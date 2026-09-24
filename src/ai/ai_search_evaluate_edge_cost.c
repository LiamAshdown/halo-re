// ai_search_evaluate_edge_cost  (Ghidra: ai_search_evaluate_edge_cost, renamed)
// address 0x43b830, size 560 bytes
// name confidence: 0.3   rewrite confidence: 0.1
// evidence: phase-4 summary "evaluates the cheapest way (direct or bending around an
// obstacle) to move between two points for the AI point search, writing the chosen cost and
// heading." Calls ai_search_find_nearest_visible_point (ai_search_find_nearest_visible_point, this rewrite) and
// path_find_trace_cluster_boundary_from_vertex (this rewrite's own path_find_trace_cluster_boundary_from_vertex).
//
// This is one of the least confident rewrites in this batch. path_find_trace_cluster_boundary_from_vertex is called six
// times in the original with three visibly different argument counts (3, then 6, then 3,
// then 6, then 3, then 3 again) even though it can only be one real function -- Ghidra's
// register/stack-slot reuse tracking has broken down for this function far more than
// anywhere else in this cluster, and the three-argument call sites cannot simply be
// "reusing" the direction/distance/out-result operands the six-argument call sites set up,
// because the first three-argument call happens *before* any of those locals exist. This
// could not be resolved without a disassembly of this function, which was not available
// here. What follows preserves every visible operation and operand exactly as Ghidra shows
// it, using a separate, reduced-arity local declaration of path_find_trace_cluster_boundary_from_vertex per distinct call
// shape (matching this module's established convention for a callee whose visible arity
// differs by call site, taken to its logical extreme here). The `param_12[2] == -NAN` /
// `local_18._2_2_` constructs are Ghidra sub-register artifacts on what is really one
// `float` and one `int32_t` pair of scratch slots; reproduced with plain float/int32_t
// locals and a same-bit-pattern NaN sentinel check (`!= !=` would not reliably match a raw
// NaN bit pattern in portable C, so the sentinel is compared through a union instead).
//
// register convention: EBX -> a caller-owned {x, y} pair (the segment's own direction,
//   read but never seen assigned in this function -- inherited from the caller); the rest
//   are the twelve Ghidra-recognized stack parameters.
//   // blam-cc: EBX -> unaff_direction (inherited, not a formal parameter), stack -> the
//   //   twelve parameters below

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern uint8_t ai_search_find_nearest_visible_point(uint32_t param_3, uint32_t param_4, real_point2d *point); // 0x43c8f0
// 0x43d790 is path_find_trace_cluster_boundary_from_vertex (src/ai/path_find_trace_cluster_boundary_from_vertex.c).
// Ghidra shows it called from here with three operands at some sites and six at others, so it is
// declared twice under suffixed aliases rather than picking one arity. UNSURE which is real.
extern uint8_t path_find_trace_cluster_boundary_from_vertex_3(uint32_t cluster, real_point2d *point, uint32_t param6); // 0x43d790
extern uint8_t path_find_trace_cluster_boundary_from_vertex_6(uint32_t cluster, real_point2d *point, uint32_t param6,
                              real_vector2d *direction, float distance, float *out_result); // 0x43d790

// blam-cc: EBX -> unaff_direction (inherited), stack -> cluster_a, cluster_b, param_3,
//   param_4, point, param6, distance, base_cost, skip_direct, apply_offset, param_11,
//   out_result
uint8_t ai_search_evaluate_edge_cost(uint32_t cluster_a, uint32_t cluster_b, uint32_t param_3, uint32_t param_4,
                                     real_point2d *point, uint32_t param6, float distance, float base_cost,
                                     char skip_direct, char apply_offset, uint32_t param_11,
                                     ai_search_edge_result *out_result)
{
    real_vector2d *unaff_direction; // UNSURE: inherited from the caller, see header
    float local_1c; // direction.i, reused as a scratch x/i component throughout
    int32_t local_18;
    float local_14;
    float local_10;
    float local_c; // path_find_trace_cluster_boundary_from_vertex's own out-distance
    float local_8; // UNSURE: never assigned anywhere in the original; read at the end regardless
    float local_4; // path_find_trace_cluster_boundary_from_vertex's own out-point-id-as-float companion

    unaff_direction = 0; // UNSURE: cannot be recovered without this function's caller; see header

    out_result->cost = base_cost;
    out_result->heading_x = -1.0f; // -NAN sentinel, see header
    out_result->heading_y = -1.0f; // -NAN sentinel, see header
    out_result->point_id = -1;
    *(int16_t *)&out_result->unknown_0e = -1;

    if (apply_offset != 0) {
        out_result->cost = base_cost - distance;
    }

    if (skip_direct == 0) {
        if ((path_find_trace_cluster_boundary_from_vertex_3(cluster_b, point, param6) != 0) && (local_c < out_result->cost)) {
            out_result->cost = local_c;
            out_result->heading_y = local_4;
        }

        local_18 = *(int32_t *)&unaff_direction->i;
        local_1c = -unaff_direction->j;
        local_14 = local_1c * distance + point->x;
        local_10 = *(float *)&local_18 * distance + point->y;
        path_find_trace_cluster_boundary_from_vertex_6(cluster_b, point, param6, (real_vector2d *)&local_1c, distance, &local_c);
        {
            real_point2d p; p.x = local_14; p.y = local_10;
            if ((path_find_trace_cluster_boundary_from_vertex_3(cluster_b, &p, (uint32_t)local_8) != 0) && (local_c < out_result->cost)) {
                out_result->cost = local_c;
                out_result->heading_y = local_4;
            }
        }

        local_14 = local_1c * -distance + point->x;
        local_10 = -distance * *(float *)&local_18 + point->y;
        path_find_trace_cluster_boundary_from_vertex_6(cluster_b, point, param6, (real_vector2d *)&local_1c, distance, &local_c);
        {
            real_point2d p; p.x = local_14; p.y = local_10;
            if ((path_find_trace_cluster_boundary_from_vertex_3(cluster_b, &p, (uint32_t)local_8) != 0) && (local_c < out_result->cost)) {
                out_result->cost = local_c;
                out_result->heading_y = local_4;
            }
        }
    }

    if ((ai_search_find_nearest_visible_point(param_3, param_4, point) != 0) && (local_1c < out_result->cost)) {
        out_result->cost = local_1c;
        out_result->heading_y = -1.0f; // -NAN sentinel
        *(int16_t *)&out_result->point_id = (int16_t)local_18;
        *(int16_t *)&out_result->unknown_0e = (int16_t)((uint32_t)local_18 >> 16);
    }

    {
        uint8_t reached;
        if ((out_result->heading_y == -1.0f) && (out_result->point_id == -1)) {
            reached = 0;
            out_result->cost = base_cost;
        } else {
            reached = 1;
        }

        path_find_trace_cluster_boundary_from_vertex_3(cluster_b, point, param6);
        out_result->heading_x = local_8;
        return reached;
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043b830 @ 0x43b830) ----
undefined1
FUN_0043b830(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            float *param_5,undefined4 param_6,float param_7,float param_8,char param_9,char param_10
            ,undefined4 param_11,float *param_12)

{
  char cVar1;
  float *unaff_EBX;
  float local_1c;
  undefined4 local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  *param_12 = param_8;
  param_12[1] = -NAN;
  param_12[2] = -NAN;
  *(undefined2 *)(param_12 + 3) = 0xffff;
  *(undefined2 *)((int)param_12 + 0xe) = 0xffff;
  if (param_10 != '\0') {
    *param_12 = param_8 - param_7;
  }
  if (param_9 == '\0') {
    cVar1 = FUN_0043d790(param_2,param_5,param_6);
    if ((cVar1 != '\0') && (local_c < *param_12)) {
      *param_12 = local_c;
      param_12[2] = local_4;
    }
    local_18 = *unaff_EBX;
    local_1c = -unaff_EBX[1];
    local_14 = local_1c * param_7 + *param_5;
    local_10 = local_18 * param_7 + param_5[1];
    FUN_0043d790(param_2,param_5,param_6,&local_1c,param_7,&local_c);
    cVar1 = FUN_0043d790(param_2,&local_14,local_8);
    if ((cVar1 != '\0') && (local_c < *param_12)) {
      *param_12 = local_c;
      param_12[2] = local_4;
    }
    local_14 = local_1c * -param_7 + *param_5;
    local_10 = -param_7 * local_18 + param_5[1];
    FUN_0043d790(param_2,param_5,param_6,&local_1c,param_7,&local_c);
    cVar1 = FUN_0043d790(param_2,&local_14,local_8);
    if ((cVar1 != '\0') && (local_c < *param_12)) {
      *param_12 = local_c;
      param_12[2] = local_4;
    }
  }
  cVar1 = FUN_0043c8f0(param_3,param_4,param_5);
  if ((cVar1 != '\0') && (local_1c < *param_12)) {
    *param_12 = local_1c;
    param_12[2] = -NAN;
    *(undefined2 *)(param_12 + 3) = (undefined2)local_18;
    *(undefined2 *)((int)param_12 + 0xe) = local_18._2_2_;
  }
  if ((param_12[2] == -NAN) && (*(short *)(param_12 + 3) == -1)) {
    param_10 = 0;
    *param_12 = param_8;
  }
  else {
    param_10 = 1;
  }
  FUN_0043d790(param_2,param_5,param_6);
  param_12[1] = local_8;
  return param_10;
}
#endif
