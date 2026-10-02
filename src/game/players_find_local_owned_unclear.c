// players_find_local_owned_unclear  (Ghidra: FUN_00477280; named per this rewrite)
// address 0x477280, size 87 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Scans players for one owned by a local player; the
//   decompiled return value is always -1, so its effective behavior here is unclear").
//   Transcribed exactly: the scan runs (for its side effects on the implicit iterator, and
//   possibly because the source had more logic here that this retail build's optimizer proved
//   irrelevant to the return value) but the function always returns the wildcard.
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

// Scans players until one with local_player_index != -1 is found (or the iterator is
// exhausted), then always returns the datum-index wildcard.
datum_index players_find_local_owned_unclear(void)
{
    data_iterator iter;
    player *plr;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    plr = (player *)data_iterator_next(&iter);
    while (plr != (player *)0) {
        if (plr->local_player_index != -1) {
            break;
        }
        plr = (player *)data_iterator_next(&iter);
    }
    return (datum_index)-1;
}

#if 0
Original Ghidra decompilation (0x477280), from tools/pack.py 0x477280:

undefined4 FUN_00477280(void)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while( true ) {
    if (iVar1 == 0) {
      return 0xffffffff;
    }
    if (*(short *)(iVar1 + 2) != -1) break;
    iVar1 = data_iterator_next();
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
