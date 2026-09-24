// game_set_local_player  (Ghidra: game_set_local_player, already named)
// address 0x474d50, size 84 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/game_functions.md ("Assigns (or clears) which player index is bound to a
//   given local-player slot, keeping the player's back-reference in sync"); types/game.h
//   player_globals::local_players (+0x04), player::local_player_index (+0x02).
// register convention: ECX -> player_handle, SI -> local_player_index.
//   // blam-cc: ECX -> player_handle, SI -> local_player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480

// Binds `player_handle` to local-player slot `local_player_index` (only slot 0 is ever valid in
// this build). Clears the previous occupant's local_player_index back-reference first, and sets
// the new occupant's local_player_index to this slot unless player_handle is the wildcard.
void game_set_local_player(datum_index player_handle, int16_t local_player_index)
    // blam-cc: ECX -> player_handle, SI -> local_player_index
{
    datum_index previous;
    player *p;

    if (local_player_index >= 0 && local_player_index < 1) {
        previous = local_player_globals->local_players[local_player_index];
        if (previous != (datum_index)-1) {
            p = (player *)((uint8_t *)player_data->data + (previous & 0xffff) * sizeof(player));
            p->local_player_index = -1;
        }
        local_player_globals->local_players[local_player_index] = player_handle;
        if (player_handle != (datum_index)-1) {
            p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
            p->local_player_index = local_player_index;
        }
    }
}

#if 0
Original Ghidra decompilation (0x474d50), from tools/pack.py 0x474d50:

void game_set_local_player(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  uint in_ECX;
  short unaff_SI;

  iVar3 = DAT_0087a480;
  if ((-1 < unaff_SI) && (unaff_SI < 1)) {
    puVar1 = (uint *)(DAT_0087a478 + 4 + unaff_SI * 4);
    uVar2 = *puVar1;
    if (uVar2 != 0xffffffff) {
      *(undefined2 *)((uVar2 & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)) = 0xffff;
    }
    *puVar1 = in_ECX;
    if (in_ECX != 0xffffffff) {
      *(short *)((in_ECX & 0xffff) * 0x200 + 2 + *(int *)(iVar3 + 0x34)) = unaff_SI;
    }
  }
  return;
}
#endif
