// chimera__hud_message  (Ghidra: chimera__hud_message, already named)
// address 0x4ae180, size 126 bytes
// name confidence: 0.65 (existing Ghidra name)   rewrite confidence: 0.6
// evidence: types/interface.h's own note: "hud_message_add @0x4ae180 does
// wcsncpy(slot + 4, text, 0x3f) ... then stamps +0x00 from game time, sets +0x84 = -1, +0x82 = 1,
// +0x83 from the rolling counter at hud_messaging + 0x465", matching hud_message_slot and
// hud_messaging_globals::next_sequence exactly.
// Phase-4 review of s2 part 2: checked against objdump 0x4ae180..0x4ae1fd; hud_message_find_slot
// takes the source in ESI (-1 here) and {record, kind 0} on the stack; the byte cleared at the
// end is prompt_changed (+0x45e), not a dirty flag.
// register convention: local player index in AX.
//   // blam-cc: local_player_index -> AX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"
#include <wchar.h>

extern hud_messaging_globals *hud_messaging; // 0x006b3a40
extern game_time_globals *game_time; // 0x006f1d6c


// blam-cc: local_player_index -> AX
// Adds a new timestamped text message into local player `local_player_index`'s HUD message slot
// array (reusing a free slot or the oldest one through hud_message_find_slot) and clears the
// reserved action line flag.
void chimera__hud_message(int16_t local_player_index, const wchar_t *text)
{
    if (local_player_index != -1) {
        hud_player_messaging_state *player_record =
            (hud_player_messaging_state *)((uint8_t *)hud_messaging + local_player_index * 0x460);
        // UNSURE: `source` (ESI) is unresolved at this call site; -1 matches the plain-message
        // source this function itself stamps into the slot right after.
        hud_message_slot *slot = hud_message_find_slot(-1, player_record, 0);

        wcsncpy((wchar_t *)slot->text, text, 0x3f);
        slot->source = -1;
        slot->timestamp = game_time->game_time;
        slot->active = 1;
        slot->sequence = hud_messaging->next_sequence;
        hud_messaging->next_sequence++;
        player_record->prompt_changed = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4ae180):

void chimera__hud_message(wchar_t *param_1)

{
  int iVar1;
  short in_AX;
  undefined4 *puVar2;
  int iVar3;

  if (in_AX != -1) {
    iVar3 = in_AX * 0x460 + DAT_006b3a40;
    puVar2 = (undefined4 *)FUN_004ae480(iVar3,0);
    _wcsncpy((wchar_t *)(puVar2 + 1),param_1,0x3f);
    iVar1 = DAT_006f1d6c;
    puVar2[0x21] = 0xffffffff;
    *puVar2 = *(undefined4 *)(iVar1 + 0xc);
    iVar1 = DAT_006b3a40;
    *(undefined1 *)((int)puVar2 + 0x82) = 1;
    *(undefined1 *)((int)puVar2 + 0x83) = *(undefined1 *)(iVar1 + 0x465);
    *(char *)(iVar1 + 0x465) = *(char *)(iVar1 + 0x465) + '\x01';
    *(undefined1 *)(iVar3 + 0x45e) = 0;
  }
  return;
}
#endif
