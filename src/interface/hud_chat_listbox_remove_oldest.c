// hud_chat_listbox_remove_oldest  (Ghidra: hud_chat_listbox_remove_oldest, already named)
// address 0x4ab240, size 177 bytes
// name confidence: 0.5 (existing Ghidra name, matches types/interface.h)   rewrite confidence: 0.45
// evidence: types/interface.h hud_chat_message_expiry[8] (0x006b3a20) and hud_chat_message_count
// (0x00719424); string "oListbox"; drives the same embedded GUI library table as
// chimera__chat_open.c, but against a different root argument (0x0069c69c vs chat's 0x0069c698),
// consistent with the header's note that the chat listbox is a separate "oListbox" control.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t hud_chat_message_count;       // 0x00719424
extern int32_t hud_chat_message_expiry[8];   // 0x006b3a20

extern void *chat_gui_root_handle;      // 0x00721ea4
extern chat_gui_find_object_fn chat_gui_find_object; // 0x00721eb8
extern void *chat_listbox_gui_find_object_arg;  // 0x0069c69c
extern chat_gui_find_child_fn chat_gui_find_child;    // 0x00721ecc
extern chat_gui_set_property_int_fn chat_gui_set_property_int; // 0x00721ee8
extern chat_gui_finalize_fn chat_gui_finalize;  // 0x00721ed0
extern chat_gui_release_fn chat_gui_release;    // 0x00721ec8

// Removes the row 0 entry from the chat listbox GUI control (if one exists) and shifts the
// hud_chat_message_expiry timestamps down by one, returning whatever the GUI's own row-removal
// property call returned (0 if the control could not be reached at all).
uint32_t hud_chat_listbox_remove_oldest(void)
{
    uint32_t result = 0;

    if (hud_chat_message_count > 0 && chat_gui_find_object != 0) {
        void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
        if (gui_object != 0) {
            void *listbox = chat_gui_find_child(gui_object, (const uint16_t *)L"oListbox");
            if (listbox != 0) {
                result = chat_gui_set_property_int(listbox, 0x182, 0, 0);
                chat_gui_set_property_int(listbox, 0x115, 2, 0);
                chat_gui_finalize(gui_object);
            }
            chat_gui_release(gui_object);
        }
    }

    hud_chat_message_count = hud_chat_message_count - 1;
    memmove(hud_chat_message_expiry, hud_chat_message_expiry + 1, hud_chat_message_count * 4);
    hud_chat_message_expiry[hud_chat_message_count] = 0;
    return result;
}

#if 0
Original Ghidra decompilation (0x4ab240):

uint __cdecl hud_chat_listbox_remove_oldest(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;

  uVar3 = 0;
  if (((0 < DAT_00719424) && (DAT_00721eb8 != (code *)0x0)) &&
     (iVar1 = (*DAT_00721eb8)(DAT_00721ea4,PTR_PTR_0069c69c), iVar1 != 0)) {
    iVar2 = (*DAT_00721ecc)(iVar1,L"oListbox");
    if (iVar2 != 0) {
      uVar3 = (*DAT_00721ee8)(iVar2,0x182,0,0);
      (*DAT_00721ee8)(iVar2,0x115,2,0);
      (*DAT_00721ed0)(iVar1);
    }
    (*DAT_00721ec8)(iVar1);
  }
  DAT_00719424 = DAT_00719424 + -1;
  _memmove(&DAT_006b3a20,&DAT_006b3a24,DAT_00719424 * 4);
  (&DAT_006b3a20)[DAT_00719424] = 0;
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
