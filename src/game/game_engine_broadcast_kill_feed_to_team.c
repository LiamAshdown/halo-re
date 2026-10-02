// game_engine_broadcast_kill_feed_to_team  (Ghidra: FUN_00460ba0; renamed per its summary)
// address 0x460ba0, size 101 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// evidence: out/phase4/game_functions.md ("Broadcasts a kill-feed message to every entry in the
// current data iteration that belongs to a given group/team id"); types/game.h player::team
// (+0x20); types/memory.h data_iterator; the identical explicit `data_iterator iter` idiom
// already committed in game_engine_on_player_death.c for the same "drain the players array"
// shape Ghidra renders here with the iterator struct optimized away.
// register convention: a "still allowed to broadcast" gate in ESI (unaff_ESI, tested every
// iteration exactly like the sibling broadcast helpers in this same address range);
// param_1 is this function's own stack parameter (the team id to match).
//   // blam-cc: unaff_ESI -> broadcast_enabled, unaff_EBX -> ebx_broadcast, stack -> team
// UNSURE: chimera__kill_feed needs four more values (this function's own stack param_1 fixed at
// -1, plus message_type, subject and a broadcast flag) that Ghidra shows nowhere in this
// function's body -- they must be genuine pass-through registers/stack slots from this
// function's own, unrecovered caller. Modeled as three additional forwarded parameters so the
// call still compiles against chimera__kill_feed's real signature; their names are guesses.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original
// FIXED (register inputs, objdump): EBX is a genuine live-in (pushed as chimera__kill_feed's
// broadcast argument at 0x460be5) that the notes did not map; added as `ebx_broadcast`.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void chimera__kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type,
    datum_index subject, char broadcast); // 0x460a30, this batch

// FIXED 2026-09-28 from objdump 0x460ba0..0x460c04: ESI is the message type (players are skipped when it is -1),
//   BL the broadcast byte, and each matching player gets chimera__kill_feed(recipient, recipient, message, -1, BL)
//   (0x460be1..0x460bea); the earlier version treated ESI as an enable flag and forwarded invented arguments.
// blam-cc: ESI -> message_type, BL -> broadcast, stack -> team
void game_engine_broadcast_kill_feed_to_team(int32_t message_type, int32_t team, uint8_t broadcast)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    element = data_iterator_next(&iter);
    while (element != 0) {
        player *p = (player *)element;

        if (p->team == team && message_type != -1) {
            chimera__kill_feed(iter.index, (int32_t)iter.index, (uint32_t)message_type, 0xffffffff, (char)broadcast);
        }
        element = data_iterator_next(&iter);
    }
}

#if 0
Original Ghidra decompilation (0x460ba0), from tools/pack.py 0x460ba0:

void FUN_00460ba0(int param_1)

{
  int iVar1;
  int unaff_ESI;
  
  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    if ((*(int *)(iVar1 + 0x20) == param_1) && (unaff_ESI != -1)) {
      chimera__kill_feed(0xffffffff);
    }
    iVar1 = data_iterator_next();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
