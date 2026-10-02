// chimera__motion_sensor_update  (Ghidra: chimera__motion_sensor_update, already named; the
// Chimera signature motion_sensor_update_sig matches at the entry)
// address 0x4b3920, size 1238 bytes (0x4b3920..0x4b396f computes the sweep and tail-jumps to
// 0x4b3980, the update proper, to 0x4b3e01)
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: rewritten from objdump 0x4b3920..0x4b3e01 in the phase-4 review (the first rewrite
// was a 0.3 literal copy). The sweep value at 0x0071943c is 1 / ((t + 1/16) * 1.1) for
// t = fmod(game time / 30, 2.1) below 2.0375, else 0.4 (handed to the rasterizer by
// motion_sensor_render). Every tick the ring slot advances modulo 10 and the update time is
// stamped. On 14 of every 15 ticks (and never at time 0) each local player just copies the
// previous frame into the new slot. Otherwise the new frame is rebuilt: all 16 blips emptied
// and the blip count zeroed per local player, then every visible biped or vehicle (object
// iterator type mask 3, flags mask 1; not hidden, object +0x106 bit 2) that
// motion_sensor_object_is_detected accepts is added, per local player with a unit, while that
// player has fewer than 16 blips and, outside multiplayer, the object is within HUDGlobals
// motion_sensor_range of the player camera in x and y (the object z is replaced by the camera
// z). The walk stops once every local player is full.
// register convention: none.
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_time_globals *game_time;               // 0x006f1d6c
extern motion_sensor_globals *motion_sensor;       // 0x00719438
extern player_globals *local_player_globals;       // 0x0087a478
extern data_array *player_data;                    // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern float motion_sensor_sweep;                  // 0x0071943c
extern float motion_sensor_sweep_scale;            // 0x00692fe4, 1.1

extern double fmod(double x, double y); // 0x628cca, MSVC 7.1 CRT _CIfmod
extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20
extern void unit_get_camera_position(datum_index unit_index, real_point3d *out); // 0x568f80, blam-cc: ECX unit_index, EDI out
extern uint8_t motion_sensor_object_is_detected(datum_index unit_index); // 0x4b36a0
extern void motion_sensor_blip_fill(int16_t local_player_index, datum_index object_index,
                                    motion_sensor_blip *blip); // 0x4b35f0, blam-cc: EAX, ECX, ESI

static int16_t motion_sensor_next_local_player(int16_t local_player_index)
{
    return (local_player_globals->local_players[0] != (datum_index)-1 && local_player_index < 0) ? 0 : -1;
}

void chimera__motion_sensor_update(void)
{
    float t;
    int32_t now;
    int16_t frame_index;
    int16_t player_count;

    t = (float)fmod((double)((float)game_time->game_time * 0.03333333507180214f), 2.0999999046325684);
    if (t < 2.0374999046325684f) {
        motion_sensor_sweep = 1.0f / ((t + 0.0625f) * motion_sensor_sweep_scale);
    } else {
        motion_sensor_sweep = 0.4f;
    }

    now = game_time->game_time;
    frame_index = (int16_t)((int16_t)(motion_sensor->frame_index + 1) % 10);
    motion_sensor->enabled = 1;
    motion_sensor->update_time = now;
    motion_sensor->frame_index = frame_index;

    if (now % 15 != 0 && now != 0) {
        int16_t previous = (int16_t)((frame_index + 9) % 10);
        int16_t local_player_index = local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1;

        for (player_count = local_player_globals->local_player_count; player_count > 0; player_count--) {
            motion_sensor->players[local_player_index].history[frame_index] =
                motion_sensor->players[local_player_index].history[previous];
            local_player_index = motion_sensor_next_local_player(local_player_index);
        }
        return;
    }

    {
        int16_t local_players[1 + 1];     // esp+0x14, one word per local player walked
        int16_t blip_counts[2];           // esp+0x18, indexed by local player index
        real_point3d cameras[2];          // esp+0x3c, indexed by local player index
        struct {
            object_iterator iterator;
            uint32_t signature;           // 0x86868686
        } walk;
        uint8_t all_full = 0;
        int16_t local_player_index = local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1;
        int16_t count = local_player_globals->local_player_count;
        int16_t k;

        blip_counts[0] = 0;
        for (k = 0; k < count; k++) {
            motion_sensor_frame *frame = &motion_sensor->players[local_player_index].history[motion_sensor->frame_index];
            datum_index unit_index = (datum_index)-1;
            int32_t i;

            if (local_player_index != -1 && local_player_index < 1 &&
                local_player_globals->local_players[local_player_index] != (datum_index)-1) {
                unit_index = ((player *)((uint8_t *)player_data->data +
                                         (local_player_globals->local_players[local_player_index] & 0xffff) * 0x200))->unit;
            }
            local_players[k] = local_player_index;
            cameras[local_player_index].x = 0.0f;
            cameras[local_player_index].y = 0.0f;
            cameras[local_player_index].z = 0.0f;
            if (unit_index != (datum_index)-1) {
                unit_get_camera_position(unit_index, &cameras[local_player_index]);
            }
            frame->blip_count = 0;
            for (i = 0; i < 0x10; i++) {
                frame->blips[i].type = _blip_type_empty;
            }
            local_player_index = (local_player_globals->local_players[0] != (datum_index)-1 &&
                                  local_player_index < count) ? 0 : -1;
        }

        walk.iterator.type_mask = 3;          // bipeds and vehicles
        walk.iterator.flags_mask = 1;
        walk.iterator.index = 0;
        walk.iterator.handle = (datum_index)-1;
        walk.signature = 0x86868686;
        while (object_iterator_next(&walk.iterator) != 0 && !all_full) {
            datum_index object_index = walk.iterator.handle;
            uint8_t *header = 0;
            int16_t full_players;

            if (object_index != (datum_index)-1 && (int16_t)object_index >= 0 &&
                (int16_t)object_index < object_data->maximum_count) {
                uint8_t *candidate = (uint8_t *)object_data->data + (int16_t)object_index * object_data->size;
                int16_t salt = (int16_t)((uint32_t)object_index >> 16);
                if (*(int16_t *)candidate != 0 && (salt == 0 || *(int16_t *)candidate == salt)) {
                    header = candidate;
                }
            }
            if (header == 0 || ((1u << (header[3] & 0x1f)) & 3) == 0 || *(uint8_t **)(header + 8) == 0 ||
                ((*(uint8_t **)(header + 8))[0x106] & 4) != 0 || motion_sensor_object_is_detected(object_index) == 0) {
                continue;
            }
            {
                real_point3d position = *(real_point3d *)((uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data + 0xa0);

                full_players = 0;
                for (k = 0; k < count; k++) {
                    int16_t index = local_players[k];
                    datum_index player_index;
                    motion_sensor_player_state *state;
                    motion_sensor_frame *frame;

                    if (index == -1 || index >= 1) {
                        continue;
                    }
                    player_index = local_player_globals->local_players[index];
                    if (player_index == (datum_index)-1 ||
                        ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200))->unit == (datum_index)-1) {
                        continue;
                    }
                    if (blip_counts[index] >= 0x10) {
                        full_players++;
                        continue;
                    }
                    position.z = cameras[index].z;
                    if (current_game_engine == 0) {
                        float dx = position.x - cameras[index].x;
                        float dy = position.y - cameras[index].y;
                        float dz = position.z - cameras[index].z;
                        float range = *(float *)((uint8_t *)hud_globals_tag_data + 0x2d0);
                        if (range * range < dz * dz + dy * dy + dx * dx) {
                            continue;
                        }
                    }
                    state = &motion_sensor->players[index];
                    frame = &state->history[motion_sensor->frame_index];
                    motion_sensor_blip_fill(index, object_index, &frame->blips[blip_counts[index]]);
                    state->tracked_objects[blip_counts[index]] = object_index;
                    blip_counts[index]++;
                    frame->blip_count++;
                }
                if (full_players == count) {
                    all_full = 1;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4b3920):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void chimera__motion_sensor_update(void)

{
  int *piVar1;
  ushort uVar2;
  short sVar3;
  uint uVar4;
  float fVar5;
  bool bVar6;
  char cVar7;
  undefined1 *puVar8;
  short *psVar9;
  short sVar10;
  int iVar11;
  int iVar12;
  uint uVar13;
  short sVar14;
  undefined4 *puVar15;
  int iVar16;
  short *psVar17;
  undefined4 *puVar18;
  float10 fVar19;
  float afStackY_6001c [98288];
  short local_44 [2];
  short local_40 [2];
  short *local_3c;
  uint local_38;
  short *local_34;
  uint local_30;
  uint local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c [4];
  undefined1 local_c;
  undefined2 local_a;
  uint local_8;
  undefined4 local_4;

  local_4 = 0x4b3939;
  fVar19 = (float10)FUN_00628cca();
  iVar16 = DAT_00719438;
  if ((float10)2.0375 <= fVar19) {
    DAT_0071943c = 0.4;
  }
  else {
    DAT_0071943c = (float)((float10)1.0 / ((fVar19 + (float10)0.0625) * (float10)_DAT_00692fe4));
  }
  iVar11 = *(int *)(DAT_006f1d6c + 0xc);
  sVar14 = (short)(*(short *)(DAT_00719438 + 0x56c) + 1) % 10;
  bVar6 = false;
  *(undefined1 *)(DAT_00719438 + 0x56e) = 1;
  *(int *)(iVar16 + 0x568) = iVar11;
  *(short *)(iVar16 + 0x56c) = sVar14;
  if ((iVar11 % 0xf == 0) || (iVar11 == 0)) {
    uVar2 = *(ushort *)(DAT_0087a478 + 0xc);
    local_30 = (uint)uVar2;
    sVar14 = -1;
    local_40[0] = 0;
    if (*(int *)(DAT_0087a478 + 4) != -1) {
      sVar14 = 0;
    }
    if (0 < (short)uVar2) {
      local_38 = (uint)uVar2;
      local_3c = local_44;
      do {
        iVar11 = (int)sVar14;
        iVar16 = iVar11 * 0x568 + *(short *)(iVar16 + 0x56c) * 0x84 + iVar16;
        if (((sVar14 == -1) || (0 < sVar14)) ||
           (uVar13 = *(uint *)(DAT_0087a478 + 4 + iVar11 * 4), uVar13 == 0xffffffff)) {
          iVar12 = -1;
        }
        else {
          iVar12 = *(int *)((uVar13 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
        }
        *local_3c = sVar14;
        local_1c[iVar11 * 3] = 0.0;
        local_1c[iVar11 * 3 + 1] = 0.0;
        local_1c[iVar11 * 3 + 2] = 0.0;
        if (iVar12 != -1) {
          unit_get_camera_position();
        }
        *(undefined4 *)(iVar16 + 0x78) = 0;
        puVar8 = (undefined1 *)(iVar16 + 2);
        iVar16 = 0x10;
        do {
          *puVar8 = 6;
          puVar8 = puVar8 + 4;
          iVar16 = iVar16 + -1;
        } while (iVar16 != 0);
        local_3c = local_3c + 1;
        sVar10 = -1;
        if ((*(int *)(DAT_0087a478 + 4) != -1) && (sVar14 < 0)) {
          sVar10 = 0;
        }
        sVar14 = sVar10;
        local_38 = local_38 - 1;
        iVar16 = DAT_00719438;
      } while (local_38 != 0);
      local_38 = 0;
    }
    uVar13 = local_30;
    local_4 = 0x86868686;
    local_1c[3] = 4.2039e-45;
    local_c = 1;
    local_a = 0;
    local_8 = 0xffffffff;
    iVar11 = object_iterator_next(local_1c + 3);
    uVar4 = local_8;
    while ((iVar11 != 0 && (!bVar6))) {
      psVar17 = (short *)0x0;
      if ((uVar4 != 0xffffffff) &&
         ((sVar14 = (short)uVar4, -1 < sVar14 && (sVar14 < *(short *)(DAT_008603b0 + 0x20))))) {
        psVar9 = (short *)((int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar14 +
                          *(int *)(DAT_008603b0 + 0x34));
        sVar14 = *psVar9;
        if ((sVar14 != 0) && ((sVar10 = (short)(uVar4 >> 0x10), sVar10 == 0 || (sVar14 == sVar10))))
        {
          psVar17 = psVar9;
        }
      }
      bVar6 = false;
      local_8 = uVar4;
      if ((((psVar17 != (short *)0x0) && ((1 << (*(byte *)((int)psVar17 + 3) & 0x1f) & 3U) != 0)) &&
          (*(int *)(psVar17 + 4) != 0)) &&
         (((*(byte *)(*(int *)(psVar17 + 4) + 0x106) & 4) == 0 &&
          (cVar7 = motion_sensor_object_is_detected(uVar4), cVar7 != '\0')))) {
        iVar11 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
        local_38 = 0;
        local_28 = *(float *)(iVar11 + 0xa0);
        local_24 = *(float *)(iVar11 + 0xa4);
        local_20 = *(float *)(iVar11 + 0xa8);
        if (0 < (short)uVar13) {
          local_3c = local_44;
          local_2c = uVar13 & 0xffff;
          do {
            sVar14 = *local_3c;
            if ((sVar14 != -1) && (sVar14 < 1)) {
              iVar11 = (int)sVar14;
              uVar4 = *(uint *)(DAT_0087a478 + 4 + iVar11 * 4);
              if ((uVar4 != 0xffffffff) &&
                 (*(int *)((uVar4 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)) != -1))
              {
                sVar14 = local_40[iVar11];
                local_34 = local_40 + iVar11;
                if (sVar14 < 0x10) {
                  local_20 = local_1c[iVar11 * 3 + 2];
                  if ((DAT_006f1d20 != 0) ||
                     (fVar5 = local_1c[iVar11 * 3 + 2] - local_1c[iVar11 * 3 + 2],
                     (local_28 - local_1c[iVar11 * 3]) * (local_28 - local_1c[iVar11 * 3]) +
                     (local_24 - local_1c[iVar11 * 3 + 1]) * (local_24 - local_1c[iVar11 * 3 + 1]) +
                     fVar5 * fVar5 <=
                     *(float *)(DAT_0071941c + 0x2d0) * *(float *)(DAT_0071941c + 0x2d0))) {
                    iVar11 = iVar11 * 0x568 + iVar16;
                    sVar10 = *(short *)(iVar16 + 0x56c);
                    FUN_004b35f0();
                    uVar13 = local_30;
                    sVar3 = *local_34;
                    *(uint *)(iVar11 + 0x528 + sVar14 * 4) = local_8;
                    *local_34 = sVar3 + 1;
                    piVar1 = (int *)(sVar10 * 0x84 + iVar11 + 0x78);
                    *piVar1 = *piVar1 + 1;
                    iVar16 = DAT_00719438;
                  }
                }
                else {
                  local_38 = local_38 + 1;
                }
              }
            }
            local_2c = local_2c - 1;
            local_3c = local_3c + 1;
          } while (local_2c != 0);
        }
        if (local_38 == (int)(short)uVar13) {
          bVar6 = true;
        }
      }
      iVar11 = object_iterator_next(local_1c + 3);
      uVar4 = local_8;
    }
  }
  else {
    sVar10 = -1;
    if (*(int *)(DAT_0087a478 + 4) != -1) {
      sVar10 = 0;
    }
    if (0 < (short)*(ushort *)(DAT_0087a478 + 0xc)) {
      local_38 = (uint)*(ushort *)(DAT_0087a478 + 0xc);
      do {
        puVar15 = (undefined4 *)((short)((sVar14 + 9) % 10) * 0x84 + sVar10 * 0x568 + iVar16);
        puVar18 = (undefined4 *)(sVar14 * 0x84 + sVar10 * 0x568 + DAT_00719438);
        for (iVar11 = 0x21; iVar11 != 0; iVar11 = iVar11 + -1) {
          *puVar18 = *puVar15;
          puVar15 = puVar15 + 1;
          puVar18 = puVar18 + 1;
        }
        sVar3 = -1;
        if ((*(int *)(DAT_0087a478 + 4) != -1) && (sVar10 < 0)) {
          sVar3 = 0;
        }
        sVar10 = sVar3;
        local_38 = local_38 - 1;
        iVar16 = DAT_00719438;
      } while (local_38 != 0);
      return;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
