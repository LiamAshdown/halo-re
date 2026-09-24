// game_engine_announce_time_remaining  (Ghidra: FUN_0045cae0; named per
// out/phase4/game_functions.md, "Periodically broadcasts a 'time remaining' kill-feed style
// announcement to all players once the game's remaining time crosses configured milestones.")
// address 0x45cae0, size 217 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: current_game_engine (0x006f1d20), network_game_mode (0x00719720),
//   game_engine_players_ready_for_bsp_switch_strict (0x45c830, this batch),
//   game_engine_get_time_remaining (0x45cab0, this batch).
// register convention: __cdecl, no arguments.
//
// UNSURE: the gate on game_engine_players_ready_for_bsp_switch_strict() returning false
// (early "return 1") is transcribed exactly, but why a time-remaining announcement would be
// gated on structure-BSP switch readiness is not understood.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern int16_t network_game_mode;                   // 0x00719720
extern data_array *player_data;                     // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module;
    // blam-cc: EDI -> iterator (matches src/memory/data_iterator_next.c)
extern uint8_t game_engine_players_ready_for_bsp_switch_strict(void); // 0x45c830, this batch
extern int32_t game_engine_get_time_remaining(void);                  // 0x45cab0, this batch
extern void chimera__kill_feed(datum_index recipient, int32_t param_1, uint32_t message_type,
    datum_index subject, char broadcast); // 0x460a30, this module; blam-cc: EDI -> recipient,
    // stack -> param_1, message_type, subject, broadcast

int32_t game_engine_announce_time_remaining(void)
{
    uint8_t ready;
    int32_t time_remaining;
    int32_t interval;
    data_iterator iterator;
    uint32_t unused_checksum; // UNSURE: written, never read back -- the 4th iterator dword
    player *p;

    if (current_game_engine == (game_engine_definition *)0) {
        return 0;
    }
    if (network_game_mode != 2) {
        return 0;
    }
    ready = game_engine_players_ready_for_bsp_switch_strict();
    if (ready == 0) {
        return 1;
    }

    time_remaining = game_engine_get_time_remaining();
    if (time_remaining == -1) {
        return 0;
    }
    if (time_remaining == 0) {
        return 1;
    }

    if (time_remaining < 0x97) {
        interval = 0x1e;
    } else if (time_remaining == 900) {
        goto announce;
    } else {
        interval = (8999 < time_remaining) ? 9000 : 0x708;
    }

    if (time_remaining % interval != 0) {
        return 0;
    }

announce:
    // CORRECTED (phase 4 review, objdump 0x45cb64..0x45cba0): Ghidra folds the iterator's
    // `index` field to its initial -1 and so prints `chimera__kill_feed(0xffffffff, ...)`, but
    // the loop reloads EDI from iterator+0x08 on every pass (`mov edi,[esp+0x14]`) and pushes
    // that same handle as the first stack argument, so both `recipient` (EDI) and `param_1`
    // are the player the iterator just returned -- not -1.
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)0xffffffff;
    unused_checksum = (uint32_t)player_data ^ 0x69746572;

    p = (player *)data_iterator_next(&iterator);
    while (p != (player *)0) {
        chimera__kill_feed(iterator.index, (int32_t)iterator.index, 0x1e,
                            (datum_index)time_remaining, 1);
        p = (player *)data_iterator_next(&iterator);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x45cae0), from tools/pack.py 0x45cae0:

undefined4 FUN_0045cae0(void)

{
  char cVar1;
  int iVar2;
  int iVar3;

  if (DAT_006f1d20 == 0) {
    return 0;
  }
  if (DAT_00719720 != 2) {
    return 0;
  }
  cVar1 = FUN_0045c830();
  if (cVar1 == '\0') {
    return 1;
  }
  iVar2 = FUN_0045cab0();
  if (iVar2 == -1) {
    return 0;
  }
  if (iVar2 == 0) {
    return 1;
  }
  if (iVar2 < 0x97) {
    iVar3 = 0x1e;
  }
  else {
    if (iVar2 == 900) goto LAB_0045cb64;
    iVar3 = 0x708;
    if (8999 < iVar2) {
      iVar3 = 9000;
    }
  }
  if (iVar2 % iVar3 != 0) {
    return 0;
  }
LAB_0045cb64:
  iVar3 = data_iterator_next();
  while (iVar3 != 0) {
    chimera__kill_feed(0xffffffff,0x1e,iVar2,1);
    iVar3 = data_iterator_next();
  }
  return 0;
}
#endif
