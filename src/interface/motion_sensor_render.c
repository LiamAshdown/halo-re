// motion_sensor_render  (Ghidra: motion_sensor_render, already named)
// address 0x4b4120, size 663 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4b4120..0x4b43b9 in the phase-4 review (the first rewrite
// was a 0.25 literal copy with the blip call unresolved). Skipped for camera types 2 and 3.
// Publishes the local player, the icon scale (0.75 in split screen, else 1.0) and the sensor
// center (EAX, int16 x and y) to the rasterizer globals 0x00873d32/0x00692fd0/0x00873d38,
// begins the blip batch (0x52b690), then draws the ten history frames oldest first: frame
// (current - k + 10) % 10 for k = 0..9 gets age a = 0.1 * (10 - k), alpha a * a and size
// pulse (1 - a) ^ 3.5 * 7 + 1. Every non-empty blip (in multiplayer with game variant flags
// bit 6, blips of types 2 and 4 are hidden) and every extra blip whose custom waypoint is
// still active (custom_waypoints[source] +0x0c, type 5, subtype 0) is plotted through
// motion_sensor_plot_blip at (x, y) * range / 127 world units, with pixels_per_unit =
// HUDGlobals motion_sensor_scale (+0x2d8) / motion_sensor_range (+0x2d0). Ends with the sweep
// draw 0x52bc40 (EBX the center globals, stack the sweep 0x0071943c).
// register convention: EAX screen center, CX local player index; one stack argument.
//   // blam-cc: screen_center -> EAX, local_player_index -> CX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern motion_sensor_globals *motion_sensor;   // 0x00719438
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;       // 0x006f1c88 (flags +0x38)
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints]; // 0x006f1888
extern int16_t motion_sensor_render_local_player; // 0x00873d32
extern float motion_sensor_render_icon_scale;  // 0x00692fd0
extern float motion_sensor_render_center[2];   // 0x00873d38
extern float motion_sensor_sweep;              // 0x0071943c

extern double pow(double base, double exponent); // 0x6283c0, MSVC 7.1 CRT _CIpow
extern int16_t camera_get_type_for_player(int16_t local_player_index); // 0x445ac0, blam-cc: ECX local_player_index (UNSURE)
extern void rasterizer_motion_sensor_begin(void); // 0x52b690, rasterizer motion sensor blip batch begin
extern void rasterizer_motion_sensor_end(const float *center, float sweep); // 0x52bc40, rasterizer motion sensor sweep, blam-cc: EBX center
extern void motion_sensor_plot_blip(const float *position, uint8_t type, const motion_sensor_frame *frame, int8_t subtype,
                                    float pixels_per_unit, float alpha, float size_factor); // 0x4b37a0, blam-cc: EAX position, BL type

// blam-cc: screen_center -> EAX, local_player_index -> CX
void motion_sensor_render(uint8_t splitscreen, const int16_t *screen_center, int16_t local_player_index)
{
    float pixels_per_unit;
    float range;
    int32_t k;

    {
        int16_t camera_type = camera_get_type_for_player(local_player_index);
        if (camera_type == 3 || camera_type == 2) {
            return;
        }
    }
    range = *(float *)((uint8_t *)hud_globals_tag_data + 0x2d0);
    pixels_per_unit = *(float *)((uint8_t *)hud_globals_tag_data + 0x2d8) / range;
    motion_sensor_render_local_player = local_player_index;
    motion_sensor_render_icon_scale = 0.75f;
    if (splitscreen == 0) {
        motion_sensor_render_icon_scale = 1.0f;
    }
    motion_sensor_render_center[0] = (float)screen_center[0];
    motion_sensor_render_center[1] = (float)screen_center[1];
    rasterizer_motion_sensor_begin();

    for (k = 0; k < 10; k++) {
        motion_sensor_player_state *state = &motion_sensor->players[local_player_index];
        motion_sensor_frame *frame = &state->history[(int16_t)((motion_sensor->frame_index - k + 10) % 10)];
        float age = (float)(10 - k) * 0.1f;
        float alpha = age * age;
        float size = (float)(pow((double)(1.0f - age), 3.5) * 7.0 + 1.0);
        int32_t i;

        for (i = 0; i < 0x10; i++) {
            motion_sensor_blip *blip = &frame->blips[i];
            float position[2];

            if (blip->type == _blip_type_empty) {
                continue;
            }
            if (current_game_engine != 0 && (*(uint8_t *)&game_engine_variant.flags & 0x40) != 0 &&
                (blip->type == 2 || blip->type == 4)) {
                continue;
            }
            position[0] = (float)blip->x * range * 0.007874015718698502f;
            position[1] = (float)blip->y * range * 0.007874015718698502f;
            motion_sensor_plot_blip(position, blip->type, frame, (int8_t)blip->subtype, pixels_per_unit, alpha, size);
        }
        for (i = 0; (int16_t)i < (int32_t)frame->extra_blip_count; i++) {
            float position[2];

            if (custom_waypoints[(int8_t)frame->extra_sources[i]].active == 0) {
                continue;
            }
            position[0] = (float)frame->extra_blips[i * 2] * range * 0.007874015718698502f;
            position[1] = (float)frame->extra_blips[i * 2 + 1] * range * 0.007874015718698502f;
            motion_sensor_plot_blip(position, 5, frame, 0, pixels_per_unit, alpha, size);
        }
    }
    rasterizer_motion_sensor_end(motion_sensor_render_center, motion_sensor_sweep);
}

