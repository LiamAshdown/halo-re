// hud_chat_listbox_clear  (Ghidra: hud_chat_listbox_clear, already named)
// address 0x4ab400, size 172 bytes
// name confidence: 0.55 (existing Ghidra name, matches types/interface.h)   rewrite confidence: 0.5
// evidence: types/interface.h hud_chat_message_count (0x00719424) and hud_chat_message_expiry
// (0x006b3a20, 8 entries); string "oListbox"; shares the chat-listbox GUI table with
// hud_chat_listbox_remove_oldest.c/hud_chat_listbox_update.c.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>

extern int32_t hud_chat_message_count;     // 0x00719424
extern int32_t hud_chat_message_expiry[8]; // 0x006b3a20

extern void *chat_gui_root_handle;      // 0x00721ea4
extern chat_gui_find_object_fn chat_gui_find_object; // 0x00721eb8
extern void *chat_listbox_gui_find_object_arg;  // 0x0069c69c
extern chat_gui_find_child_fn chat_gui_find_child;    // 0x00721ecc
extern chat_gui_set_property_int_fn chat_gui_set_property_int; // 0x00721ee8
extern chat_gui_finalize_fn chat_gui_finalize;  // 0x00721ed0
extern chat_gui_release_fn chat_gui_release;    // 0x00721ec8

// Removes every row from the chat listbox GUI control (if one exists) and resets the module's
// own message-count and expiry-timestamp bookkeeping.
void hud_chat_listbox_clear(void)
{
    if (chat_gui_find_object != 0) {
        void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
        if (gui_object != 0) {
            void *listbox = chat_gui_find_child(gui_object, (const uint16_t *)L"oListbox");
            if (listbox != 0) {
                while (hud_chat_message_count != 0 &&
                       (int32_t)chat_gui_set_property_int(listbox, 0x182, 0, 0) > 0) {
                    hud_chat_message_count = hud_chat_message_count - 1;
                }
                chat_gui_finalize(gui_object);
            }
            chat_gui_release(gui_object);
        }
    }

    {
        int32_t i;
        for (i = 0; i < 8; i = i + 1) {
            hud_chat_message_expiry[i] = 0;
        }
    }
    hud_chat_message_count = 0;
}

#if 0
Original Ghidra decompilation (0x4ab400):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl hud_chat_listbox_clear(void)

{
  int iVar1;
  int iVar2;
  int iVar3;

  if ((DAT_00721eb8 != (code *)0x0) &&
     (iVar1 = (*DAT_00721eb8)(DAT_00721ea4,PTR_PTR_0069c69c), iVar1 != 0)) {
    iVar2 = (*DAT_00721ecc)(iVar1,L"oListbox");
    if (iVar2 != 0) {
      while ((DAT_00719424 != 0 && (iVar3 = (*DAT_00721ee8)(iVar2,0x182,0,0), 0 < iVar3))) {
        DAT_00719424 = DAT_00719424 + -1;
      }
      (*DAT_00721ed0)(iVar1);
    }
    (*DAT_00721ec8)(iVar1);
  }
  DAT_006b3a20 = 0;
  _DAT_006b3a24 = 0;
  _DAT_006b3a28 = 0;
  _DAT_006b3a2c = 0;
  _DAT_006b3a30 = 0;
  _DAT_006b3a34 = 0;
  _DAT_006b3a38 = 0;
  DAT_00719424 = 0;
  _DAT_006b3a3c = 0;
  return;
}
#endif
