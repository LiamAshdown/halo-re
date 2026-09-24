// game_engine_player_changed_object  (Ghidra: FUN_0045c570)
// address 0x45c570, size 286 bytes
// name confidence: 0.55 (still FUN_0045c570 in Ghidra; types/game.h's game_engine_definition
//   comment cites this exact address as the +0x18 "player_changed_object" vtable slot)
// rewrite confidence: 0.3
// evidence: types/game.h current_game_engine (0x006f1d20, player_changed_object at +0x18).
// register convention: __cdecl; the recognized stack parameter is forwarded verbatim to the
//   vtable slot without ever being read here.
//
// UNSURE: Ghidra reports eleven "Removing unreachable block" warnings and shows an empty
// while-loop body -- MSVC's optimizer proved out whatever this function used to do per object
// and left only the iterator-draining loop and the final vtable call as observable behavior.
// That is transcribed exactly: the loop still runs (for data_iterator_next's own side effects
// on the implicit iterator it advances) but its body does nothing.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; iterator elided

// Drains the (elided) data iterator -- a no-op left over from optimized-away per-object work --
// then forwards `param` to the active game engine's player_changed_object vtable slot.
void game_engine_player_changed_object(uint32_t param)
{
    player *p;

    p = (player *)data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
    while (p != (player *)0) {
        p = (player *)data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
    }

    if (current_game_engine->player_changed_object != (void *)0) {
        ((void (*)(uint32_t))current_game_engine->player_changed_object)(param);
    }
}

#if 0
Original Ghidra decompilation (0x45c570), from tools/pack.py 0x45c570:

/* WARNING: Removing unreachable block (ram,0x0045c5cd) */
/* WARNING: Removing unreachable block (ram,0x0045c5de) */
/* WARNING: Removing unreachable block (ram,0x0045c5e8) */
/* WARNING: Removing unreachable block (ram,0x0045c5fd) */
/* WARNING: Removing unreachable block (ram,0x0045c602) */
/* WARNING: Removing unreachable block (ram,0x0045c607) */
/* WARNING: Removing unreachable block (ram,0x0045c60e) */
/* WARNING: Removing unreachable block (ram,0x0045c61a) */
/* WARNING: Removing unreachable block (ram,0x0045c631) */
/* WARNING: Removing unreachable block (ram,0x0045c64b) */
/* WARNING: Removing unreachable block (ram,0x0045c662) */

void FUN_0045c570(undefined4 param_1)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    iVar1 = data_iterator_next();
  }
  if (*(code **)(DAT_006f1d20 + 0x18) != (code *)0x0) {
    (**(code **)(DAT_006f1d20 + 0x18))(param_1);
  }
  return;
}
#endif
