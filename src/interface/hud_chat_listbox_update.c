// hud_chat_listbox_update  (Ghidra: hud_chat_listbox_update, already named)
// address 0x4ab300, size 252 bytes
// name confidence: 0.5 (existing Ghidra name, matches types/interface.h)   rewrite confidence: 0.4
// evidence: types/interface.h hud_chat_message_count (0x00719424), hud_chat_message_expiry
// (0x006b3a20) and hud_chat_listbox_visible (0x00692ed8); shares the chat-listbox GUI table
// with hud_chat_listbox_remove_oldest.c.
// UNSURE: DAT_0087aa10/DAT_0087aa14 (an int and a float gating the listbox's visibility state)
// are not documented anywhere in this pass.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t hud_chat_message_count;     // 0x00719424
extern int32_t hud_chat_message_expiry[8]; // 0x006b3a20
extern uint8_t hud_chat_listbox_visible;   // 0x00692ed8

extern int32_t game_engine_state_value; // UNSURE
extern float game_engine_nameplate_fade_opacity_array;   // UNSURE

extern void *chat_gui_root_handle;      // 0x00721ea4
extern chat_gui_find_object_fn chat_gui_find_object; // 0x00721eb8
extern void *chat_listbox_gui_find_object_arg;  // 0x0069c69c
extern chat_gui_set_state_fn chat_gui_set_state; // 0x00721edc
extern chat_gui_release_fn chat_gui_release;    // 0x00721ec8

extern int32_t time_query_performance_counter_ms(void); // 0x449210, UNSURE: appears to be a millisecond clock
extern uint32_t hud_chat_listbox_remove_oldest(void); // 0x4ab240

// Expires timed-out chat messages (whose stored expiry has passed the current time) from the
// front of the listbox, then shows or hides the GUI listbox control depending on
// game_engine_state_value/game_engine_nameplate_fade_opacity_array.
void hud_chat_listbox_update(void)
{
    if (hud_chat_message_count <= 0 || chat_gui_find_object == 0) {
        return;
    }

    {
        uint32_t now = (uint32_t)time_query_performance_counter_ms();
        while (hud_chat_message_count > 0 && hud_chat_message_expiry[0] != 0 &&
               (uint32_t)hud_chat_message_expiry[0] <= now) {
            hud_chat_listbox_remove_oldest();
        }
    }

    if (hud_chat_listbox_visible == 0) {
        if (game_engine_state_value != 0 || game_engine_nameplate_fade_opacity_array == 0.0f) {
            void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
            if (gui_object != 0) {
                chat_gui_set_state(gui_object, 5);
                chat_gui_release(gui_object);
            }
            hud_chat_listbox_visible = 1;
        }
    } else if (game_engine_state_value == 0 && game_engine_nameplate_fade_opacity_array != 0.0f) {
        void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
        if (gui_object != 0) {
            chat_gui_set_state(gui_object, 0);
            chat_gui_release(gui_object);
        }
        hud_chat_listbox_visible = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4ab300):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl hud_chat_listbox_update(void)

{
  uint uVar1;
  int iVar2;

  if ((0 < DAT_00719424) && (DAT_00721eb8 != (code *)0x0)) {
    uVar1 = FUN_00449210();
    while (((0 < DAT_00719424 && (DAT_006b3a20 != 0)) && (DAT_006b3a20 <= uVar1))) {
      hud_chat_listbox_remove_oldest();
    }
    if (DAT_00692ed8 == '\0') {
      if ((DAT_0087aa10 != 0) || (_DAT_0087aa14 == 0.0)) {
        iVar2 = (*DAT_00721eb8)(DAT_00721ea4,PTR_PTR_0069c69c);
        if (iVar2 != 0) {
          (*DAT_00721edc)(iVar2,5);
          (*DAT_00721ec8)(iVar2);
        }
        DAT_00692ed8 = 1;
        return;
      }
    }
    else if ((DAT_0087aa10 == 0) && (_DAT_0087aa14 != 0.0)) {
      iVar2 = (*DAT_00721eb8)(DAT_00721ea4,PTR_PTR_0069c69c);
      if (iVar2 != 0) {
        (*DAT_00721edc)(iVar2,0);
        (*DAT_00721ec8)(iVar2);
      }
      DAT_00692ed8 = '\0';
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
