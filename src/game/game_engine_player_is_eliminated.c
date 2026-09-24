// game_engine_player_is_eliminated  (Ghidra: FUN_00460f30; renamed per its summary)
// address 0x460f30, size 59 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Checks whether a player has been dead long enough...
// to be considered eliminated for the round"); types/game.h game_variant::lives_per_round
// (+0x50, "0 means unlimited. A player whose death count (player+0xae) reaches it is
// eliminated"), player::unit (+0x34), player::deaths (+0xae).
// register convention: a player index in ECX (in_ECX).
//   // blam-cc: ECX -> player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data;          // 0x0087a480
extern game_variant game_engine_variant;  // 0x006f1c88 (lives_per_round aliased as 0x006f1cd8)

// blam-cc: ECX -> player_index
// True when the game has a lives limit, the player is currently dead, and their death count has
// reached (or passed) that limit.
uint8_t game_engine_player_is_eliminated(uint32_t player_index)
{
    if (0 < game_engine_variant.lives_per_round) {
        player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
        if (p->unit == (datum_index)0xffffffff && game_engine_variant.lives_per_round <= p->deaths) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x460f30), from tools/pack.py 0x460f30:

undefined1 FUN_00460f30(void)

{
  undefined1 uVar1;
  uint in_ECX;
  int iVar2;
  
  uVar1 = 0;
  if (0 < DAT_006f1cd8) {
    iVar2 = (in_ECX & 0xffff) * 0x200;
    if ((*(int *)(iVar2 + 0x34 + *(int *)(DAT_0087a480 + 0x34)) == -1) &&
       (DAT_006f1cd8 <= *(short *)(iVar2 + *(int *)(DAT_0087a480 + 0x34) + 0xae))) {
      uVar1 = 1;
    }
  }
  return uVar1;
}
#endif
