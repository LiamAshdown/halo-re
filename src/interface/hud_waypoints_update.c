// hud_waypoints_update  (Ghidra: FUN_004af320, renamed in the phase-4 review)
// address 0x4af320, size 75 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4af320..0x4af36a. The standard local player loop of retail PC (index 0
// while local_players[0] is set, then -1) around hud_waypoints_update_for_player (0x4af370).
// register convention: none.

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
extern player_globals *local_player_globals; // 0x0087a478

extern void hud_waypoints_update_for_player(int16_t local_player_index); // 0x4af370

void hud_waypoints_update(void)
{
    int16_t local_player_index = local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1;

    while (local_player_index != -1) {
        hud_waypoints_update_for_player(local_player_index);
        local_player_index = (local_player_globals->local_players[0] != (datum_index)-1 && local_player_index < 0)
                                 ? 0 : -1;
    }
}

#if 0
Original Ghidra decompilation (0x4af320):

void FUN_004af320(void)

{
  short sVar1;
  undefined4 uVar2;
  undefined4 uVar3;

  uVar3 = 0xffffffff;
  if (*(int *)(DAT_0087a478 + 4) != -1) {
    uVar3 = 0;
  }
  sVar1 = (short)uVar3;
  while (sVar1 != -1) {
    FUN_004af370(uVar3);
    uVar2 = 0xffffffff;
    if ((*(int *)(DAT_0087a478 + 4) != -1) && ((short)uVar3 < 0)) {
      uVar2 = 0;
    }
    uVar3 = uVar2;
    sVar1 = (short)uVar2;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
