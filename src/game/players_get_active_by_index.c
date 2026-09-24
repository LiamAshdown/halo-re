// players_get_active_by_index  (Ghidra: FUN_0045c6f0; named per out/phase4/game_functions.md,
// "Returns the player datum at a given index among active players.")
// address 0x45c6f0, size 91 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md summary; register convention per functions.md
//   ("register arg in_AX = player index" -- widened to EAX in the disassembly).
// register convention: index in EAX (in_EAX).
//   // blam-cc: EAX -> index
//
// UNSURE, PRESERVED AS-IS: exactly like cheat_get_target_object_index.c (0x45a7a0), Ghidra's own
// decompilation has a single `return 0xffffffff;` shared by both the "iterator exhausted" path
// and the "found the requested index" (`break`) path -- the player pointer the loop actually
// walks to is never returned on any path. This is transcribed literally rather than "fixed" to
// return the found player, matching the precedent set for the sibling function.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; iterator elided

// UNSURE: always returns -1; see header note. `index` is consumed by counting down while the
// (implicit) player iterator still has entries.
uint32_t players_get_active_by_index(int32_t index)
    // blam-cc: EAX -> index
{
    player *p;

    p = (player *)data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
    while (1) {
        if (p == (player *)0) {
            return 0xffffffff;
        }
        if (index == 0) {
            break;
        }
        index = index - 1;
        p = (player *)data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
    }
    return 0xffffffff; // preserved as-is; see header UNSURE note
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
