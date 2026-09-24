// chat_poll_hotkeys  (Ghidra: FUN_004aaa90, renamed)
// address 0x4aaa90, size 102 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.45
// evidence: phase-4 summary "Polls the chat hotkey action flags each frame, opening the chat
// dialog in the requested scope, and updates the chat message listbox"; reuses chimera__chat_open
// and hud_chat_listbox_update.
// UNSURE: the three hotkey flag bytes (0x007124a7/a8/a9) and the "chat busy" gate 0x006b7020
// are not documented anywhere in this pass.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint8_t chat_busy;          // 0x006b7020, UNSURE name (see chimera__chat_open.c)
extern uint8_t chat_dialog_open;   // 0x006b3858
extern uint8_t chat_hotkey_all;    // 0x007124a7, UNSURE name
extern uint8_t chat_hotkey_team;   // 0x007124a8, UNSURE name
extern uint8_t chat_hotkey_vehicle; // 0x007124a9, UNSURE name

extern void chimera__chat_open(int32_t chat_scope); // 0x4aa700
extern void hud_chat_listbox_update(void); // 0x4ab300, the caller reloads AL from 0x006b3858 afterwards

// Opens the chat dialog for whichever scope hotkey is set (all, team, vehicle, checked in that
// order), then always refreshes the chat message listbox.
uint8_t chat_poll_hotkeys(void)
{
    if (chat_busy == 0) {
        if (chat_hotkey_all == 1) {
            chimera__chat_open(0);
            hud_chat_listbox_update();
            return chat_dialog_open;
        }
        if (chat_hotkey_team == 1) {
            chimera__chat_open(1);
            hud_chat_listbox_update();
            return chat_dialog_open;
        }
        if (chat_hotkey_vehicle == 1) {
            chimera__chat_open(2);
        }
    }
    hud_chat_listbox_update();
    return chat_dialog_open;
}

#if 0
Original Ghidra decompilation (0x4aaa90):

undefined1 FUN_004aaa90(void)

{
  if (DAT_006b7020 == '\0') {
    if (DAT_007124a7 == '\x01') {
      chimera__chat_open(0);
      hud_chat_listbox_update();
      return DAT_006b3858;
    }
    if (DAT_007124a8 == '\x01') {
      chimera__chat_open(1);
      hud_chat_listbox_update();
      return DAT_006b3858;
    }
    if (DAT_007124a9 == '\x01') {
      chimera__chat_open(2);
    }
  }
  hud_chat_listbox_update();
  return DAT_006b3858;
}
#endif
