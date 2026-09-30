// game_engine_team_has_scoring_capacity  (Ghidra: FUN_0046e250; named per its summary)
// address 0x46e250, size 186 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x46e250
//   --stop-address=0x46e30a): Ghidra's own decompile shows a nonsensical
//   `*(int*)(iVar2+0x34) + 0x1fffe34` (a stack-slot misattribution after the extra `push ebp`
//   shifts every later `[esp+N]` by 4); the disassembly resolves it as `[esp+0x18]` actually
//   being the loop's OWN data_iterator.index (each iteration's current player's own datum
//   handle), and the two fields read off it are player::unit (0x34) and player::deaths (0xae,
//   read as int16) -- not some unrelated pointer chain. game_engine_variant::lives_per_round
//   aliased 0x006f1cd8, score_limit aliased 0x006f1ce0.
// register convention: `game_engine_variant.ctf_value_80 != 0` short-circuits with a leaked
//   register value in its return (see UNSURE); team index is the stack parameter (Ghidra's own
//   param_1).
// UNSURE: when ctf_value_80 != 0, Ghidra shows `return CONCAT31(garbage, 1)` -- narrowed here
//   to a plain `return 1` (matching the object_disconnect_from_map.c precedent for this exact
//   decompiler artifact).
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <stdint.h>

extern data_array *player_data;          // 0x0087a480
extern game_variant game_engine_variant; // 0x006f1c88 (ctf_value_80/lives_per_round/score_limit
                                          // aliased 0x006f1d08/0x006f1cd8/0x006f1ce0)

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

// Scans every in-use player on `team` with deaths below score_limit; if lives_per_round is
// configured (> 0) and any such player's unit is gone (-1) while their own deaths have already
// reached lives_per_round, the team is reported as NOT having scoring capacity.
uint8_t game_engine_team_has_scoring_capacity(int32_t team)
{
    data_iterator iter;
    player *p;
    uint8_t has_capacity = 1;

    if (game_engine_variant.ctf_value_80 != 0) {
        return 1; // UNSURE: see header
    }

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    p = (player *)data_iterator_next(&iter);
    while (p != (player *)0) {
        if (p->team == team &&
            *(int16_t *)((uint8_t *)p + 0xc6) < game_engine_variant.score_limit && // UNSURE field
            game_engine_variant.lives_per_round > 0) {
            // The original re-derives this same player via the iterator's own index field
            // (a corrected stack-slot read, see header); `p` already is that same player.
            if (p->unit == (datum_index)0xffffffff &&
                (int32_t)p->deaths >= game_engine_variant.lives_per_round) {
                has_capacity = 0;
            }
        }
        p = (player *)data_iterator_next(&iter);
    }
    return has_capacity;
}

#if 0
Original Ghidra decompilation (0x46e250), from tools/pack.py 0x46e250:

uint FUN_0046e250(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  byte bVar4;

  iVar2 = DAT_0087a480;
  bVar4 = 1;
  if (DAT_006f1d08 != 0) {
    return CONCAT31((int3)((uint)DAT_006f1d08 >> 8),1);
  }
  iVar3 = data_iterator_next();
  iVar1 = DAT_006f1cd8;
  if (iVar3 == 0) {
    return 1;
  }
  do {
    if ((((*(int *)(iVar3 + 0x20) == param_1) && (*(short *)(iVar3 + 0xc6) < DAT_006f1ce0)) &&
        (0 < iVar1)) &&
       ((*(int *)(*(int *)(iVar2 + 0x34) + 0x1fffe34) == -1 &&
        (iVar1 <= *(short *)(*(int *)(iVar2 + 0x34) + 0x1fffeae))))) {
      bVar4 = 0;
    }
    iVar3 = data_iterator_next();
  } while (iVar3 != 0);
  return (uint)bVar4;
}
#endif
