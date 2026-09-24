// game_engine_compute_local_player_look_vector  (Ghidra: FUN_00471f40; renamed, no established
// name)
// address 0x471f40, size 73 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Thin wrapper around the look-vector computation
// player_compute_view_forward_vector for a given local-player/unit context"); types/game.h local_player_control (yaw
// +0x0c, immediately followed by pitch +0x10), player_globals::local_player_units (+0x04).
//
// Fully reconstructed against objdump (--start-address=0x471f40 --stop-address=0x471f90
// bin/halo.exe): Ghidra's rendering shows both branches calling player_compute_view_forward_vector() identically with
// no visible arguments, but the real code sets EAX differently per branch (the local player's
// driven unit, or k_datum_index_none when local_player_index is out of the single-local-player
// range), which is why the two branches are not actually redundant.
// register convention: local_player_index in CX; EAX is saved into ESI at entry and restored
// before returning, so it is not a real input.
//   // blam-cc: CX -> local_player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern player_globals *local_player_globals;                  // 0x0087a478

extern void player_compute_view_forward_vector(datum_index unit, real *yaw_pitch); // 0x473d70, not in this batch;
    // blam-cc: EAX -> unit, ECX -> yaw_pitch (address of local_player_control::yaw, with pitch
    // immediately following it)

// blam-cc: CX -> local_player_index
void game_engine_compute_local_player_look_vector(int16_t local_player_index)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    datum_index unit = k_datum_index_none;

    if (local_player_index != -1 && local_player_index < 1) {
        unit = local_player_globals->local_player_units[local_player_index];
    }
    player_compute_view_forward_vector(unit, &look->yaw);
}

#if 0
Original Ghidra decompilation (0x471f40), from tools/pack.py 0x471f40 -- the two branches look
identical here; see the header comment for what objdump shows they actually do differently.

void FUN_00471f40(void)

{
  short in_CX;

  if ((in_CX != -1) && (in_CX < 1)) {
    FUN_00473d70();
    return;
  }
  FUN_00473d70();
  return;
}
#endif
