// cheat_get_target_object_index  (Ghidra: FUN_0045a7a0; renamed per symbols/review_queue.txt)
// address 0x45a7a0, size 86 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: types/game.h player::unit (0x34); symbols/review_queue.txt 0x45a7a0 "Finds the
//   first object datum with a valid controlling-unit index, used by the debug cheat helpers to
//   find their target object."
// register convention: no arguments. data_iterator_next's iterator (EDI) is the inline
//   types/memory.h data_iterator over player_data built at 0x45a7a3..0x45a7c5 (data, WORD
//   next_index = 0, index = -1, signature = data ^ 'iter'); Ghidra lost it because it is a
//   stack object passed in a register.
// The found path (0x45a7ec) returns [esp+0x10], the iterator's index, i.e. the handle of the
//   first player whose unit (+0x34) is not -1; the exhausted path returns -1 (ESI). Ghidra's
//   "always returns -1" was the lost iterator, not a stub. The name is kept from the review
//   queue although the result is a player handle.
// reconciled: R16 the elided iterator is the inline 0x10-byte data_iterator over player_data (0x45a7a3); the found path returns iterator.index (0x45a7ec), not -1

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

uint32_t cheat_get_target_object_index(void)
{
    data_iterator iterator;
    player *p;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (p->unit != k_datum_index_none) {
            return iterator.index;
        }
        p = (player *)data_iterator_next(&iterator);
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x45a7a0), from tools/pack.py 0x45a7a0:

undefined4 FUN_0045a7a0(void)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while( true ) {
    if (iVar1 == 0) {
      return 0xffffffff;
    }
    if (*(int *)(iVar1 + 0x34) != -1) break;
    iVar1 = data_iterator_next();
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
