// game_engine_apply_catchup_speed_boost  (Ghidra: FUN_0046e310; named per this rewrite)
// address 0x46e310, size 227 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md's guess ("HUD highlight/pulse weight... how recently
//   each object last scored") does not match the field written: player + 0x6c is player::speed
//   (types/game.h; the constructors seed it to 1.0). CORRECTED here: this sets each player's
//   speed multiplier to one of three tiers (1.0 / ~1.1 / ~1.2) based on how far behind the
//   current per-player leader (the highest player + 0xc6, the same UNSURE field used elsewhere
//   in this batch) they are, halved into thirds for the CTF ctf_option_7c==2 sub-mode.
// register convention: no parameters.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data;          // 0x0087a480
extern game_variant game_engine_variant; // 0x006f1c88 (ctf_option_7c aliased 0x006f1d04)

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

// Finds the highest player::unknown_c6 (this batch's UNSURE "score-ish" field) across all
// players, then gives every player a speed multiplier of 1.0 (at or ahead of the leader),
// ~1.1 (behind by 1, or by <3 in ctf_option_7c==2 mode where the gap is divided by 3), or ~1.2
// (further behind than that).
void game_engine_apply_catchup_speed_boost(void)
{
    data_iterator iter;
    player *p;
    int32_t leader = 0;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    p = (player *)data_iterator_next(&iter);
    while (p != (player *)0) {
        int16_t value = *(int16_t *)((uint8_t *)p + 0xc6);
        if (leader <= value) {
            leader = value;
        }
        p = (player *)data_iterator_next(&iter);
    }

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    p = (player *)data_iterator_next(&iter);
    while (p != (player *)0) {
        float speed = 1.0f;
        int32_t gap = leader - *(int16_t *)((uint8_t *)p + 0xc6);
        if (game_engine_variant.engine.race.race_type == 2) {
            gap /= 3;
        }
        if (gap < 2) {
            if (gap > 0) {
                speed = 1.1f;
            }
        } else {
            speed = 1.2f;
        }
        p->speed = speed;
        p = (player *)data_iterator_next(&iter);
    }
}

#if 0
Original Ghidra decompilation (0x46e310), from tools/pack.py 0x46e310:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0046e310(void)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  int iVar4;

  iVar4 = 0;
  iVar2 = data_iterator_next();
  while (iVar2 != 0) {
    if (iVar4 <= *(short *)(iVar2 + 0xc6)) {
      iVar4 = (int)*(short *)(iVar2 + 0xc6);
    }
    iVar2 = data_iterator_next();
  }
  iVar2 = data_iterator_next();
  while (iVar2 != 0) {
    uVar1 = 0x3f800000;
    iVar3 = iVar4 - *(short *)(iVar2 + 0xc6);
    if (_DAT_006f1d04 == 2) {
      iVar3 = iVar3 / 3;
    }
    if (iVar3 < 2) {
      if (0 < iVar3) {
        uVar1 = 0x3f8ccccd;
      }
    }
    else {
      uVar1 = 0x3f99999a;
    }
    *(undefined4 *)(iVar2 + 0x6c) = uVar1;
    iVar2 = data_iterator_next();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
