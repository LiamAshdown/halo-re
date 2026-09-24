// game_engine_update_custom_waypoint_navpoints  (Ghidra: FUN_00462a90; renamed per its summary)
// address 0x462a90, size 313 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("For each active custom waypoint that passes the
// player/team filter, pushes an add-or-update call into the interface HUD nav-point system");
// types/game.h player_globals::local_players (+0x04), player::unit (+0x34), player::team
// (+0x20), custom_waypoint::team (+0x14), game_variant::ctf_option_7c (+0x7c, aliased
// 0x006f1d04), game_variant::unknown_3c (+0x3c, aliased 0x006f1cc4), game_engine_index
// (_game_engine_ctf == 1); this batch's custom_waypoint_matches_filter (0x4620c0).
// register convention: the local player index in BX (unaff_BX, only ever tested against -1/0
// and used to index player_globals::local_players -- i.e. it must be 0, matching
// k_maximum_local_players).
//   // blam-cc: unaff_BX -> local_player_slot
// UNSURE: unit_get_primary_eye_marker_position (a per-tick reset of some kind, called once up front), hud_waypoint_visibility (the
// nav-point add/update call, both its 1- and 2-argument shapes) and hud_waypoint_draw's own
// arguments here are all outside this batch's evidence.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88 (unknown_3c aliased 0x006f1cc4,
                                                     // ctf_option_7c aliased 0x006f1d04)
extern player_globals *local_player_globals;          // 0x0087a478
extern data_array *player_data;                     // 0x0087a480
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints]; // 0x006f1888

extern uint8_t custom_waypoint_matches_filter(int32_t candidate, custom_waypoint *slot,
    int32_t reference_team); // 0x4620c0, this batch
extern void unit_get_primary_eye_marker_position(void); // 0x568f50, not in this batch
extern int16_t hud_waypoint_visibility(uint32_t unknown_0, uint32_t unknown_1); // 0x4af540, not in this batch;
    // UNSURE: also called here with just one argument
extern void hud_waypoint_draw(void); // 0x4af5e0, not in this batch; UNSURE args

// blam-cc: unaff_BX -> local_player_slot
void game_engine_update_custom_waypoint_navpoints(int16_t local_player_slot)
{
    datum_index local_player;
    player *p;
    int32_t slot;

    if (current_game_engine == 0 || game_engine_variant.unknown_3c != 1 ||
        local_player_slot == -1 || 1 <= local_player_slot) {
        return;
    }
    local_player = local_player_globals->local_players[local_player_slot];
    if (local_player == (datum_index)0xffffffff) {
        return;
    }
    p = (player *)((uint8_t *)player_data->data + (local_player & 0xffff) * sizeof(player));
    if (p->unit == (datum_index)0xffffffff) {
        return;
    }

    unit_get_primary_eye_marker_position();

    for (slot = 0; slot < k_maximum_custom_waypoints; slot++) {
        if (custom_waypoint_matches_filter((int32_t)local_player, &custom_waypoints[slot], p->team) != 0) {
            if (current_game_engine == 0 || current_game_engine->index != _game_engine_ctf ||
                game_engine_variant.ctf_option_7c != 0 ||
                custom_waypoints[slot].team == p->team || custom_waypoints[slot].team == -1) {
                hud_waypoint_visibility(0xffffffff, 1);
            } else {
                int16_t result = hud_waypoint_visibility(0xffffffff, 0); // UNSURE: 1-arg call in Ghidra;
                    // modeled with a placeholder second argument
                if (result != 0) {
                    continue;
                }
            }
            hud_waypoint_draw();
        }
    }
}

#if 0
Original Ghidra decompilation (0x462a90), from tools/pack.py 0x462a90:

void FUN_00462a90(void)

{
  uint uVar1;
  char cVar2;
  short sVar3;
  short unaff_BX;
  int iVar4;
  undefined4 *puVar5;

  if (((((DAT_006f1d20 != 0) && (DAT_006f1cc4 == 1)) && (unaff_BX != -1)) &&
      ((unaff_BX < 1 && (uVar1 = *(uint *)(DAT_0087a478 + 4 + unaff_BX * 4), uVar1 != 0xffffffff))))
     && (iVar4 = (uVar1 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34),
        *(int *)(iVar4 + 0x34) != -1)) {
    FUN_00568f50();
    puVar5 = &DAT_006f1888;
    do {
      cVar2 = FUN_004620c0(uVar1);
      if (cVar2 != '\0') {
        if (((DAT_006f1d20 == 0) || (*(int *)(DAT_006f1d20 + 4) != 1)) ||
           ((DAT_006f1d04 != '\0' ||
            (((int)*(short *)(puVar5 + 5) == *(int *)(iVar4 + 0x20) ||
             (*(short *)(puVar5 + 5) == -1)))))) {
          FUN_004af540(0xffffffff,1);
        }
        else {
          sVar3 = FUN_004af540(0xffffffff);
          if (sVar3 != 0) goto LAB_00462baa;
        }
        hud_waypoint_draw();
      }
LAB_00462baa:
      puVar5 = puVar5 + 8;
    } while ((int)puVar5 < 0x6f1c88);
  }
  return;
}
#endif
