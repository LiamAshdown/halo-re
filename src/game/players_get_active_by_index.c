// players_get_active_by_index  (Ghidra: FUN_0045c6f0; named per out/phase4/game_functions.md,
// "Returns the player datum at a given index among active players.")
// address 0x45c6f0, size 91 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md summary; register convention per functions.md
//   ("register arg in_AX = player index" -- widened to EAX in the disassembly).
// register convention: index in EAX (in_EAX).
//   // blam-cc: EAX -> index
//
// The iterator is the inline types/memory.h data_iterator over player_data (0x45c6f7..0x45c718:
// data, WORD next_index = 0, index = -1, signature = data ^ 'iter'); Ghidra lost it because it
// lives on the stack and is passed in EDI. Found path 0x45c740 returns [esp+0x14] = the
// iterator's index (the player handle); the exhausted path 0x45c737 returns -1 (EBX).
// reconciled: R16 the elided iterator is the inline 0x10-byte data_iterator over player_data (0x45c6f7); the found path returns iterator.index (0x45c740), not -1

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; blam-cc: EDI -> iterator

// Returns the handle of the index-th live player datum, or -1 when there are fewer.
uint32_t players_get_active_by_index(int32_t index)
    // blam-cc: EAX -> index
{
    data_iterator iterator;
    player *p;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (index == 0) {
            return iterator.index;
        }
        index = index - 1;
        p = (player *)data_iterator_next(&iterator);
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x45c6f0), from tools/pack.py 0x45c6f0:

undefined4 FUN_0045c6f0(void)

{
  int in_EAX;
  int iVar1;

  iVar1 = data_iterator_next();
  while( true ) {
    if (iVar1 == 0) {
      return 0xffffffff;
    }
    if (in_EAX == 0) break;
    in_EAX = in_EAX + -1;
    iVar1 = data_iterator_next();
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
