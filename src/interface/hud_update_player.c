// hud_update_player  (Ghidra: hud_update_player, already named)
// address 0x4a99f0, size 321 bytes
// name confidence: 0.5 (existing Ghidra name)   rewrite confidence: 0.6
// evidence: types/game.h player_globals::local_players (0x0087a478) and player_data (0x0087a480,
// stride 0x200 = sizeof(player)); types/game.h player::unit (offset 0x34, "-1 when dead",
// matching the `+0x34 != -1` test here); types/interface.h hud_globals_flags::hud_enabled
// (0x00719420); src/game/*.c's established camera_get_type_for_player.
// UNSURE: the five globals gating the waypoint-draw and motion-sensor calls (0x006f1d20,
// 0x006f1cc0, 0x006f1cbc, 0x006f187c, 0x006f1d6c) and DAT_007c3108 (the "current player" short
// this function starts from) are not documented by any module read in this pass. UNSURE: six of
// this function's callees are out of this module's range (camera_get_type_for_player already
// has a signature established elsewhere; chimera__spectate_hud, hud_render_unit_interface,
// FUN_004afb90, FUN_004b14c0, hud_messaging_update, chimera__motion_sensor_update do not) and
// are declared here with best-guess signatures.
// Phase-4 review against objdump 0x4a99f0..0x4a9b30: FUN_004afee0 takes the player record in
// EAX besides the flag, FUN_004b14c0 and hud_messaging_update take the local player index in
// EAX (the last is a tail jump); the renamed 0x4a9b80 is hud_update_interaction_prompt.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t current_local_player_index; // 0x007c3108
extern player_globals *local_player_globals;   // 0x0087a478, established in src/game/
extern data_array *player_data;                // 0x0087a480, established in src/game/
extern hud_globals_flags *hud_flags;           // 0x00719420

extern uint8_t current_game_engine; // UNSURE
extern uint8_t motion_sensor_override_value; // UNSURE
extern uint8_t game_engine_teams_enabled_flag; // UNSURE
extern uint8_t *cinematic_globals_ptr; // UNSURE: byte 9 tested
extern game_time_globals *game_time; // 0x006f1d6c

extern int16_t camera_get_type_for_player(int16_t local_player_index); // 0x445ac0, CX
extern void hud_draw_weapon_interface(player *p); // 0x4b1e20, cdecl
extern void hud_update_interaction_prompt(datum_index player_index); // 0x4a9b80, blam-cc: player_index -> EDX
extern void hud_unit_sounds_update(player *p, uint8_t hud_enabled); // 0x4afee0, blam-cc: EAX player
extern void hud_render_unit_interface(player *p); // 0x4b0320, cdecl
extern void hud_waypoints_draw_for_player(int16_t local_player_index); // 0x4afb90, cdecl
extern void hud_draw_damage_indicators(int16_t local_player_index); // 0x4b14c0, blam-cc: EAX
extern void hud_messaging_update(int16_t local_player_index); // 0x4ae550, blam-cc: EAX
extern void hud_waypoint_draw_all_for_player(void); // 0x4aa5f0
extern void chimera__motion_sensor_update(void); // 0x4b3920, UNSURE signature

// Updates and renders the full player HUD for the currently displayed local player: waypoints
// and motion sensor first (subject to several unrelated gates), then either the full
// unit-driving HUD (spectate overlay, weapon message state, unit interface, ammo/name overlay)
// or, if not currently driving a unit or the camera is in a cutscene/first-person-only mode,
// just the weapon message state and a bare messaging update.
void hud_update_player(void)
{
    int16_t local_player_index = current_local_player_index;
    datum_index player_index;
    int16_t camera_type;

    if (local_player_index == -1 || local_player_index > 0) {
        player_index = (datum_index)-1;
    } else {
        player_index = local_player_globals->local_players[local_player_index];
    }
    camera_type = camera_get_type_for_player(local_player_index); // FIXED: CX = the local player index (0x4a99f0)

    if (player_index == (datum_index)-1) {
        return;
    }

    {
        player *local_player = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));

        if ((current_game_engine == 0 || ((motion_sensor_override_value & 2) != 0 && game_engine_teams_enabled_flag != 0)) &&
            cinematic_globals_ptr[9] == 0) {
            hud_waypoint_draw_all_for_player();
        }

        if (game_time->paused == 0) {
            int16_t expected = (local_player_globals->local_players[0] != (datum_index)-1) ? 0 : -1;
            if (current_local_player_index == expected) {
                chimera__motion_sensor_update();
            }
        }

        if (hud_flags->hud_enabled == 0) {
            hud_unit_sounds_update(local_player, 0);
            hud_messaging_update(current_local_player_index);
            return;
        }

        if (camera_type != 3 && camera_type != 2 && local_player->unit != (datum_index)-1) {
            hud_draw_weapon_interface(local_player);
            hud_update_interaction_prompt(player_index);
            hud_unit_sounds_update(local_player, hud_flags->hud_enabled);
            hud_render_unit_interface(local_player);
            hud_waypoints_draw_for_player(current_local_player_index);
            hud_draw_damage_indicators(current_local_player_index);
            hud_messaging_update(current_local_player_index);
            return;
        }

        hud_update_interaction_prompt(player_index);
        hud_unit_sounds_update(local_player, hud_flags->hud_enabled);
        hud_messaging_update(current_local_player_index);
    }
}

#if 0
Original Ghidra decompilation (0x4a99f0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl hud_update_player(void)

{
  short sVar1;
  short sVar2;
  int iVar3;
  uint uVar4;
  char cVar5;

  sVar1 = (short)_DAT_007c3108;
  if ((sVar1 == -1) || (0 < sVar1)) {
    uVar4 = 0xffffffff;
  }
  else {
    uVar4 = *(uint *)(DAT_0087a478 + 4 + sVar1 * 4);
  }
  sVar1 = camera_get_type_for_player();
  if (uVar4 != 0xffffffff) {
    iVar3 = (uVar4 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
    if (((DAT_006f1d20 == 0) || ((((byte)DAT_006f1cc0 & 2) != 0 && (DAT_006f1cbc != '\0')))) &&
       (*(char *)(DAT_006f187c + 9) == '\0')) {
      FUN_004aa5f0();
    }
    if (*(char *)(DAT_006f1d6c + 2) == '\0') {
      sVar2 = -1;
      if (*(int *)(DAT_0087a478 + 4) != -1) {
        sVar2 = 0;
      }
      if (DAT_007c3108 == sVar2) {
        chimera__motion_sensor_update();
      }
    }
    if (*DAT_00719420 == '\0') {
      cVar5 = '\0';
    }
    else {
      if (((sVar1 != 3) && (sVar1 != 2)) && (*(int *)(iVar3 + 0x34) != -1)) {
        chimera__spectate_hud(iVar3);
        FUN_004a9b80();
        FUN_004afee0(*DAT_00719420);
        hud_render_unit_interface(iVar3);
        FUN_004afb90(_DAT_007c3108 & 0xffff);
        FUN_004b14c0();
        hud_messaging_update();
        return;
      }
      FUN_004a9b80();
      cVar5 = *DAT_00719420;
    }
    FUN_004afee0(cVar5);
    hud_messaging_update();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
