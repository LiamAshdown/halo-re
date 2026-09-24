// path_find_simplify_waypoints  (Ghidra: path_find_simplify_waypoints, renamed)
// address 0x43cc00, size 846 bytes
// name confidence: 0.4   rewrite confidence: 0.1
// evidence: types/ai.h path_find_context.start_position(+0x14, confirmed: `local_94`/
// `local_90` here are its x/y) / start_vertex_id(+0x20, confirmed: `local_8c`) /
// path_find_request.ignores_glass(+0x04) / bsp_generation(+0x64, "used as an opaque handle"
// per the module header). phase-4 summary "simplifies a raw waypoint path into a small set
// of shortcut points by greedily extending clear segments and snapping to occluding navmesh
// corners." Calls ai_search_find_circle_tangent_point/0x43d100/0x43d240/0x43d4b0/0x43d9b0/0x43de90 (all this
// rewrite) and decal_plane_solve_third_axis (outside this rewrite's range).
//
// This is one of the least confident rewrites in this batch: it is a greedy path-shortcut
// search whose helper calls (path_find_trace_cluster_boundary, ai_search_choose_shorter_corner, path_find_trace_bsp_boundary, path_find_test_segment_unobstructed) each
// have their own low-confidence, partially-guessed signatures, and several float literals
// here are Ghidra's hex-encoded bit patterns for ordinary constants (0x3e99999a = 0.3,
// 0x3eb33333 = 0.35) rather than raw integers. Reproduced with those constants decoded, and
// with each helper called using the reduced arity Ghidra shows at this call site rather than
// any other file's canonical signature for the same address, per this module's established
// convention for that situation.
//
// register convention: stack -> the six Ghidra-recognized formal parameters.
//   // blam-cc: stack -> context, waypoint_count, waypoints, out_count, out_waypoints,
//   //   out_success
// reconciled: R06 path_find_context.bsp_generation -> structure_bsp (0x00746f9c, the resident ScenarioStructureBSP pointer)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h> // uintptr_t

extern uint8_t path_find_test_segment_unobstructed(uint8_t ignores_glass, int32_t start_vertex, real_point3d *point, uint32_t param4,
                            float margin, uint32_t param6, void *out_result); // 0x43de90
extern uint8_t path_find_trace_cluster_boundary(void *start, float margin, uint32_t direction, uint8_t ignores_glass,
                            void *out_result); // 0x43d4b0
extern uint8_t ai_search_choose_shorter_corner(void *a, real_point3d *b, void *out_result); // 0x43d240
extern void ai_search_find_circle_tangent_point(float param1, uint32_t param2); // 0x43cf60
extern void ai_search_find_circle_portal_crossing(float param1); // 0x43d100
extern uint8_t path_find_trace_bsp_boundary(uint32_t bsp_generation, uint8_t ignores_glass, void *from, int32_t from_vertex,
                            void *to, uint32_t param6, void *out_result); // 0x43d9b0
extern real_point3d *decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis,
    const real_plane3d *plane, const real_point2d *known);
    // 0x44d860, src/math; blam-cc: stack out, AL component_sign, SI dominant_axis, EBX plane, EDI known

