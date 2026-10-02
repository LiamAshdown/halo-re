// chat_close  (Ghidra: chat_close, already named)
// address 0x4aa900, size 169 bytes
// name confidence: 0.5 (existing Ghidra name)   rewrite confidence: 0.5
// evidence: shares every global with chimera__chat_open.c (same embedded GUI library table,
// chat_dialog_open/chat_scope_active) and the DirectInput device reset tail already established
// in virtual_keyboard_close.c; controls_input_capture_flags per src/interface/widget_close_all.c.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t chat_dialog_open;  // 0x006b3858
extern int32_t chat_scope_active; // 0x006b385c
extern uint8_t controls_input_capture_flags; // 0x00712542, UNSURE (per src/interface/widget_close_all.c)

extern void **keyboard_device;        // 0x006b1800
extern uint8_t key_frames[0x6d]; // 0x006b1620
extern uint8_t key_release_pending[0x6d]; // 0x006b168d

extern void *chat_gui_root_handle;    // 0x00721ea4
extern chat_gui_find_object_fn chat_gui_find_object; // 0x00721eb8
extern void *chat_gui_find_object_arg; // 0x0069c698
extern chat_gui_set_focus_fn chat_gui_set_focus;    // 0x00721ed4
extern chat_gui_set_state_fn chat_gui_set_state;    // 0x00721edc
extern chat_gui_release_fn chat_gui_release;        // 0x00721ec8
extern uint8_t chat_gui_active;                     // 0x00721eec

// Closes the multiplayer chat input dialog: clears the chat-open state, resets the DirectInput
// keyboard device the same way virtual_keyboard_close does, and releases the GUI dialog object.
void chat_close(void)
{
    void *gui_object;

    if (chat_dialog_open == 0) {
        return;
    }

    controls_input_capture_flags &= 0xfb;
    chat_scope_active = -1;
    chat_dialog_open = 0;

    if (keyboard_device != 0) {
        int32_t minus_one = -1;
        void **vtable = *(void ***)keyboard_device;
        ((directinput_set_property_fn)vtable[0x28 / 4])(keyboard_device, 0x14, 0, &minus_one, 0);
        memset(key_release_pending, 0, sizeof(key_release_pending));
        memset(key_frames, 0, sizeof(key_frames));
    }

    chat_gui_active = 0;
    gui_object = chat_gui_find_object(chat_gui_root_handle, chat_gui_find_object_arg);
    if (gui_object != 0) {
        chat_gui_set_focus(gui_object, 0);
        chat_gui_set_state(gui_object, 0);
        chat_gui_release(gui_object);
    }
}

#if 0
Original Ghidra decompilation (0x4aa900):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void chat_close(void)

{
  undefined4 in_ECX;
  int iVar1;
  undefined4 *puVar2;
  undefined4 local_4;

  if (DAT_006b3858 != '\0') {
    DAT_00712542 = DAT_00712542 & 0xfb;
    DAT_006b385c = 0xffffffff;
    DAT_006b3858 = '\0';
    local_4 = in_ECX;
    if (DAT_006b1800 != (int *)0x0) {
      local_4 = 0xffffffff;
      (**(code **)(*DAT_006b1800 + 0x28))(DAT_006b1800,0x14,0,&local_4,0);
      puVar2 = &DAT_006b168d;
      for (iVar1 = 0x1b; iVar1 != 0; iVar1 = iVar1 + -1) {
        *puVar2 = 0;
        puVar2 = puVar2 + 1;
      }
      *(undefined1 *)puVar2 = 0;
      puVar2 = (undefined4 *)&DAT_006b1620;
      for (iVar1 = 0x1b; iVar1 != 0; iVar1 = iVar1 + -1) {
        *puVar2 = 0;
        puVar2 = puVar2 + 1;
      }
      *(undefined1 *)puVar2 = 0;
    }
    _DAT_00721eec = 0;
    iVar1 = (*DAT_00721eb8)(DAT_00721ea4,PTR_PTR_0069c698);
    if (iVar1 != 0) {
      (*DAT_00721ed4)(iVar1,0);
      (*DAT_00721edc)(iVar1,0);
      (*DAT_00721ec8)(iVar1);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