#if 0
Original Ghidra decompilation (0x4b4120):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void motion_sensor_render(char param_1)

{
  float fVar1;
  char cVar2;
  float fVar3;
  float fVar4;
  short sVar5;
  short *in_EAX;
  int iVar6;
  short in_CX;
  int iVar7;
  int iVar8;
  char *pcVar9;
  float10 fVar10;
  int local_30;
  int local_2c;
  int local_28;
  int local_18;

  sVar5 = camera_get_type_for_player();
  if ((sVar5 != 3) && (sVar5 != 2)) {
    fVar3 = *(float *)(DAT_0071941c + 0x2d8) / *(float *)(DAT_0071941c + 0x2d0);
    _DAT_00692fd0 = 0x3f400000;
    _DAT_00873d38 = (float)(int)*in_EAX;
    iVar8 = in_CX * 0x568 + DAT_00719438;
    _DAT_00873d3c = (float)(int)in_EAX[1];
    if (param_1 == '\0') {
      _DAT_00692fd0 = 0x3f800000;
    }
    _DAT_00873d32 = in_CX;
    FUN_0052b690();
    local_30 = 0;
    local_2c = 10;
    local_18 = 10;
    do {
      fVar4 = (float)local_2c * 0.1 * (float)local_2c * 0.1;
      iVar7 = (short)(((*(short *)(DAT_00719438 + 0x56c) - local_30) + 10) % 10) * 0x84 + iVar8;
      fVar10 = (float10)FUN_006283c0();
      pcVar9 = (char *)(iVar7 + 2);
      local_28 = 0x10;
      fVar1 = (float)(fVar10 * (float10)7.0 + (float10)1.0);
      do {
        cVar2 = *pcVar9;
        if ((cVar2 != '\x06') &&
           (((DAT_006f1d20 == 0 || (((byte)DAT_006f1cc0 & 0x40) == 0)) ||
            ((cVar2 != '\x02' && (cVar2 != '\x04')))))) {
          FUN_004b37a0(iVar7,pcVar9[1],fVar3,fVar4,fVar1);
        }
        pcVar9 = pcVar9 + 4;
        local_28 = local_28 + -1;
      } while (local_28 != 0);
      sVar5 = 0;
      if (*(char *)(iVar7 + 0x80) != '\0') {
        iVar6 = 0;
        do {
          if (*(char *)(&DAT_006f1894 + *(char *)(iVar6 + 0x60 + iVar7) * 8) != '\0') {
            FUN_004b37a0(iVar7,0,fVar3,fVar4,fVar1);
          }
          sVar5 = sVar5 + 1;
          iVar6 = (int)sVar5;
        } while (iVar6 < (int)(uint)*(byte *)(iVar7 + 0x80));
      }
      local_30 = local_30 + 1;
      local_2c = local_2c + -1;
      local_18 = local_18 + -1;
    } while (local_18 != 0);
    FUN_0052bc40(DAT_0071943c);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