// blam-cc: stack -> context, waypoint_count, waypoints, out_count, out_waypoints, out_success
void path_find_simplify_waypoints(path_find_context *context, int16_t waypoint_count, int32_t *waypoints,
                                  int16_t *out_count, int32_t *out_waypoints, uint8_t *out_success)
{
    if (waypoint_count < 2) {
        *out_count = 1;
        out_waypoints[0] = waypoints[0];
        out_waypoints[1] = waypoints[1];
        out_waypoints[2] = waypoints[2];
        out_waypoints[3] = waypoints[3];
        return;
    }

    {
        float start_x = *(float *)&context->start_position.x;
        float start_y = context->start_position.y;
        int32_t start_vertex = context->start_vertex_id;
        int16_t out_index = 0;
        int16_t cursor = 1;
        uint8_t reached_end = 0;
        uint8_t ignores_glass = *(uint8_t *)((uint8_t *)context + 4);

        for (;;) {
            int16_t best_index = -1;
            int16_t best_cursor = -1;
            uint8_t clear = 0;
            int32_t *entry;
            path_find_simplify_scratch trace_scratch;

            if (waypoint_count <= cursor) {
                break;
            }

            entry = waypoints + cursor * 4;
            do {
                real_point3d point;
                point.x = *(float *)&entry[1]; point.y = *(float *)&entry[2]; point.z = *(float *)&entry[3];

                if (path_find_test_segment_unobstructed(ignores_glass, start_vertex, &point, (uint32_t)entry[0], 0.3f, 1, &trace_scratch) == 0) {
                    if (clear) {
                        best_index = -1;
                        best_cursor = -1;
                        clear = 0;
                    }
                } else if (!clear) {
                    best_cursor = cursor;
                    clear = 1;
                    best_index = trace_scratch.result;
                }

                cursor = cursor + 1;
                entry = entry + 4;
            } while (cursor < waypoint_count);

            if ((!clear) || (best_index == -1)) {
                break;
            }

            {
                path_find_simplify_scratch scratch_a, scratch_b, corner_scratch, bend_scratch;
                real_point3d origin;
                real_point3d bend_point;
                uint8_t corner_side;
                int32_t new_vertex;

                origin.x = start_x; origin.y = start_y; origin.z = 0.0f;

                if ((path_find_trace_cluster_boundary(&origin, 0.3f, 1, ignores_glass, &scratch_a) == 0) ||
                    (path_find_trace_cluster_boundary(&origin, 0.3f, 0, ignores_glass, &scratch_b) == 0)) {
                    break;
                }

                corner_side = ai_search_choose_shorter_corner(&scratch_b, (real_point3d *)(waypoints + best_cursor * 4 + 1), &corner_scratch);
                ai_search_find_circle_tangent_point(0.35f, corner_side);
                ai_search_find_circle_tangent_point(0.35f, (uint32_t)(corner_side == 0));
                ai_search_find_circle_portal_crossing(0.35f);

                bend_point.x = start_x; bend_point.y = start_y; bend_point.z = 0.0f; // point_a before the bend, see header
                start_x = corner_scratch.unknown_00[0]; // UNSURE, see header

                if (start_vertex == -1) {
                    start_vertex = -1;
                } else {
                    path_find_trace_bsp_boundary((uint32_t)context->structure_bsp, ignores_glass, &bend_point, start_vertex,
                                &origin, 0xffffffff, &bend_scratch);
                    if (bend_scratch.result != -1) {
                        start_vertex = bend_scratch.result;
                    }
                }

                {
                    int32_t *out_entry = out_waypoints + out_index * 4;
                    uint8_t *collision_bsp = (uint8_t *)(uintptr_t)*(uint32_t *)((uint8_t *)(uintptr_t)context->structure_bsp + 0xb4);
                    const real_plane3d *planes = (const real_plane3d *)(uintptr_t)*(uint32_t *)(collision_bsp + 0x10);
                    const uint32_t *surfaces = (const uint32_t *)(uintptr_t)*(uint32_t *)(collision_bsp + 0x40);

                    out_index = out_index + 1;
                    // 0x43ce58..0x43cea3: lift `origin` (EDI) onto the plane of surface start_vertex
                    // (ESI, 0xc-byte surfaces of the structure BSP's collision BSP at +0xb4), solving z,
                    // into out_entry[1..3]
                    decal_plane_solve_third_axis((real_point3d *)(out_entry + 1), 1, 2,
                        &planes[surfaces[start_vertex * 3] & 0x7fffffff], (const real_point2d *)&origin);
                    out_entry[0] = start_vertex;
                }

                cursor = best_cursor;
                if (3 < out_index) {
                    reached_end = 0;
                    goto done;
                }
            }
        }

        {
            int32_t *last = waypoints + waypoint_count * 4 - 4;
            int32_t *out_entry = out_waypoints + out_index * 4;
            out_entry[0] = last[0];
            out_entry[1] = last[1];
            out_entry[2] = last[2];
            out_index = out_index + 1;
            out_entry[3] = last[3];
            reached_end = 1;
        }

    done:
        *out_count = out_index;
        if (!reached_end) {
            *out_success = 0;
        }
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043cc00 @ 0x43cc00) ----
void FUN_0043cc00(int param_1,short param_2,undefined4 *param_3,undefined2 *param_4,
                 undefined4 *param_5,undefined1 *param_6)

{
  undefined1 uVar1;
  bool bVar2;
  bool bVar3;
  char cVar4;
  char cVar5;
  int iVar6;
  undefined4 *puVar7;
  undefined4 local_94;
  undefined4 local_90;
  int local_8c;
  int local_88;
  int *local_84;
  int local_80;
  undefined4 local_7c;
  undefined1 local_78 [8];
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined1 local_60 [8];
  undefined1 local_58 [24];
  undefined1 local_40 [4];
  undefined4 local_3c;
  undefined4 local_38;
  int local_30;
  undefined1 local_24 [20];
  int local_10;

  if (param_2 < 2) {
    *param_4 = 1;
    *param_5 = *param_3;
    param_5[1] = param_3[1];
    param_5[2] = param_3[2];
    param_5[3] = param_3[3];
  }
  else {
    local_90 = *(undefined4 *)(param_1 + 0x18);
    local_88 = 0;
    bVar3 = false;
    local_94 = *(undefined4 *)(param_1 + 0x14);
    local_8c = *(int *)(param_1 + 0x20);
    local_84 = (int *)1;
    while( true ) {
      iVar6 = -1;
      local_80 = -1;
      bVar2 = false;
      if (param_2 <= (short)local_84) break;
      puVar7 = param_3 + (short)local_84 * 4;
      do {
        cVar4 = FUN_0043de90(*(undefined1 *)(param_1 + 4),local_8c,puVar7 + 1,*puVar7,0x3e99999a,1,
                             local_24);
        if (cVar4 == '\0') {
          if (bVar2) {
            iVar6 = -1;
            local_80 = -1;
            bVar2 = false;
          }
        }
        else if (!bVar2) {
          local_80 = (int)local_84;
          bVar2 = true;
          iVar6 = local_10;
        }
        local_84 = (int *)((int)local_84 + 1);
        puVar7 = puVar7 + 4;
      } while ((short)local_84 < param_2);
      if ((!bVar2) || (iVar6 == -1)) break;
      uVar1 = *(undefined1 *)(param_1 + 4);
      cVar4 = FUN_0043d4b0(&local_94,0x3e99999a,1,uVar1,local_60);
      cVar5 = FUN_0043d4b0(&local_94,0x3e99999a,0,uVar1,local_58);
      if ((cVar5 == '\0') || (cVar4 == '\0')) goto LAB_0043cf06;
      cVar4 = FUN_0043d240(local_58,param_3 + (short)local_80 * 4 + 1,local_78);
      local_7c = CONCAT31(local_7c._1_3_,cVar4);
      FUN_0043cf60(0x3eb33333,local_7c);
      FUN_0043cf60(0x3eb33333,cVar4 == '\0');
      FUN_0043d100(0x3eb33333);
      local_68 = local_94;
      local_64 = local_90;
      local_94 = local_70;
      local_90 = local_6c;
      if (local_8c == -1) {
        local_8c = -1;
      }
      else {
        FUN_0043d9b0(*(undefined4 *)(param_1 + 100),*(undefined1 *)(param_1 + 4),&local_68,local_8c,
                     &local_94,0xffffffff,local_40);
        local_94 = local_3c;
        local_90 = local_38;
        if (local_30 != -1) {
          local_8c = local_30;
        }
      }
      local_84 = param_5 + (short)local_88 * 4;
      local_88 = local_88 + 1;
      FUN_0044d860(local_84 + 1);
      *local_84 = local_8c;
      local_84 = (int *)local_80;
      if (3 < (short)local_88) goto LAB_0043cf06;
    }
    param_3 = param_3 + param_2 * 4 + -4;
    param_5 = param_5 + (short)local_88 * 4;
    *param_5 = *param_3;
    param_5[1] = param_3[1];
    param_5[2] = param_3[2];
    local_88 = local_88 + 1;
    param_5[3] = param_3[3];
    bVar3 = true;
LAB_0043cf06:
    *param_4 = (short)local_88;
    if (!bVar3) {
      *param_6 = 0;
      return;
    }
  }
  return;
}
#endif
