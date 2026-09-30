// path_find_simplify_waypoints  (Ghidra: path_find_simplify_waypoints, renamed)
// address 0x43cc00, size 846 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// REWRITTEN from objdump 0x43cc00..0x43cf50 (the draft had lost most arguments of every helper and was never
//   called with its own). Stack: context, count, waypoints, out_count, out_waypoints, out_valid.
//   One waypoint or none: it is copied and out_count = 1 (out_valid untouched). Otherwise, starting from the
//   request's start point (+0x14, 2D) and surface (+0x20): scan the remaining waypoints with
//   path_find_test_segment_unobstructed(map +0x64, current, surface, waypoint, radius 0.3, flags 1); the first
//   waypoint of the trailing run that cannot be seen, and the edge that blocked it, name the obstacle. With none,
//   the last waypoint is appended and the path is valid. Otherwise the boundary is traced both ways from the edge
//   (path_find_trace_cluster_boundary, radius 0.3, sides 1 and 0), ai_search_choose_shorter_corner picks the corner
//   around it (from the current point, the waypoint before the obstacle and the obstacle waypoint), the tangent
//   points from the current point and from the obstacle waypoint around that corner (radius 0.35, opposite sides)
//   give the portal whose crossing (ai_search_find_circle_portal_crossing, 0.35) becomes the new current point,
//   traced from the old one to find its surface, and is emitted with its height on that surface. A fifth corner,
//   or a failed boundary trace, ends the path unfinished: out_valid = 0.
// blam-cc: stack -> context, count, waypoints, out_count, out_waypoints, out_valid

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"
#include "fn_math.h"
#include <stdint.h> // uintptr_t


extern uint8_t path_find_trace_bsp_boundary(void *map, uint8_t ignore_permission, real_point3d *start,
    int32_t start_surface, real_point3d *end, int32_t target_surface,
    path_find_boundary_crossing *out_result); // 0x43d9b0, stack


void path_find_simplify_waypoints(path_find_context *context, int16_t count, path_find_waypoint *waypoints,
    int16_t *out_count, path_find_waypoint *out_waypoints, uint8_t *out_valid)
{
    path_find_request *request = (path_find_request *)context;
    void *map = (void *)(uintptr_t)context->structure_bsp;
    uint8_t ignore_permission = request->ignores_glass;
    real_point3d current;
    int32_t current_surface;
    int16_t emitted = 0;
    uint8_t valid = 0;
    int16_t first = 1;

    if (count <= 1) {
        *out_count = 1;
        out_waypoints[0] = waypoints[0];
        return;
    }
    current = context->start_position; // only x and y are read
    current_surface = (int32_t)context->start_vertex_id;

    for (;;) {
        int16_t blocked_index = -1;
        int32_t blocked_edge = -1;
        uint8_t blocked = 0;
        int16_t i;

        for (i = first; i < count; i++) {
            path_find_boundary_crossing hit;

            if (path_find_test_segment_unobstructed(map, &current, ignore_permission, current_surface,
                    &waypoints[i].position, waypoints[i].surface_index, 0.3f, 1, &hit) != 0) {
                if (!blocked) {
                    blocked_index = i;
                    blocked_edge = hit.edge_b;
                    blocked = 1;
                }
            } else if (blocked) {
                blocked_edge = -1;
                blocked_index = -1;
                blocked = 0;
            }
        }
        if (!blocked || blocked_edge == -1) {
            // 0x43cec9: everything left is visible
            out_waypoints[emitted++] = waypoints[count - 1];
            valid = 1;
            break;
        }
        {
            real_point2d corner_a;
            real_point2d corner_b;
            real_point2d corner;
            real_point2d tangent[2];
            real_point2d crossing;
            real_point3d previous = current;
            uint8_t side;
            path_find_waypoint *entry;

            if (!path_find_trace_cluster_boundary(map, blocked_edge, (real_point2d *)&current, 0.3f, 1, ignore_permission,
                    &corner_a) ||
                !path_find_trace_cluster_boundary(map, blocked_edge, (real_point2d *)&current, 0.3f, 0, ignore_permission,
                    &corner_b)) {
                break;
            }
            side = ai_search_choose_shorter_corner((real_point2d *)&current, &corner_a,
                (real_point2d *)&waypoints[blocked_index - 1].position, &corner_b,
                (real_point2d *)&waypoints[blocked_index].position, &corner);
            ai_search_find_circle_tangent_point(&corner, (real_point2d *)&current, &tangent[0], 0.35f, side);
            ai_search_find_circle_tangent_point(&corner, (real_point2d *)&waypoints[blocked_index].position, &tangent[1],
                0.35f, (uint8_t)(side == 0));
            ai_search_find_circle_portal_crossing(&corner, tangent, &crossing, (real_point2d *)&current, 0.35f);
            current.x = crossing.x;
            current.y = crossing.y;
            if (current_surface == -1) {
                current_surface = -1;
            } else {
                path_find_boundary_crossing trace;

                path_find_trace_bsp_boundary(map, ignore_permission, &previous, current_surface, &current, -1, &trace);
                current.x = trace.position.x;
                current.y = trace.position.y;
                if (trace.edge_a != -1) {
                    current_surface = trace.edge_a;
                }
            }
            {
                uint8_t *bsp = *(uint8_t **)((uint8_t *)map + 0xb4);
                uint32_t plane = *(uint32_t *)(*(uint8_t **)(bsp + 0x40) + current_surface * 12) & 0x7fffffff;

                entry = &out_waypoints[emitted++];
                decal_plane_solve_third_axis(&entry->position, 1, 2,
                    (real_plane3d *)(*(uint8_t **)(bsp + 0x10) + plane * 16), (real_point2d *)&current);
                entry->surface_index = current_surface;
            }
            if (emitted >= 4) {
                break;
            }
            first = blocked_index;
        }
    }
    *out_count = emitted;
    if (!valid) {
        *out_valid = 0;
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
