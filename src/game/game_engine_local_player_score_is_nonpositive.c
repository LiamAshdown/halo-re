// game_engine_local_player_score_is_nonpositive  (Ghidra: FUN_00466340; named per this rewrite --
// sharpens out/phase4/game_functions.md's guess: "Given a player identifier in EAX, returns
// whether that player's team currently has a non-positive score (used as a UI/logic
// predicate).")
// address 0x466340, size 77 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: types/game.h player::local_player_index (+0x02) and k_maximum_local_players (== 1,
//   "every local-player bound test is < 1"); the array this function actually indexes,
//   0x0087aa14, is documented in types/game.h as a scalar `int32_t game_engine_unknown_aa14`,
//   which does not match this function's float-array-by-local-player-index read -- kept as a
//   raw float array here rather than redefining that header's declaration.
// register convention: player handle in EAX (in_EAX).
//   // blam-cc: EAX -> player_handle
// UNSURE: this function's real name/purpose; whether it is really about "team score" (the
// functions.md guess) or a per-local-player value, given what it actually indexes; the true
// element count and type of 0x0087aa14 (types/game.h's own int32_t declaration of the same
// address looks incompatible with this function's float read).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern data_array *player_data;                     // 0x0087a480
extern float unknown_0087aa14[1]; // 0x0087aa14, UNSURE: see header (game.h has this as int32_t)

// blam-cc: EAX -> player_handle
// Returns whether `player_handle` is a local player whose slot in unknown_0087aa14 is <= 0.0
// (true), or whether the checks don't apply (no multiplayer engine, invalid handle, not a local
// player) -- also true in those cases. False only when the engine is running, the handle is
// valid, it is a local player, and its value is > 0.0.
uint8_t game_engine_local_player_score_is_nonpositive(datum_index player_handle)
{
    player *p;
    int16_t local_player_index;

    if (current_game_engine == 0 || player_handle == (datum_index)0xffffffff) {
        return 1;
    }

    p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    local_player_index = p->local_player_index;
    if (local_player_index != -1) {
        if (unknown_0087aa14[local_player_index] > 0.0f) {
            return 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x466340), from tools/pack.py 0x466340:

undefined2 FUN_00466340(void)

{
  short sVar1;
  uint in_EAX;

  if ((DAT_006f1d20 != 0) && (in_EAX != 0xffffffff)) {
    sVar1 = *(short *)((in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 2);
    if (sVar1 != -1) {
      if (0.0 < *(float *)(&DAT_0087aa14 + sVar1 * 4)) {
        return 0;
      }
    }
  }
  return 1;
}
#endif
