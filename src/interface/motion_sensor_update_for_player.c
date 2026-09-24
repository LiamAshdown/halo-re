// motion_sensor_update_for_player  (Ghidra: motion_sensor_update_for_player, already named)
// address 0x4b3e10, size 773 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4b3e10..0x4b4114 in the phase-4 review. Needs
// motion_sensor_globals::enabled and a unit. Refreshes the current frame of one local player:
// the viewer position is the unit camera position; each tracked object that still exists is
// either dropped (type 6, tracked -1) when it is no longer detected or outside
// HUDGlobals motion_sensor_range in x and y, or gets its blip x and y as
// ftol(delta / range * 127) (type and subtype stay as motion_sensor_update filled them);
// the facing is the local player yaw plus pi/2. The custom waypoints matching the player
// (game_engine_collect_matching_waypoints 0x462190, at most 16) become the extra blips,
// compacted to the ones inside the range.
// Behaviour kept from the binary: a tracked object that no longer exists keeps its blip; the
// extra blip sources are not compacted with the positions.
// register convention: plain cdecl, one stack argument (a short).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"

extern motion_sensor_globals *motion_sensor;        // 0x00719438
extern player_globals *local_player_globals;        // 0x0087a478
extern data_array *player_data;                     // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern player_control_globals *player_control_globals_ptr; // 0x006b145c

extern int32_t __ftol(double x); // 0x006391b4, MSVC 7.1 CRT float-to-int truncation
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern void unit_get_camera_position(datum_index unit_index, real_point3d *out); // 0x568f80, blam-cc: ECX unit_index, EDI out
extern uint8_t motion_sensor_object_is_detected(datum_index unit_index); // 0x4b36a0
extern int16_t game_engine_collect_matching_waypoints(int32_t candidate, float *out_positions, uint8_t *out_slots,
                                                      int32_t max_count); // 0x462190

void motion_sensor_update_for_player(int16_t local_player_index)
{
    motion_sensor_player_state *state = &motion_sensor->players[local_player_index];
    motion_sensor_frame *frame;
    datum_index player_index;
    datum_index unit_index;
    real_point3d camera;
    float waypoints[0x10][2];
    float range = *(float *)((uint8_t *)hud_globals_tag_data + 0x2d0);
    uint8_t removed;
    int32_t i;

    if (motion_sensor->enabled == 0) {
        return;
    }
    frame = &state->history[motion_sensor->frame_index];
    unit_index = (datum_index)-1;
    if (local_player_index != -1 && local_player_index < 1 &&
        local_player_globals->local_players[local_player_index] != (datum_index)-1) {
        unit_index = ((player *)((uint8_t *)player_data->data +
                                 (local_player_globals->local_players[local_player_index] & 0xffff) * 0x200))->unit;
    }
    if (unit_index == (datum_index)-1) {
        return;
    }
    unit_get_camera_position(unit_index, &camera);
    frame->viewer_x = camera.x;
    frame->viewer_y = camera.y;

    for (i = 0; i < 0x10; i++) {
        datum_index tracked = state->tracked_objects[i];
        uint8_t detected;
        real_point3d position;
        float dx;
        float dy;

        if (object_try_and_get(tracked, 3) == 0) {
            continue;
        }
        detected = motion_sensor_object_is_detected(tracked);
        position = *(real_point3d *)((uint8_t *)((object_header *)object_data->data)[tracked & 0xffff].data + 0xa0);
        dx = position.x - frame->viewer_x;
        dy = position.y - frame->viewer_y;
        if (detected != 0 && !(range * range < dy * dy + dx * dx)) {
            frame->blips[i].x = (int8_t)__ftol((double)(dx / range * 127.0f));
            frame->blips[i].y = (int8_t)__ftol((double)(dy / range * 127.0f));
        } else {
            frame->blips[i].type = _blip_type_empty;
            state->tracked_objects[i] = (datum_index)-1;
        }
    }

    frame->viewer_facing = player_control_globals_ptr->local_players[local_player_index].yaw + 1.5707963705062866f;
    player_index = (local_player_index != -1 && local_player_index < 1)
                       ? local_player_globals->local_players[local_player_index] : (datum_index)-1;
    frame->extra_blip_count = (uint8_t)game_engine_collect_matching_waypoints((int32_t)player_index, &waypoints[0][0],
                                                                               frame->extra_sources, 0x10);
    unit_get_camera_position(unit_index, &camera);
    removed = 0;
    for (i = 0; i < frame->extra_blip_count; i++) {
        float dx;
        float dy;

        waypoints[i][0] = waypoints[i][0] - camera.x;
        waypoints[i][1] = waypoints[i][1] - camera.y;
        dx = waypoints[i][0];
        dy = waypoints[i][1];
        if (range * range < dx * dx + dy * dy) {
            removed++;
            continue;
        }
        frame->extra_blips[(i - removed) * 2] = (int8_t)__ftol((double)(waypoints[i][0] / range * 127.0f));
        frame->extra_blips[(i - removed) * 2 + 1] = (int8_t)__ftol((double)(waypoints[i][1] / range * 127.0f));
    }
    frame->extra_blip_count = (uint8_t)(frame->extra_blip_count - removed);
}

