// hud_display_loading_message  (Ghidra: FUN_004aa2a0, renamed)
// address 0x4aa2a0, size 101 bytes
// name confidence: 0.35 (chosen; the phase-4 auto-summary calls this a "reload/no-ammo"
// message, but the tag offsets are HUDGlobals::loading_begin_text/loading_end_text, see below)
// rewrite confidence: 0.5
// evidence: types/interface.h hud_globals_tag_data (0x0071941c) and hud_messaging_globals
// (players[0].messages[i].active at +0x82, stride 0x8c, matching hud_message_slot exactly);
// types/tags.h HUDGlobals::loading_begin_text/loading_end_text sit at offsets 0x3d8/0x3da once
// the struct is sized out to carnage_report_bitmap, matching the `+0x3d8`/`+0x3da` reads here
// exactly.
// register convention: bool flag in AL (in_AL, unresolved register read); AL nonzero selects
// loading_begin_text, matching hud_display_checkpoint_message.c's DL polarity.
//   // blam-cc: is_begin -> AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern hud_messaging_globals *hud_messaging; // 0x006b3a40

extern uint16_t *hud_get_message_string(int32_t message_index); // 0x4aa3f0
extern void chimera__hud_message(int16_t local_player_index, const uint16_t *text); // 0x4ae180, blam-cc: AX local_player_index
extern player_globals *local_player_globals; // 0x0087a478

// blam-cc: is_begin -> AL
// Clears the local player's 4 message-slot active flags, then displays the HUDGlobals loading
// begin/end text (picked by is_begin, see header note) if a string is configured for it.
void hud_display_loading_message(uint8_t is_begin)
{
    HUDGlobals *hud_globals = (HUDGlobals *)hud_globals_tag_data;
    int16_t message_index = is_begin ? hud_globals->loading_begin_text : hud_globals->loading_end_text;
    int32_t i;

    for (i = 0; i < 4; i = i + 1) {
        hud_messaging->players[0].messages[i].active = 0;
    }

    if (message_index != -1) {
        // s2 part 2 review: AX is 0 when local player 0 is set, else -1 (objdump 0x4aa2dc..0x4aa2f9)
        chimera__hud_message(local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1,
                             hud_get_message_string(message_index));
    }
}

#if 0
Original Ghidra decompilation (0x4aa2a0):

void FUN_004aa2a0(void)

{
  char in_AL;
  undefined1 *puVar1;
  undefined4 uVar2;
  int iVar3;
  short sVar4;

  if (in_AL == '\0') {
    sVar4 = *(short *)(DAT_0071941c + 0x3da);
  }
  else {
    sVar4 = *(short *)(DAT_0071941c + 0x3d8);
  }
  puVar1 = (undefined1 *)(DAT_006b3a40 + 0x82);
  iVar3 = 4;
  do {
    *puVar1 = 0;
    puVar1 = puVar1 + 0x8c;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  if (sVar4 != -1) {
    uVar2 = hud_get_message_string();
    chimera__hud_message(uVar2);
  }
  return;
}
#endif
