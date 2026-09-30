// hud_unit_meters_update  (Ghidra: FUN_004b0110, renamed in the phase-4 review)
// address 0x4b0110, size 71 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4b0110..0x4b0156. The retail local player loop around
// hud_unit_meters_update_for_player (0x4b0160), which takes the index in DI (the first rewrite
// called it with no argument).
// register convention: none.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern player_globals *local_player_globals; // 0x0087a478


void hud_unit_meters_update(void)
{
    int16_t local_player_index = local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1;

    while (local_player_index != -1) {
        hud_unit_meters_update_for_player(local_player_index);
        local_player_index = (local_player_globals->local_players[0] != (datum_index)-1 && local_player_index < 0)
                                 ? 0 : -1;
    }
}

#if 0
Original Ghidra decompilation (0x4b0110):

void FUN_004b0110(void)

{
  short sVar1;
  short sVar2;

  sVar2 = -1;
  if (*(int *)(DAT_0087a478 + 4) != -1) {
    sVar2 = 0;
  }
  while (sVar1 = sVar2, sVar1 != -1) {
    hud_meter_update_value();
    sVar2 = -1;
    if ((*(int *)(DAT_0087a478 + 4) != -1) && (sVar1 < 0)) {
      sVar2 = 0;
    }
  }
  return;
}
#endif