#if 0
Original Ghidra decompilation (0x4b3e10):

void motion_sensor_update_for_player(short param_1)

{
  float fVar1;
  float fVar2;
  uint uVar3;
  char cVar4;
  undefined1 uVar5;
  undefined4 uVar6;
  int iVar7;
  int iVar8;
  undefined1 *puVar9;
  uint *puVar10;
  int iVar11;
  int local_ac;
  float local_a8;
  float local_a4;
  float local_80 [32];

  iVar8 = param_1 * 0x568 + DAT_00719438;
  if (*(char *)(DAT_00719438 + 0x56e) != '\0') {
    if (((param_1 == -1) || (0 < param_1)) ||
       (uVar3 = *(uint *)(DAT_0087a478 + 4 + param_1 * 4), uVar3 == 0xffffffff)) {
      iVar7 = -1;
    }
    else {
      iVar7 = *(int *)((uVar3 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
    }
    iVar11 = *(short *)(DAT_00719438 + 0x56c) * 0x84 + iVar8;
    if (iVar7 != -1) {
      unit_get_camera_position();
      puVar10 = (uint *)(iVar8 + 0x528);
      *(float *)(iVar11 + 0x70) = local_a8;
      *(float *)(iVar11 + 0x74) = local_a4;
      puVar9 = (undefined1 *)(iVar11 + 2);
      local_ac = 0x10;
      do {
        uVar3 = *puVar10;
        iVar8 = object_try_and_get(3);
        if (iVar8 != 0) {
          cVar4 = motion_sensor_object_is_detected(uVar3);
          iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
          local_a8 = *(float *)(iVar8 + 0xa0);
          local_a4 = *(float *)(iVar8 + 0xa4);
          if ((cVar4 == '\0') ||
             (fVar1 = local_a8 - *(float *)(iVar11 + 0x70),
             fVar2 = local_a4 - *(float *)(iVar11 + 0x74),
             *(float *)(DAT_0071941c + 0x2d0) * *(float *)(DAT_0071941c + 0x2d0) <
             fVar1 * fVar1 + fVar2 * fVar2 + 0.0)) {
            *puVar9 = 6;
            *puVar10 = 0xffffffff;
          }
          else {
            uVar5 = __ftol();
            puVar9[-2] = uVar5;
            uVar5 = __ftol();
            puVar9[-1] = uVar5;
          }
        }
        puVar9 = puVar9 + 4;
        puVar10 = puVar10 + 1;
        local_ac = local_ac + -1;
      } while (local_ac != 0);
      local_ac = 0;
      *(float *)(iVar11 + 0x7c) = *(float *)(param_1 * 0x40 + 0x1c + DAT_006b145c) + 1.5707964;
      if ((param_1 == -1) || (0 < param_1)) {
        uVar6 = 0xffffffff;
      }
      else {
        uVar6 = *(undefined4 *)(DAT_0087a478 + 4 + param_1 * 4);
      }
      uVar5 = FUN_00462190(uVar6,local_80,iVar11 + 0x60,0x10);
      *(undefined1 *)(iVar11 + 0x80) = uVar5;
      unit_get_camera_position();
      iVar8 = DAT_0071941c;
      iVar7 = 0;
      if (*(char *)(iVar11 + 0x80) != '\0') {
        do {
          fVar1 = local_80[iVar7 * 2];
          fVar2 = local_80[iVar7 * 2 + 1];
          local_80[iVar7 * 2] = local_80[iVar7 * 2] - local_a8;
          local_80[iVar7 * 2 + 1] = local_80[iVar7 * 2 + 1] - local_a4;
          if (*(float *)(iVar8 + 0x2d0) * *(float *)(iVar8 + 0x2d0) <
              (fVar1 - local_a8) * (fVar1 - local_a8) + (fVar2 - local_a4) * (fVar2 - local_a4)) {
            local_ac = local_ac + 1;
          }
          else {
            puVar9 = (undefined1 *)(iVar11 + 0x40 + (iVar7 - local_ac) * 2);
            uVar5 = __ftol();
            *puVar9 = uVar5;
            uVar5 = __ftol();
            puVar9[1] = uVar5;
          }
          iVar7 = iVar7 + 1;
        } while (iVar7 < (int)(uint)*(byte *)(iVar11 + 0x80));
      }
      *(char *)(iVar11 + 0x80) = *(char *)(iVar11 + 0x80) - (char)local_ac;
    }
  }
  return;
}
#endif
