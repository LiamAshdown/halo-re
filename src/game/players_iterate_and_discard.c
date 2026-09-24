// players_iterate_and_discard  (Ghidra: FUN_00474db0; named per this rewrite)
// address 0x474db0, size 86 bytes
// name confidence: 0.25   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Iterates all players; the visible decompiled
//   behavior always returns -1, so its true per-call purpose (given 18 call sites) is not fully
//   recoverable from this view", conf 0.25). The loop body is genuinely empty in the
//   disassembly, not merely in Ghidra's rendering (checked with objdump); this transcribes
//   exactly what the compiled code does: drain the player data_iterator and always return the
//   wildcard.
// register convention: no arguments; return value in EAX.
//
// UNSURE: with 18 call sites all apparently only using the return value (always k_datum_index_none),
// this function's real historical purpose is not recoverable here; it may be dead code left over
// from a removed feature, or the visible behavior may be incomplete due to whole-program
// optimization proving some other branch unreachable in this particular retail build.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator

// Drains the player iterator (for its side effects on the implicit iterator only) and always
// returns the datum-index wildcard.
datum_index players_iterate_and_discard(void)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;

    element = data_iterator_next(&iter);
    while (element != (void *)0) {
        element = data_iterator_next(&iter);
    }
    return (datum_index)-1;
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
