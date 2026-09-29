// ai_navigate_around_obstacles  (Ghidra: ai_navigate_around_obstacles, renamed)
// address 0x43be90, size 1196 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// REWRITTEN from objdump 0x43be90..0x43c33b. Stack: path find context, waypoint count, waypoints, out_count,
//   out_waypoints, out_valid. For each waypoint, from the previous point (the request's start +0x14 / surface +0x20
//   at first): the obstacle list and search context come from the context's cache (+0x48: valid +0x10588,
//   count +0x1058a, lists +0x1058c (0xa08 each), searches +0x12dac (0x1534 each)) or the stack. Without valid
//   cached obstacles they are gathered around the previous point (radius 4, toward the waypoint, excluding the
//   request's objects +0x08/+0x0c), the avoid sphere (+0x24: object +0x34, position +0x28, radius +0x38) is added
//   flagged, and the list is grouped with the search radius (max(request radius, 0.2)). The point search starts
//   at the previous point toward the waypoint (target surface = the waypoint's, the goal flag on the last
//   waypoint when the path is still valid); with no result and flagged obstacles it is rerun ignoring them
//   (unknown_2a = 1). No result fails the whole path (returns 0). A complete search continues from the waypoint
//   itself, a best-effort one from its best node (height from its surface); the node chain (up to 0x80, heights
//   from their surface planes) is appended in path order to out_waypoints while it holds fewer than 4. Running
//   out of room clears out_valid and stops (returns 1).
// blam-cc: stack -> context, count, waypoints, out_count, out_waypoints, out_valid

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>
#include "objects.h"
#include "units.h"
#include "fn_ai.h"

extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98
extern ScenarioStructureBSP *global_structure_bsp;                         // 0x00746f9c
extern double sqrt(double x); // FSQRT
extern double fabs(double x); // FABS


extern real_point3d *decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis,
    const real_plane3d *plane, const real_point2d *known); // 0x44d860

static real_plane3d *ai_navigate_surface_plane(ModelCollisionGeometryBSP *bsp, int32_t surface)
{
    uint8_t *raw = (uint8_t *)bsp;
    uint32_t plane = *(uint32_t *)(*(uint8_t **)&((struct ModelCollisionGeometryBSP *)raw)->surfaces.pointer + surface * 12) & 0x7fffffff;

    return (real_plane3d *)(*(uint8_t **)&((struct ModelCollisionGeometryBSP *)raw)->planes.pointer + plane * 16);
}

