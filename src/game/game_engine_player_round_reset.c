// game_engine_player_round_reset  (Ghidra: FUN_00463620; renamed per its summary and per
// game_engine_definition::player_round_reset, the vtable slot it invokes)
// address 0x463620, size 50 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Clears a player per-round engine bookkeeping and
// invokes the active game variant optional reset callback"); types/game.h
// game_engine_definition::player_round_reset (+0x98, "0x463620" -- this function); src/game/
// game_engine_reset_respawns_and_cleanup_bipeds.c's player_kill_and_release_unit signature.
// register convention: a player handle in EBX (unaff_EBX, forwarded to player_kill_and_release_unit unchanged).
//   // blam-cc: unaff_EBX -> player_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20

extern void player_kill_and_release_unit(int32_t respawn_time); // 0x476250, not in this batch;
    // blam-cc: EBX -> player_handle, stack -> respawn_time

// blam-cc: unaff_EBX -> player_handle (not modeled as a C parameter; see
// game_engine_reset_respawns_and_cleanup_bipeds.c for the same convention)
void game_engine_player_round_reset(void)
{
    if (current_game_engine != 0) {
        player_kill_and_release_unit(0); // UNSURE: relies on EBX already holding the player handle
        if (current_game_engine->player_round_reset != 0) {
            ((void (*)(void))current_game_engine->player_round_reset)();
        }
    }
}

#if 0
Original Ghidra decompilation (0x463620), from tools/pack.py 0x463620:

void FUN_00463620(void)

{
  if (DAT_006f1d20 != 0) {
    FUN_00476250(0);
    if (*(code **)(DAT_006f1d20 + 0x98) != (code *)0x0) {
      (**(code **)(DAT_006f1d20 + 0x98))();
    }
  }
  return;
}
#endif
