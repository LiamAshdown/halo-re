// game_engine_is_tracked_object_winner  (Ghidra: FUN_00463730; renamed per its summary)
// address 0x463730, size 130 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Returns whether a given tracked object should be
// treated as this round's winner, deferring to the active variant's custom win-check callback
// when one exists"); types/game.h player::team (+0x20), game_engine_definition::is_winner
// (+0x8c); types/memory.h data_iterator; this batch's game_engine_is_object_winning (0x463660).
// register convention: a team id in EBX (unaff_EBX, matched against every player's own team);
// no stack parameters.
//   // blam-cc: unaff_EBX -> team
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;                     // 0x0087a480
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern uint32_t game_engine_is_object_winning(uint32_t handle); // 0x463660, this batch

// blam-cc: unaff_EBX -> team
uint32_t game_engine_is_tracked_object_winner(int32_t team)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    for (element = data_iterator_next(&iter); element != 0; element = data_iterator_next(&iter)) {
        player *p = (player *)element;
        if (p->team == team) {
            break;
        }
    }
    if (element == 0) {
        return 0;
    }

    if (current_game_engine == 0) {
        return 0;
    }
    if (current_game_engine->is_winner == 0) {
        return game_engine_is_object_winning(iter.index);
    }
    return ((uint32_t (*)(datum_index))current_game_engine->is_winner)((datum_index)0xffffffff); // UNSURE args
}

#if 0
Original Ghidra decompilation (0x463730), from tools/pack.py 0x463730:

undefined4 FUN_00463730(void)

{
  int iVar1;
  undefined4 uVar2;
  int unaff_EBX;

  iVar1 = data_iterator_next();
  while( true ) {
    if (iVar1 == 0) {
      return 0;
    }
    if (*(int *)(iVar1 + 0x20) == unaff_EBX) break;
    iVar1 = data_iterator_next();
  }
  if (DAT_006f1d20 == 0) {
    return 0;
  }
  if (*(code **)(DAT_006f1d20 + 0x8c) == (code *)0x0) {
    uVar2 = FUN_00463660();
    return uVar2;
  }
  uVar2 = (**(code **)(DAT_006f1d20 + 0x8c))(0xffffffff);
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
