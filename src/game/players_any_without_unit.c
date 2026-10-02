// players_any_without_unit  (Ghidra: players_any_without_unit, already named)
// address 0x475210, size 82 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/game_functions.md ("Returns whether any player currently lacks a
//   controlled unit"); types/game.h player::unit (+0x34).
// register convention: no arguments; return value in EAX.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator

// Returns true if any live player currently has no controlled unit.
uint8_t players_any_without_unit(void)
{
    data_iterator iter;
    player *plr;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    plr = (player *)data_iterator_next(&iter);
    while (plr != (player *)0) {
        if (plr->unit == (datum_index)-1) {
            return 1;
        }
        plr = (player *)data_iterator_next(&iter);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x475210), from tools/pack.py 0x475210:

undefined4 players_any_without_unit(void)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while( true ) {
    if (iVar1 == 0) {
      return 0;
    }
    if (*(int *)(iVar1 + 0x34) == -1) break;
    iVar1 = data_iterator_next();
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
