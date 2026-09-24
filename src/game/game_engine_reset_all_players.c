// game_engine_reset_all_players  (Ghidra: FUN_0045b8b0; renamed per symbols/review_queue.txt)
// address 0x45b8b0, size 98 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: symbols/review_queue.txt 0x45b8b0 "iterates the player datum pool calling
//   game_engine_player_new_life (FUN_0045c440) for each, then dispatches through the current
//   game-engine vtable at offset 0x20 (reset_round, types/game.h)".
// register convention: no arguments.
//
// UNSURE: transcribed exactly as decompiled, including calling game_engine_player_new_life with
// the literal constant player index 0xffffffff on every iteration (never the iterated player's
// own index) -- game_engine_player_new_life (0x45c440, this batch) itself also calls
// data_iterator_next() with no visible iterator, so it is plausible both functions share one
// global iterator and this outer loop's real purpose is to drive that shared iterator forward a
// matching number of times rather than to reset each player individually; not resolved further.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data; // 0x0087a480
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; iterator elided
extern void game_engine_player_new_life(uint32_t player_index); // this batch, 0x45c440

// Iterates the player datum pool calling game_engine_player_new_life once per entry, then notifies
// the active multiplayer game engine's reset_round callback.
void game_engine_reset_all_players(void)
{
    uint32_t cookie;
    void *entry;

    cookie = (uint32_t)(uint8_t *)player_data ^ 0x69746572; // "iter" -- UNSURE purpose, see #if 0

    entry = data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
    while (entry != (void *)0) {
        game_engine_player_new_life(0xffffffff); // UNSURE: see header
        entry = data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
    }

    if (current_game_engine != (game_engine_definition *)0 && current_game_engine->reset_round != (void *)0) {
        ((void (*)(data_array *, uint16_t, uint32_t, uint32_t))current_game_engine->reset_round)(
            player_data, 0, 0xffffffff, cookie);
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
