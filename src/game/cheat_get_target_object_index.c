// cheat_get_target_object_index  (Ghidra: FUN_0045a7a0; renamed per symbols/review_queue.txt)
// address 0x45a7a0, size 86 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: types/game.h player::unit (0x34); symbols/review_queue.txt 0x45a7a0 "Finds the
//   first object datum with a valid controlling-unit index, used by the debug cheat helpers to
//   find their target object."
// register convention: no visible arguments; data_iterator_next's own iterator (EDI) is elided
//   here, so this function relies on whatever iterator a caller already set up (or a global
//   iterator this batch does not otherwise reference).
//
// UNSURE, PRESERVED AS-IS: the loop below walks every player datum looking for one with a valid
// `unit` field, but Ghidra's own decompile shows this function returning the literal constant
// -1 on EVERY path, including the one right after the loop `break`s on a match -- the found
// player's handle is never actually returned. This may be a genuine bug/stub in the original
// debug cheat code (its three callers all guard on the result being -1, so a hard-coded "not
// found" cheat helper would simply never trigger, which is plausible for tooling nobody
// finished), or a decompiler failure to track a return value through the iterator. No fix is
// applied; the always-fails behavior is transcribed exactly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; iterator elided (EDI)

// UNSURE: iterator source not identified (see header).
uint32_t cheat_get_target_object_index(void)
{
    player *p;

    p = (player *)data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
    while (1) {
        if (p == (player *)0) {
            return 0xffffffff;
        }
        if (p->unit != k_datum_index_none) {
            break;
        }
        p = (player *)data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
    }
    return 0xffffffff; // preserved as-is; see header UNSURE note
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
