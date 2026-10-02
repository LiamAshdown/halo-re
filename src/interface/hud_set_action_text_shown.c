// hud_set_action_text_shown  (Ghidra: FUN_004ae110, renamed in the phase-4 review)
// address 0x4ae110, size 102 bytes
// name confidence: 0.5 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4ae110..0x4ae175. Raises prompt_changed when message_shown flips, stores
// the new state in message_shown and message_shown_copy, clears the message pointer and, when
// turning the line on, empties action_text (wcsncpy of the empty wide string at 0x00660c34 with
// limit 0xff; 0x627a94 is wcsncpy). hud_update_interaction_prompt then writes the text.
// register convention: EAX local player index, BL shown.
//   // blam-cc: local_player_index -> EAX, shown -> BL

#include <wchar.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hud_messaging_globals *hud_messaging;  // 0x006b3a40

// blam-cc: local_player_index -> EAX, shown -> BL
void hud_set_action_text_shown(int16_t local_player_index, uint8_t shown)
{
    static const uint16_t empty_text[1] = {0}; // 0x00660c34
    hud_player_messaging_state *record = &hud_messaging->players[local_player_index];

    record->prompt_changed |= (uint8_t)(record->message_shown != shown);
    record->message_shown = shown;
    record->message = 0;
    if (shown != 0) {
        record->message = 0;
        wcsncpy((wchar_t *)record->action_text, (const wchar_t *)empty_text, 0xff); // 0x627a94
    }
    record->message_shown_copy = shown;
}

#if 0
Original Ghidra decompilation (0x4ae110):

void FUN_004ae110(void)

{
  short in_AX;
  char unaff_BL;
  int iVar1;
  int iVar2;

  iVar1 = in_AX * 0x460;
  iVar2 = iVar1 + DAT_006b3a40;
  *(byte *)(iVar2 + 0x45e) =
       *(byte *)(iVar1 + 0x45e + DAT_006b3a40) | *(char *)(iVar1 + 0x458 + DAT_006b3a40) != unaff_BL
  ;
  *(char *)(iVar2 + 0x458) = unaff_BL;
  *(undefined4 *)(iVar2 + 0x454) = 0;
  if (unaff_BL != '\0') {
    *(undefined4 *)(iVar2 + 0x454) = 0;
    _wcsncpy((wchar_t *)(iVar2 + 0x230),L"",0xff);
  }
  *(char *)(iVar2 + 0x45f) = unaff_BL;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
