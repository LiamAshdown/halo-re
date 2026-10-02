// game_engine_multiplayer_sound_queue_tick  (Ghidra: game_engine_multiplayer_sound_queue_tick,
//   already named)
// address 0x46bd80, size 84 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/game.h multiplayer_sound_request/multiplayer_sound_queue_count; game_engine_
//   play_multiplayer_sound.c (0x46bd00, this batch).
// register convention: no parameters.
// UNSURE: the head entry's own broadcast flag (queue[0].broadcast) is forwarded to game_engine_
//   play_multiplayer_sound as its `broadcast` stack argument, per Ghidra's own
//   `FUN_0046bd00(DAT_006b10fc)`; the sound_index/recipient_player arguments that function also
//   needs are not visible at this call site either and are modeled as the new head entry's own
//   sound_index and "no recipient".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t multiplayer_sound_queue_count; // 0x006b1140
extern multiplayer_sound_request multiplayer_sound_queue[k_maximum_queued_multiplayer_sounds]; // 0x006b10f0

extern void game_engine_play_multiplayer_sound(int32_t sound_index, datum_index recipient_player,
    uint8_t broadcast); // 0x46bd00, this batch

// Counts down the head queue entry's remaining_ticks; once it expires, shifts the rest of the
// queue down by one slot, and if anything is left, starts playing the new head entry.
void game_engine_multiplayer_sound_queue_tick(void)
{
    if (multiplayer_sound_queue_count != 0) {
        multiplayer_sound_queue[0].remaining_ticks--;
        if (multiplayer_sound_queue[0].remaining_ticks == 0) {
            if (multiplayer_sound_queue_count > 1) {
                int32_t i;
                for (i = 1; i < multiplayer_sound_queue_count; i++) {
                    multiplayer_sound_queue[i - 1] = multiplayer_sound_queue[i];
                }
            }
            multiplayer_sound_queue_count--;
            if (multiplayer_sound_queue_count != 0) {
                game_engine_play_multiplayer_sound(multiplayer_sound_queue[0].sound_index,
                    (datum_index)0xffffffff, multiplayer_sound_queue[0].broadcast);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x46bd80), from tools/pack.py 0x46bd80:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl game_engine_multiplayer_sound_queue_tick(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;

  if ((DAT_006b1140 != 0) && (_DAT_006b10f8 = _DAT_006b10f8 + -1, _DAT_006b10f8 == 0)) {
    if (1 < DAT_006b1140) {
      puVar2 = &DAT_006b1100;
      puVar3 = &DAT_006b10f0;
      for (iVar1 = (DAT_006b1140 - 1U & 0xfffffff) << 2; iVar1 != 0; iVar1 = iVar1 + -1) {
        *puVar3 = *puVar2;
        puVar2 = puVar2 + 1;
        puVar3 = puVar3 + 1;
      }
    }
    DAT_006b1140 = DAT_006b1140 + -1;
    if (DAT_006b1140 != 0) {
      FUN_0046bd00(DAT_006b10fc);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
