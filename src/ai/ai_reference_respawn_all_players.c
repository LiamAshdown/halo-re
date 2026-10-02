// ai_reference_respawn_all_players  (Ghidra: ai_reference_respawn_all_players; named for this rewrite)
// address 0x432d90, size 82 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: objdump (bin/halo.exe 0x432d90..0x432de1) shows the inline data_iterator this
// function builds on the stack is seeded with player_data (ds:0x87a480), not actor_data --
// Ghidra's decompile shows it only as an opaque `data_iterator_next()` because it never
// recognized the stack layout as a data_iterator at all. For every player, calls
// ai_reference_respawn_member (0x432df0, this batch) with that player's unit (player+0x34,
// types/game.h player.unit).
// register convention: confirmed by objdump: ESI -> packed_reference.
//   // blam-cc: ESI -> packed_reference
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480, stride 0x200 (no types/players.h yet)

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void ai_reference_respawn_member(uint32_t packed_reference, datum_index unit_index); // 0x432df0, this batch

// blam-cc: ESI -> packed_reference
void ai_reference_respawn_all_players(uint32_t packed_reference)
{
    if (packed_reference != (uint32_t)k_datum_index_none) {
        data_iterator iterator;
        player *p;

        iterator.data = player_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

        p = (player *)data_iterator_next(&iterator);
        while (p != 0) {
            ai_reference_respawn_member(packed_reference, p->unit);
            p = (player *)data_iterator_next(&iterator);
        }
    }
}

#if 0
Original Ghidra decompilation (0x432d90):

void FUN_00432d90(void)

{
  int iVar1;
  int unaff_ESI;

  if (unaff_ESI != -1) {
    iVar1 = data_iterator_next();
    while (iVar1 != 0) {
      FUN_00432df0();
      iVar1 = data_iterator_next();
    }
  }
  return;
}

Real disassembly (0x432d90-0x432de1), used to recover the player_data iterator and the
FUN_00432df0 arguments:

00432d98: mov    eax,ds:0x87a480    ; player_data
...                                  ; inline data_iterator construction
00432dbd: call   0x4d05d0           ; data_iterator_next
00432dc6: mov    edi,[eax+0x34]     ; player.unit
00432dc9: mov    eax,esi            ; packed_reference
00432dcb: call   0x432df0
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
