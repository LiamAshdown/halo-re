// hud_message_broadcast_to_local_players  (Ghidra: FUN_00495f50, unnamed)
// address 0x495f50, size 86 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/interface_functions.md "Walks a generic data-array (register-passed) and
// dispatches chimera__hud_message() for every entry w[hose local_player_index is set]"; the
// tested field (element + 2, != -1) matches types/game.h player::local_player_index exactly, and
// the referenced global (0x0087a480) is player_data, confirming the array walked is the player
// list; local_player_index_for_unit.c's precedent name/type for player_data.
// register convention: none (void); player_data is walked directly via a local data_iterator.
// s2 part 2 review: the text comes in ESI and is pushed as the stack argument of
// chimera__hud_message (0x4ae180), whose AX is the player local_player_index (objdump
// 0x495f81..0x495f8c); player_profile_save_495fb0 passes the saved message or L"" (0x660c34).
//   // blam-cc: text -> ESI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern data_array *player_data; // 0x0087a480, "players"

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void chimera__hud_message(int16_t local_player_index, const uint16_t *text); // 0x4ae180, blam-cc: AX local_player_index

// Posts a HUD message (via chimera__hud_message) once for every local player in player_data.
void hud_message_broadcast_to_local_players(const uint16_t *text)
{
    data_iterator iterator;
    player *record;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)0xffffffff;

    record = (player *)data_iterator_next(&iterator);
    while (record != (player *)0) {
        if (record->local_player_index != -1) {
            chimera__hud_message(record->local_player_index, text);
        }
        record = (player *)data_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x495f50):

void FUN_00495f50(void)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    if (*(short *)(iVar1 + 2) != -1) {
      chimera__hud_message();
    }
    iVar1 = data_iterator_next();
  }
  return;
}
#endif
