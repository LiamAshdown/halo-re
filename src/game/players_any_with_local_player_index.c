// players_any_with_local_player_index  (Ghidra: FUN_004736d0; named per this rewrite)
// address 0x4736d0, size 88 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: objdump -d -M intel --start-address=0x4736d0 --stop-address=0x473730 bin/halo.exe --
//   the loop dereferences "movsx eax,WORD PTR [eax+0x2]" on every player_data element and
//   compares it against ESI, and player+0x02 is types/game.h player::local_player_index, not
//   player::team (+0x20). out/phase4/game_functions.md guessed "checks whether any player
//   belongs to a given team" (conf 0.4); that is CORRECTED here against the disassembly.
// register convention: ESI -> local_player_index (the only caller, this batch's
//   local_player_find_free_slot_index at 0x473730, loads it straight into ESI with no visible
//   mov ahead of the call, so Ghidra shows it as unaff_ESI).
//   // blam-cc: ESI -> local_player_index
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator

// Returns true if any live player currently has the given local_player_index (only ever 0 or
// -1 in this build, since k_maximum_local_players is 1).
uint8_t players_any_with_local_player_index(int16_t local_player_index)
    // blam-cc: ESI -> local_player_index
{
    data_iterator iter;
    player *p;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    p = (player *)data_iterator_next(&iter);
    while (p != (player *)0) {
        if (p->local_player_index == local_player_index) {
            return 1;
        }
        p = (player *)data_iterator_next(&iter);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4736d0), from tools/pack.py 0x4736d0:

undefined4 FUN_004736d0(void)

{
  int iVar1;
  int unaff_ESI;

  iVar1 = data_iterator_next();
  while( true ) {
    if (iVar1 == 0) {
      return 0;
    }
    if (*(short *)(iVar1 + 2) == unaff_ESI) break;
    iVar1 = data_iterator_next();
  }
  return CONCAT31((int3)(char)((ushort)*(short *)(iVar1 + 2) >> 8),1);
}
#endif
