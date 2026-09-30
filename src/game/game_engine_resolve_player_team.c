// game_engine_resolve_player_team  (Ghidra: FUN_004611b0; renamed per its summary)
// address 0x4611b0, size 64 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Resolves and stores a player's team id, preferring
// the active game variant's callback over the default derivation"); types/game.h
// game_engine_definition::player_team_changed (+0x74, "0x4611b0" -- this function is its sole
// caller), player::local_player_index (+0x02), player::team (+0x20).
// register convention: a player index in EAX (in_EAX).
//   // blam-cc: EAX -> player_index
// UNSURE: the sign-correcting mask (`& 0x80000001`, then the two's-complement fixup for a
// negative result) computes local_player_index modulo 2 with defensive handling for a negative
// index; transcribed literally rather than simplified to `% 2`, since local_player_index is
// documented elsewhere as -1 for a non-local player and this function runs for every player, not
// just local ones.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern data_array *player_data;                     // 0x0087a480

// blam-cc: EAX -> player_index
// If the active game engine implements player_team_changed, defers the whole team assignment to
// it. Otherwise falls back to alternating teams by parity of the player's local_player_index.
void game_engine_resolve_player_team(uint32_t player_index)
{
    player *p;

    if (current_game_engine == 0) {
        return;
    }
    if (current_game_engine->player_team_changed != 0) {
        ((void (*)(void))current_game_engine->player_team_changed)(); // UNSURE: real arguments unrecoverable
        return;
    }

    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    {
        uint32_t team = (uint32_t)p->local_player_index & 0x80000001;
        if ((int32_t)team < 0) {
            team = (team - 1 | 0xfffffffe) + 1;
        }
        p->team = (int32_t)team;
    }
}

#if 0
Original Ghidra decompilation (0x4611b0), from tools/pack.py 0x4611b0:

void FUN_004611b0(void)

{
  uint in_EAX;
  int iVar1;
  uint uVar2;
  
  if (DAT_006f1d20 != 0) {
    if (*(code **)(DAT_006f1d20 + 0x74) != (code *)0x0) {
      (**(code **)(DAT_006f1d20 + 0x74))();
      return;
    }
    iVar1 = (in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
    uVar2 = (int)*(short *)(iVar1 + 2) & 0x80000001;
    if ((int)uVar2 < 0) {
      uVar2 = (uVar2 - 1 | 0xfffffffe) + 1;
    }
    *(uint *)(iVar1 + 0x20) = uVar2;
  }
  return;
}
#endif
