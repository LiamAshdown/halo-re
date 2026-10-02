// network_object_release_ownership_claim  (Ghidra: FUN_004dfc10, unnamed)
// address 0x4dfc10, size 119 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Releases the network ownership claim on the
// object resolved from an unaff_BL machine/index value, notifying FUN_004779d0 and clearing
// the local ownership state via FUN_00466e80/FUN_00466ee0." Shares the exact
// player_data (0x0087a480)-lookup idiom with network_game_server_handoff_object_ownership.c
// (team at player+0x20, salt check against the resolved datum's high half).
// register convention: BL = slot_index (uint8_t).
// blam-cc: BL -> slot_index
// UNSURE: player_data_iterator_advance's, FUN_004779d0's, and the two ownership-state
// helpers' real signatures are inferred only from this and the sibling call site in
// network_game_server_handoff_object_ownership.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480, stride 0x200 (game module)
extern uint32_t player_data_iterator_advance(uint8_t slot_index); // 0x4d98f0, other module (UNSURE)
extern int32_t game_engine_notify_object_value_event(int32_t team); // 0x4779d0, other module (UNSURE)
extern int32_t game_engine_player_profile_cache_find(void); // 0x466e80, other module (UNSURE)
extern void game_engine_capture_player_profile(int32_t value); // 0x466ee0, other module (UNSURE)

// Resolves `slot_index` to a player datum; if it is live, notifies game_engine_notify_object_value_event of the
// player's team and, if a local ownership claim is active, clears it.
void network_object_release_ownership_claim(uint8_t slot_index)
{
    uint32_t datum;
    int16_t player_index;
    int16_t salt;
    player *plr;

    datum = player_data_iterator_advance(slot_index);
    if (datum == 0xffffffff) {
        return;
    }
    player_index = (int16_t)datum;
    if (player_index < 0 || player_index >= player_data->maximum_count) {
        return;
    }
    plr = (player *)((uint8_t *)player_data->data + player_data->size * player_index);
    if (plr->identifier == 0) {
        return;
    }
    salt = (int16_t)(datum >> 16);
    if (salt != 0 && plr->identifier != salt) {
        return;
    }
    game_engine_notify_object_value_event(plr->team);
    if (game_engine_player_profile_cache_find() != -1) {
        game_engine_capture_player_profile(0);
    }
}

#if 0
Original Ghidra decompilation (0x4dfc10):

void FUN_004dfc10(void)

{
  int iVar1;
  int iVar2;
  short sVar3;
  undefined1 unaff_BL;
  short sVar4;

  iVar1 = FUN_004d98f0(unaff_BL);
  if (((iVar1 != -1) && (sVar4 = (short)iVar1, -1 < sVar4)) &&
     (sVar4 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar2 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar4;
    sVar4 = *(short *)(iVar2 + *(int *)(DAT_0087a480 + 0x34));
    if ((sVar4 != 0) && ((sVar3 = (short)((uint)iVar1 >> 0x10), sVar3 == 0 || (sVar4 == sVar3)))) {
      FUN_004779d0(*(undefined4 *)(iVar2 + *(int *)(DAT_0087a480 + 0x34) + 0x20));
      iVar1 = FUN_00466e80();
      if (iVar1 != -1) {
        FUN_00466ee0(0);
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
