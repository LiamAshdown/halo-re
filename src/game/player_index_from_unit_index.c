// player_index_from_unit_index  (Ghidra: FUN_00474db0; was player_index_from_unit_index)
// address 0x474db0, size 86 bytes
// name confidence: 0.8 (Blam's player_index_from_unit_index; the body is exactly that lookup)
// rewrite confidence: 0.9
// evidence: objdump. `mov ebx,[esp+0x18]` (after sub esp,0x10 and push ebx) is the first stack
//   argument; each iterated player is compared with `cmp [eax+0x34],ebx` (types/game.h
//   player.unit), and a match records the iterator's current index (`mov esi,[esp+0x14]`,
//   data_iterator.index). The loop does not stop at a match, so the last matching player wins;
//   ESI starts at -1 (`or esi,0xffffffff`) and is returned.
// register convention: stack -> unit_index; returns the player index in EAX. ESI is saved and
//   overwritten before use, so it is not an argument.
// FIXED: the draft missed the stack argument (Ghidra hid it) and documented the function as
//   "drain the iterator and always return -1". All 18 call sites already passed the unit and used
//   the result; with the draft, every one of them got -1 ("no player") back.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator

// Returns the index of the player whose unit is unit_index, or -1 when no player drives it.
datum_index player_index_from_unit_index(datum_index unit_index)
{
    data_iterator iter;
    player *p;
    datum_index result = (datum_index)-1;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    while ((p = (player *)data_iterator_next(&iter)) != 0) {
        if (p->unit == unit_index) {
            result = iter.index; // no break: the original keeps scanning, so the last match wins
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x474db0), from tools/pack.py 0x474db0:

undefined4 FUN_00474db0(void)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    iVar1 = data_iterator_next();
  }
  return 0xffffffff;
}
#endif
