// game_engine_player_round_reset  (Ghidra: FUN_00463620; renamed per its summary and per
// game_engine_definition::player_round_reset, the vtable slot it invokes)
// address 0x463620, size 50 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Clears a player per-round engine bookkeeping and
// invokes the active game variant optional reset callback"); types/game.h
// game_engine_definition::player_round_reset (+0x98, "0x463620" -- this function); src/game/
// game_engine_reset_respawns_and_cleanup_bipeds.c's player_kill_and_release_unit signature.
// register convention: a player handle, in EAX at entry (immediately copied into the
// callee-saved EBX -- see FIXED note below -- and forwarded to player_kill_and_release_unit and
// to the player_round_reset callback unchanged); a second, callback-only argument is this
// function's own stack parameter.
//   // blam-cc: EAX -> player_handle, stack -> callback_argument
// FIXED (register inputs, objdump): EAX carries player_handle (read at 0x463621, mov ebx,eax,
// right after the prologue's `push ebx`). The old note attributed the handle to EBX itself, but
// EBX is only a callee-saved scratch copy of EAX made here and restored at 0x463650/0x463651;
// EAX is the true live-in. Disassembly also shows the player_round_reset callback is invoked
// with two arguments (0x463645-0x46364b: `push ecx` (this function's own first stack slot,
// callback_argument) then `push ebx` (player_handle), `call eax`), not zero as the old
// `(void (*)(void))` cast modeled -- corrected here since it was found while tracing player_handle.

// VERIFIED against disassembly 0x463620..0x463651 (2026-09-30): callback (+0x98) gets (player_handle, callback_argument).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20

extern void player_kill_and_release_unit(int32_t respawn_time); // 0x476250, not in this batch;
    // blam-cc: EBX -> player_handle, stack -> respawn_time

// blam-cc: EAX -> player_handle, stack -> callback_argument
void game_engine_player_round_reset(int32_t player_handle, int32_t callback_argument)
{
    if (current_game_engine != 0) {
        player_kill_and_release_unit(0); // relies on EBX already holding player_handle, see below
        if (current_game_engine->player_round_reset != 0) {
            // UNSURE: callback_argument's real meaning; forwarded exactly as objdump shows it.
            ((void (*)(int32_t, int32_t))current_game_engine->player_round_reset)(
                player_handle, callback_argument);
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