uint8_t ai_navigate_around_obstacles(path_find_context *context, int16_t count, path_find_waypoint *waypoints,
    int16_t *out_count, path_find_waypoint *out_waypoints, uint8_t *out_valid)
{
    path_find_request *request = (path_find_request *)context;
    ModelCollisionGeometryBSP *collision_bsp = global_structure_collision_bsp;
    float radius = (request->pathfinding_radius > 0.2f) ? request->pathfinding_radius : 0.2f;
    uint8_t *cache = *(uint8_t **)&((struct path_find_context *)context)->unknown_48;
    ai_search_obstacle_list local_obstacles;
    ai_search_context local_search;
    path_find_waypoint path[0x80];
    real_point3d previous;
    int32_t previous_surface = 0;
    int16_t i;

    if (cache != 0 && cache[0x10588] == 0) {
        *(int16_t *)(cache + 0x1058a) = 0;
    }
    for (i = 0; i < count; i++) {
        ai_search_obstacle_list *obstacles = &local_obstacles;
        ai_search_context *search = &local_search;
        uint8_t last = (uint8_t)(i == count - 1 && *out_valid != 0);
        real_point3d *from;
        int32_t from_surface;
        int32_t surface = waypoints[i].surface_index;
        real_point3d *to = &waypoints[i].position;
        real_vector3d direction;
        uint8_t found;
        uint8_t overflow = 0;
        int16_t length = 0;
        int16_t index;

        if (i > 0) {
            from = &previous;
            from_surface = previous_surface;
        } else {
            from = &context->start_position;
            from_surface = (int32_t)context->start_vertex_id;
        }
        direction.i = to->x - from->x;
        direction.j = to->y - from->y;
        direction.k = to->z - from->z;
        {
            float magnitude = (float)sqrt(direction.j * direction.j + direction.k * direction.k +
                direction.i * direction.i);

            if (!((float)fabs(magnitude) < 0.0001f)) {
                float scale = 1.0f / magnitude;

                direction.i *= scale;
                direction.j *= scale;
                direction.k *= scale;
            }
        }
        if (cache != 0) {
            obstacles = (ai_search_obstacle_list *)(cache + 0x1058c + i * 0xa08);
            search = (ai_search_context *)(cache + 0x12dac + i * 0x1534);
        }
        if (cache == 0 || cache[0x10588] == 0 || !(i < *(int16_t *)(cache + 0x1058a))) {
            // 0x43c027: gather this leg's obstacles
            obstacles->unknown_00 = 0;
            obstacles->count = 0;
            obstacles->flagged_count = 0;
            ai_search_gather_obstacles(obstacles, from, 4.0f, &direction, request->unknown_08, request->unknown_0c);
            if (request->have_avoid_sphere && obstacles->count != 0x80) {
                ai_search_obstacle *entry = &obstacles->obstacles[obstacles->count++];

                obstacles->flagged_count++;
                entry->flags = 1;
                entry->link = -1;
                entry->object_index = *(uint32_t *)((uint8_t *)context + 0x34);
                entry->position.x = request->avoid_position.x;
                entry->position.y = request->avoid_position.y;
                entry->radius = request->avoid_radius;
            }
            ai_search_partition_into_groups(obstacles, radius);
            if (cache != 0 && cache[0x10588] == 0) {
                (*(int16_t *)(cache + 0x1058a))++;
            }
        }

        ai_search_context_init(search, request->ignores_glass, *(uint32_t *)&radius, obstacles, (real_point2d *)to,
            (uint32_t)(uintptr_t)global_structure_bsp, (real_point2d *)from, from_surface, (uint32_t)surface, last, 0);
        while (ai_search_step(search) != 0) {
        }
        if (search->result_node != -1) {
            search->complete = 1;
        } else if (search->best_node != -1) {
            search->result_node = search->best_node;
        }
        found = (uint8_t)(search->result_node != -1);
        if (!found) {
            if (obstacles->flagged_count <= 0 ||
                !ai_search_run(search, request->ignores_glass, obstacles, *(uint32_t *)&radius, (real_point2d *)from,
                    from_surface, (real_point2d *)to, (uint32_t)surface, last, 1)) {
                return 0;
            }
        }

        if (search->complete) {
            previous = *to;
            previous_surface = surface;
        } else {
            ai_search_node *best = &search->nodes[search->result_node];

            previous_surface = *(int32_t *)&best->z;
            decal_plane_solve_third_axis(&previous, 1, 2, ai_navigate_surface_plane(collision_bsp, previous_surface),
                &best->position);
        }

        // 0x43c1f9: collect the node chain back to the root
        index = search->result_node;
        while (index != 0) {
            ai_search_node *node = &search->nodes[index];
            real_plane3d *plane = ai_navigate_surface_plane(collision_bsp, *(int32_t *)&node->z);
            path_find_waypoint *point = &path[length++];

            point->surface_index = *(int32_t *)&node->z;
            point->position.x = node->position.x;
            point->position.y = node->position.y;
            if ((float)fabs(plane->normal.k) < 0.0001f) {
                point->position.z = 0.0f;
            } else {
                point->position.z = (plane->d - node->position.x * plane->normal.i - plane->normal.j * node->position.y) /
                    plane->normal.k;
            }
            index = node->parent;
            if (length >= 0x80) {
                overflow = 1;
                break;
            }
        }
        {
            int16_t emitted = *out_count;

            while (--length >= 0) {
                if (emitted >= 4) {
                    overflow = 1;
                    break;
                }
                out_waypoints[emitted++] = path[length];
            }
            *out_count = emitted;
        }
        if (overflow) {
            *out_valid = 0;
            return 1;
        }
    }
    return 1;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043be90 @ 0x43be90) ----
/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4
FUN_0043be90(float *param_1,short param_2,int param_3,short *param_4,int param_5,char *param_6)

