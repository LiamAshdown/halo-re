// ai_search_evaluate_edge_cost  (Ghidra: ai_search_evaluate_edge_cost, renamed)
// address 0x43b830, size 560 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (REWRITTEN from objdump 0x43b830..0x43ba5f)
// evidence: re-derived from the disassembly (0x43b830..0x43ba5f). Evaluates the cheapest way
//   to leave `point` for the AI point search: a direct boundary trace along `direction`, two
//   traces from points offset sideways by +/- distance along the perpendicular, and the nearest
//   visible search point (ai_search_find_nearest_visible_point), keeping the lowest cost.
// reconciled: R53 path_find_trace_cluster_boundary_from_vertex (0x43d790) has one signature and
//   all seven calls here push 6 stack arguments with EAX = context and clean with add esp,0x18
//   (0x30 for the paired calls): the _3/_6 aliases are gone. The parameter roles follow from
//   those calls: arg0 is the boundary-trace context (EAX of every 0x43d790 call), arg1 the
//   ignore_permission byte, arg2/arg3 the obstacle list and excluded point handed to 0x43c8f0,
//   arg5 the start surface index, arg6 the sideways distance, arg10 require_unflagged, and EBX
//   the direction (read at 0x43b8b1 as [ebx]/[ebx+4], pushed as the trace direction). The
//   result record's +0x04/+0x08 are the trace's surface_index/edge_index (ints), not headings.
//
// register convention: EBX -> direction; stack -> the twelve parameters below.
//   // blam-cc: EBX -> direction, stack -> context, ignore_permission, obstacle_list,
//   //   exclude_index, point, start_surface_index, distance, base_cost, skip_direct,
//   //   apply_offset, require_unflagged, out_result
// Observed as-is: the second sideways trace (0x43b93b..0x43b973) places its probe point at
//   point - distance * perpendicular but still walks the surfaces along +perpendicular, the
//   same direction as the first; that is what the binary does, so it is kept.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"


    // 0x43c8f0; EDI -> out_result, see its own file for the stack roles

    // 0x43d790; EAX -> context

// blam-cc: EBX -> direction, stack -> context .. out_result
uint8_t ai_search_evaluate_edge_cost(void *context, uint8_t ignore_permission,
                                     ai_search_obstacle_list *obstacle_list, int16_t exclude_index,
                                     real_point2d *point, int32_t start_surface_index, float distance,
                                     float base_cost, uint8_t skip_direct, uint8_t apply_offset,
                                     uint8_t require_unflagged, ai_search_edge_result *out_result,
                                     real_vector2d *direction)
{
    path_find_boundary_trace_result trace;
    ai_search_nearest_point_result nearest;
    real_vector2d perpendicular;
    real_point2d offset;
    uint8_t hit;

    out_result->cost = base_cost;
    out_result->surface_index = -1;
    out_result->edge_index = -1;
    out_result->point_id = -1;
    out_result->link = -1;
    if (apply_offset) {
        out_result->cost = base_cost - distance;
    }
    if (!skip_direct) {
        // 0x43b873: straight ahead from the point
        if (path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, point, start_surface_index,
                direction, out_result->cost, &trace) != 0 && out_result->cost > trace.distance) {
            out_result->cost = trace.distance;
            out_result->edge_index = trace.edge_index;
        }
        // 0x43b8b1: step sideways by +distance along the left normal, then look ahead from there
        perpendicular.i = -direction->j;
        perpendicular.j = direction->i;
        offset.x = perpendicular.i * distance + point->x;
        offset.y = perpendicular.j * distance + point->y;
        path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, point, start_surface_index,
            &perpendicular, distance, &trace);
        if (path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, &offset, trace.surface_index,
                direction, out_result->cost, &trace) != 0 && out_result->cost > trace.distance) {
            out_result->cost = trace.distance;
            out_result->edge_index = trace.edge_index;
        }
        // 0x43b93b: the other side's offset point; the binary traces its start surface along +normal again
        offset.x = perpendicular.i * -distance + point->x;
        offset.y = -distance * perpendicular.j + point->y;
        path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, point, start_surface_index,
            &perpendicular, distance, &trace);
        if (path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, &offset, trace.surface_index,
                direction, out_result->cost, &trace) != 0 && out_result->cost > trace.distance) {
            out_result->cost = trace.distance;
            out_result->edge_index = trace.edge_index;
        }
    }
    // 0x43b9b6: the nearest obstacle point along the way
    if (ai_search_find_nearest_visible_point(obstacle_list, exclude_index, point, direction, distance,
            out_result->cost, require_unflagged, &nearest) != 0 && out_result->cost > nearest.distance) {
        out_result->cost = nearest.distance;
        out_result->edge_index = -1;
        out_result->point_id = nearest.point_id;
        out_result->link = nearest.link;
    }
    if (out_result->edge_index == -1 && out_result->point_id == -1) {
        out_result->cost = base_cost;
        hit = 0;
    } else {
        hit = 1;
    }
    path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, point, start_surface_index, direction,
        out_result->cost, &trace);
    out_result->surface_index = trace.surface_index;
    return hit;
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
