// players_active_count  (Ghidra: FUN_0045c6a0; named per out/phase4/game_functions.md,
// "Returns the number of active player datum entries.")
// address 0x45c6a0, size 73 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: out/phase4/game_functions.md summary; same data_iterator_next-with-elided-argument
//   idiom already established in cheat_get_target_object_index.c (0x45a7a0).
// register convention: __cdecl, no arguments.
//
// UNSURE: pack.py's global-reference scan lists 0x0087a480 (player_data) for this function even
// though it never appears in the decompiled body; the iterator data_iterator_next reads is
// presumably seeded to walk the player data_array by a caller-side or register-held setup this
// decompilation does not show. Transcribed as a plain count of however many elements the
// (elided) iterator yields.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; iterator elided

// Counts how many entries the (implicit) player data iterator yields.
int32_t players_active_count(void)
{
    player *p;
    int32_t count;

    count = 0;
    p = (player *)data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
    while (p != (player *)0) {
        count = count + 1;
        p = (player *)data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
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
