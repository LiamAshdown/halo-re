// players_active_count  (Ghidra: FUN_0045c6a0; named per out/phase4/game_functions.md,
// "Returns the number of active player datum entries.")
// address 0x45c6a0, size 73 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: out/phase4/game_functions.md summary; same data_iterator_next-with-elided-argument
//   idiom already established in cheat_get_target_object_index.c (0x45a7a0).
// register convention: __cdecl, no arguments.
//
// The iterator is the inline types/memory.h data_iterator over player_data (0x45c6a3..0x45c6c6:
// data, WORD next_index = 0, index = -1, signature = data ^ 'iter'), a stack object passed in
// EDI that Ghidra's decompile lost; hence the player_data reference pack.py reports.
// reconciled: R16 the elided iterator is the inline 0x10-byte data_iterator over player_data (0x45c6a3)

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

// Counts the live player datums.
int32_t players_active_count(void)
{
    data_iterator iterator;
    player *p;
    int32_t count;

    count = 0;
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)data_iterator_next(&iterator);
    while (p != (player *)0) {
        count = count + 1;
        p = (player *)data_iterator_next(&iterator);
    }
    return count;
}

#if 0
Original Ghidra decompilation (0x45c6a0), from tools/pack.py 0x45c6a0:

int FUN_0045c6a0(void)

{
  int iVar1;
  int iVar2;

  iVar2 = 0;
  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    iVar2 = iVar2 + 1;
    iVar1 = data_iterator_next();
  }
  return iVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
