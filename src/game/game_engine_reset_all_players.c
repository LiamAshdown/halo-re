// game_engine_reset_all_players  (Ghidra: FUN_0045b8b0; renamed per symbols/review_queue.txt)
// address 0x45b8b0, size 98 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: symbols/review_queue.txt 0x45b8b0 "iterates the player datum pool calling
//   game_engine_player_new_life (FUN_0045c440) for each, then dispatches through the current
//   game-engine vtable at offset 0x20 (reset_round, types/game.h)".
// register convention: no arguments.
//
// The iterator is the inline types/memory.h data_iterator over player_data (0x45b8b3..0x45b8d4:
// data, WORD next_index = 0, index = -1, signature = data ^ 'iter'), which Ghidra lost because it
// is a stack object passed in EDI. Each iteration pushes [esp+0xc], the iterator's index, to
// game_engine_player_new_life (0x45b8e1); Ghidra's constant -1 was the index's initial value.
// reset_round is called with no arguments (0x45b90c `call eax`, nothing pushed); the
// (player_data, 0, -1, cookie) Ghidra showed were the iterator's own stack slots.
// reconciled: R16 the elided iterator is the inline 0x10-byte data_iterator over player_data (0x45b8b3); new_life gets iterator.index, reset_round takes no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; blam-cc: EDI -> iterator
extern void game_engine_player_new_life(uint32_t player_index); // this batch, 0x45c440

// Iterates the player datum pool calling game_engine_player_new_life once per entry, then notifies
// the active multiplayer game engine's reset_round callback.
void game_engine_reset_all_players(void)
{
    data_iterator iterator;
    void *entry;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    entry = data_iterator_next(&iterator);
    while (entry != (void *)0) {
        game_engine_player_new_life(iterator.index);
        entry = data_iterator_next(&iterator);
    }

    if (current_game_engine != (game_engine_definition *)0 && current_game_engine->reset_round != (void *)0) {
        ((void (*)(void))current_game_engine->reset_round)();
    }
}

#if 0
Original Ghidra decompilation (0x45b8b0), from tools/pack.py 0x45b8b0:

void FUN_0045b8b0(void)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  undefined2 uVar4;
  undefined4 uVar5;

  uVar1 = DAT_0087a480 ^ 0x69746572;
  uVar4 = 0;
  uVar5 = 0xffffffff;
  uVar3 = DAT_0087a480;
  iVar2 = data_iterator_next();
  while (iVar2 != 0) {
    FUN_0045c440(uVar5);
    iVar2 = data_iterator_next();
  }
  if ((DAT_006f1d20 != 0) && (*(code **)(DAT_006f1d20 + 0x20) != (code *)0x0)) {
    (**(code **)(DAT_006f1d20 + 0x20))(uVar3,uVar4,uVar5,uVar1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