{
  undefined2 *puVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  char cVar5;
  float *pfVar6;
  int iVar7;
  int iVar8;
  short sVar9;
  float fVar10;
  undefined1 *puVar11;
  undefined2 *puVar12;
  int iVar13;
  float *pfVar14;
  short sVar15;
  float local_2774;
  float local_2770;
  float local_276c;
  float local_2768;
  float local_2764;
  int local_2760;
  float local_275c;
  uint local_2758;
  float local_2754;
  int local_2750;
  int local_274c;
  float local_2748;
  float local_2744;
  float local_2740;
  float local_273c [512];
  undefined2 local_1f3c [1284];
  undefined1 local_1534 [5424];
  undefined4 uStack_4;

  uStack_4 = 0x43be9a;
  local_2750 = DAT_00746f98;
  if (*param_1 <= 0.2) {
    local_2764 = 0.2;
  }
  else {
    local_2764 = *param_1;
  }
  fVar10 = param_1[0x12];
  if ((fVar10 != 0.0) && (*(char *)((int)fVar10 + 0x10588) == '\0')) {
    *(undefined2 *)((int)fVar10 + 0x1058a) = 0;
  }
  local_2760 = 0;
  if (param_2 < 1) {
    return 1;
  }
  local_274c = param_2 + -1;
  do {
    sVar15 = (short)local_2760;
    iVar7 = (int)sVar15;
    puVar12 = local_1f3c;
    puVar11 = local_1534;
    if (iVar7 == local_274c) {
      local_2758 = CONCAT31(local_2758._1_3_,1);
      if (*param_6 == '\0') goto LAB_0043bf41;
    }
    else {
LAB_0043bf41:
      local_2758 = local_2758 & 0xffffff00;
    }
    if (sVar15 < 1) {
      local_2774 = param_1[8];
      pfVar14 = param_1 + 5;
    }
    else {
      pfVar14 = &local_2748;
    }
    pfVar6 = (float *)(iVar7 * 0x10 + param_3);
    local_2770 = pfVar6[1] - *pfVar14;
    fVar10 = *pfVar6;
    local_276c = pfVar6[2] - pfVar14[1];
    local_2768 = pfVar6[3] - pfVar14[2];
    fVar2 = SQRT(local_2770 * local_2770 + local_2768 * local_2768 + local_276c * local_276c);
    if (0.0001 <= ABS(fVar2)) {
      fVar2 = 1.0 / fVar2;
      local_2770 = local_2770 * fVar2;
      local_276c = local_276c * fVar2;
      local_2768 = local_2768 * fVar2;
    }
    fVar2 = param_1[0x12];
    local_275c = local_2774;
    if (fVar2 == 0.0) {
LAB_0043c027:
      *puVar12 = 0;
      puVar12[1] = 0;
      puVar12[2] = 0;
      ai_search_gather_obstacles(puVar12,pfVar14,0x40800000,&local_2770,param_1[2],param_1[3]);
      if (*(char *)(param_1 + 9) != '\0') {
        local_2754 = param_1[0xd];
        fVar2 = param_1[0xe];
        sVar15 = puVar12[1];
        if (sVar15 != 0x80) {
          puVar12[2] = puVar12[2] + 1;
          puVar12[1] = sVar15 + 1;
          puVar1 = puVar12 + sVar15 * 10 + 4;
          *puVar1 = 1;
          puVar1[1] = 0xffff;
          *(float *)(puVar1 + 2) = local_2754;
          fVar3 = param_1[10];
          *(float *)(puVar1 + 6) = param_1[0xb];
          *(float *)(puVar1 + 4) = fVar3;
          *(float *)(puVar1 + 8) = fVar2;
        }
      }
      FUN_0043cb60(local_2764);
      fVar2 = param_1[0x12];
      if ((fVar2 != 0.0) && (*(char *)((int)fVar2 + 0x10588) == '\0')) {
        *(short *)((int)fVar2 + 0x1058a) = *(short *)((int)fVar2 + 0x1058a) + 1;
      }
    }
    else {
      puVar11 = (undefined1 *)(iVar7 * 0x1534 + 0x12dac + (int)fVar2);
      puVar12 = (undefined2 *)(iVar7 * 0xa08 + 0x1058c + (int)fVar2);
      if ((*(char *)((int)fVar2 + 0x10588) == '\0') || (*(short *)((int)fVar2 + 0x1058a) <= sVar15))
      goto LAB_0043c027;
    }
    FUN_0043b790(puVar11,*(undefined1 *)(param_1 + 1),local_2764,pfVar14,local_275c,fVar10,
                 local_2758,0);
    do {
      cVar5 = FUN_0043bcb0();
    } while (cVar5 != '\0');
    if (*(short *)(puVar11 + 0x1e) == -1) {
      if (*(short *)(puVar11 + 0x20) != -1) {
        *(short *)(puVar11 + 0x1e) = *(short *)(puVar11 + 0x20);
      }
    }
    else {
      puVar11[0x28] = 1;
    }
    if ((*(short *)(puVar11 + 0x1e) == -1) &&
       (((short)puVar12[2] < 1 ||
        (cVar5 = FUN_0043be20(*(undefined1 *)(param_1 + 1),puVar12,local_2764,pfVar14,local_275c,
                              pfVar6 + 1), cVar5 == '\0')))) {
      return 0;
    }
    bVar4 = false;
    local_2754 = 0.0;
    if (puVar11[0x28] == '\0') {
      local_2774 = *(float *)(puVar11 + *(short *)(puVar11 + 0x1e) * 0x28 + 0x38);
      FUN_0044d860(&local_2748);
    }
    else {
      local_2748 = pfVar6[1];
      local_2744 = pfVar6[2];
      local_2740 = pfVar6[3];
      local_2774 = fVar10;
    }
    sVar15 = *(short *)(puVar11 + 0x1e);
    fVar10 = local_2754;
    do {
      if (sVar15 == 0) goto LAB_0043c292;
      pfVar14 = (float *)(puVar11 + sVar15 * 0x28 + 0x30);
      fVar2 = pfVar14[2];
      iVar13 = (int)SUB42(fVar10,0);
      local_273c[iVar13 * 4] = fVar2;
      iVar7 = *(int *)(*(int *)(local_2750 + 0x40) + (int)fVar2 * 0xc);
      iVar8 = *(int *)(local_2750 + 0x10);
      local_273c[iVar13 * 4 + 1] = *pfVar14;
      pfVar6 = (float *)(iVar7 * 0x10 + iVar8);
      local_273c[iVar13 * 4 + 2] = pfVar14[1];
      fVar10 = (float)((int)fVar10 + 1);
      if (0.0001 <= ABS(pfVar6[2])) {
        fVar2 = ((pfVar6[3] - *pfVar14 * *pfVar6) - pfVar6[1] * pfVar14[1]) / pfVar6[2];
      }
      else {
        fVar2 = 0.0;
      }
      local_273c[iVar13 * 4 + 3] = fVar2;
      sVar15 = *(short *)(pfVar14 + 9);
    } while (SUB42(fVar10,0) < 0x80);
    bVar4 = true;
LAB_0043c292:
    sVar15 = *param_4;
    iVar7 = (int)fVar10 + -1;
    sVar9 = (short)iVar7;
    while (-1 < sVar9) {
      if (3 < sVar15) {
        bVar4 = true;
        break;
      }
      iVar8 = (int)(short)iVar7;
      pfVar14 = (float *)(sVar15 * 0x10 + param_5);
      *pfVar14 = local_273c[iVar8 * 4];
      pfVar14[1] = local_273c[iVar8 * 4 + 1];
      fVar10 = local_273c[iVar8 * 4 + 3];
      sVar15 = sVar15 + 1;
      iVar7 = iVar7 + -1;
      pfVar14[2] = local_273c[iVar8 * 4 + 2];
      pfVar14[3] = fVar10;
      sVar9 = (short)iVar7;
    }
    *param_4 = sVar15;
    if (bVar4) {
      *param_6 = '\0';
      return 1;
    }
    local_2760 = local_2760 + 1;
    if (param_2 <= (short)local_2760) {
      return 1;
    }
  } while( true );
}
#endif
