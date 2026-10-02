// hud_unit_meters_update_for_player  (Ghidra: hud_meter_update_value, renamed in the phase-4
// review)
// address 0x4b0160, size 442 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4b0160..0x4b0319 in the phase-4 review (the first rewrite
// had the local player index as a stack argument; it comes in DI). Snaps the displayed health
// and shield of a -1.0 record to the unit; when the shield dropped, holds the displayed shield
// for 15 ticks (shield_update_time is re-armed whenever shield_drain_time is outside 0..1),
// then follows the shield and grows shield_drain_time by 1/30 per elapsed tick; a recharging
// shield snaps and sets shield_drain_time to -1.0; an unchanged shield keeps growing a running
// drain time. During a cinematic (byte 9 of 0x006f187c) it also runs hud_unit_sounds_update
// with the HUD enabled byte, since the HUD pass does not.
// register convention: DI local player index.
//   // blam-cc: local_player_index -> DI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"
#include "units.h"
#include "cutscene.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern player_globals *local_player_globals;   // 0x0087a478
extern data_array *player_data;                // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern game_time_globals *game_time;           // 0x006f1d6c
extern hud_unit_meter_globals *hud_unit_meters; // 0x0071942c
extern hud_globals_flags *hud_flags;           // 0x00719420
extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c

extern void hud_unit_sounds_update(player *p, uint8_t hud_enabled); // 0x4afee0, blam-cc: EAX player

// blam-cc: local_player_index -> DI
void hud_unit_meters_update_for_player(int16_t local_player_index)
{
    datum_index player_index;

    if (local_player_index != -1 && local_player_index < 1) {
        player_index = local_player_globals->local_players[local_player_index];
        if (player_index != (datum_index)-1) {
            datum_index unit_index = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200))->unit;

            if (unit_index != (datum_index)-1) {
                uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
                hud_unit_meter_state *state = &hud_unit_meters->players[local_player_index];
                float shield = ((unit_object *)unit)->base.shield_vitality;

                if (state->displayed_health == -1.0f) {
                    state->displayed_health = ((unit_object *)unit)->base.body_vitality;
                }
                if (state->displayed_shield == -1.0f) {
                    state->displayed_shield = shield;
                }
                if (state->displayed_shield > shield) {
                    int32_t elapsed;

                    if (state->shield_drain_time < 0.0f || state->shield_drain_time > 1.0f) {
                        state->shield_update_time = game_time->game_time;
                    }
                    elapsed = game_time->game_time - state->shield_update_time;
                    if (elapsed < 0xf) {
                        state->shield_drain_time = 0.0f;
                    } else {
                        state->displayed_shield = ((unit_object *)unit)->base.shield_vitality;
                        state->shield_drain_time = (float)(game_time->game_time - state->shield_update_time) *
                                                       0.03333333507180214f + state->shield_drain_time;
                        state->shield_update_time = game_time->game_time;
                    }
                } else if (state->displayed_shield < shield) {
                    state->displayed_shield = shield;
                    state->shield_drain_time = -1.0f;
                    state->shield_update_time = game_time->game_time;
                } else {
                    state->displayed_shield = shield;
                    if (state->shield_drain_time > 0.0f) {
                        state->shield_drain_time = (float)(game_time->game_time - state->shield_update_time) *
                                                       0.03333333507180214f + state->shield_drain_time;
                    }
                    state->shield_update_time = game_time->game_time;
                }
            }
        }
    }

    if (cinematic_globals_ptr->in_progress != 0 && local_player_index != -1 && local_player_index < 1) {
        player_index = local_player_globals->local_players[local_player_index];
        if (player_index != (datum_index)-1) {
            hud_unit_sounds_update((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200),
                                   hud_flags->hud_enabled);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4b0160):

void hud_meter_update_value(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  float fVar4;
  int iVar5;
  float *pfVar6;
  short unaff_DI;

  iVar3 = DAT_0087a478;
  if ((unaff_DI != -1) && (unaff_DI < 1)) {
    uVar1 = *(uint *)(DAT_0087a478 + 4 + unaff_DI * 4);
    if ((uVar1 != 0xffffffff) &&
       (uVar1 = *(uint *)((uVar1 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)),
       uVar1 != 0xffffffff)) {
      iVar5 = unaff_DI * 0x58;
      iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
      pfVar6 = (float *)(iVar5 + DAT_0071942c);
      if (*(int *)(iVar5 + 4 + DAT_0071942c) == -0x40800000) {
        pfVar6[1] = *(float *)(iVar2 + 0xe0);
      }
      if (*pfVar6 == -1.0) {
        *pfVar6 = *(float *)(iVar2 + 0xe4);
      }
      iVar5 = DAT_006f1d6c;
      if (*pfVar6 <= *(float *)(iVar2 + 0xe4)) {
        if (*pfVar6 < *(float *)(iVar2 + 0xe4)) {
          *pfVar6 = *(float *)(iVar2 + 0xe4);
          pfVar6[2] = -1.0;
          pfVar6[3] = *(float *)(iVar5 + 0xc);
          goto LAB_004b02ce;
        }
        *pfVar6 = *(float *)(iVar2 + 0xe4);
        iVar2 = DAT_006f1d6c;
        if (0.0 < pfVar6[2]) {
          pfVar6[2] = (float)(*(int *)(DAT_006f1d6c + 0xc) - (int)pfVar6[3]) * 0.033333335 +
                      pfVar6[2];
        }
        fVar4 = *(float *)(iVar2 + 0xc);
      }
      else {
        if ((pfVar6[2] < 0.0) || (1.0 < pfVar6[2])) {
          pfVar6[3] = *(float *)(DAT_006f1d6c + 0xc);
        }
        if (*(int *)(iVar5 + 0xc) - (int)pfVar6[3] < 0xf) {
          pfVar6[2] = 0.0;
          goto LAB_004b02ce;
        }
        *pfVar6 = *(float *)(iVar2 + 0xe4);
        pfVar6[2] = (float)(*(int *)(iVar5 + 0xc) - (int)pfVar6[3]) * 0.033333335 + pfVar6[2];
        fVar4 = *(float *)(iVar5 + 0xc);
      }
      pfVar6[3] = fVar4;
    }
  }
LAB_004b02ce:
  if ((((*(char *)(DAT_006f187c + 9) != '\0') && (unaff_DI != -1)) && (unaff_DI < 1)) &&
     (*(int *)(iVar3 + 4 + unaff_DI * 4) != -1)) {
    FUN_004afee0(*DAT_00719420);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
