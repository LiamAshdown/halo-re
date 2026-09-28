// chimera__multiplayer_message  (Ghidra: chimera__multiplayer_message, already named)
// address 0x4ab4b0, size 214 bytes
// name confidence: 0.55 (existing Ghidra name, matches types/interface.h)   rewrite confidence: 0.6
// evidence: types/interface.h hud_chat_message_count (0x00719424) and hud_chat_message_expiry
// (0x006b3a20, "8 seconds past post time"); string "oListbox"; shares the chat-listbox GUI
// table with hud_chat_listbox_remove_oldest.c.
// UNSURE: the QueryPerformanceCounter-based millisecond conversion (0x006ac8f8/0x006ac8fc, a
// 64 bit performance-counter frequency) is transcribed with plain 64 bit arithmetic in place of
// the original's __allmul/__alldiv helper calls, which is behaviorally identical on this
// target.
// register convention: __cdecl, text as the recognized parameter. The text is wide: every caller
// (chat_dispatch_incoming, chimera__kill_feed, game_engine_on_player_death) builds a wchar_t line.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>

extern int32_t hud_chat_message_count;     // 0x00719424
extern int32_t hud_chat_message_expiry[8]; // 0x006b3a20
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, established in src/cache/

extern void *chat_gui_root_handle;      // 0x00721ea4
extern chat_gui_find_object_fn chat_gui_find_object; // 0x00721eb8
extern void *chat_listbox_gui_find_object_arg;  // 0x0069c69c
extern chat_gui_find_child_fn chat_gui_find_child;    // 0x00721ecc
extern chat_gui_set_property_int_fn chat_gui_set_property_int; // 0x00721ee8
extern chat_gui_finalize_fn chat_gui_finalize;  // 0x00721ed0
extern chat_gui_release_fn chat_gui_release;    // 0x00721ec8

extern uint32_t hud_chat_listbox_remove_oldest(void); // 0x4ab240

// Appends a new line of text to the on-screen chat/message listbox GUI control, evicting the
// oldest entry first if 8 or more are already shown, and stamps its expiry 8000 ms past the
// current performance-counter time.
void chimera__multiplayer_message(const wchar_t *text)
{
    int64_t counter;
    int64_t now_ms;

    if (hud_chat_message_count > 7) {
        hud_chat_listbox_remove_oldest();
    }

    if (chat_gui_find_object != 0) {
        void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
        if (gui_object != 0) {
            void *listbox = chat_gui_find_child(gui_object, (const uint16_t *)L"oListbox");
            if (listbox != 0) {
                chat_gui_set_property_int(listbox, 0x180, 0, text);
                chat_gui_set_property_int(listbox, 0x115, 2, 0);
                chat_gui_finalize(gui_object);
            }
            chat_gui_release(gui_object);
        }
    }

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (counter * 1000) / performance_frequency;
    hud_chat_message_expiry[hud_chat_message_count] = (int32_t)now_ms + 8000;
    hud_chat_message_count = hud_chat_message_count + 1;
}

#if 0
Original Ghidra decompilation (0x4ab4b0):

void __cdecl chimera__multiplayer_message(char *text)

{
  int iVar1;
  int iVar2;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  if (7 < DAT_00719424) {
    hud_chat_listbox_remove_oldest();
  }
  if ((DAT_00721eb8 != (code *)0x0) &&
     (iVar1 = (*DAT_00721eb8)(DAT_00721ea4,PTR_PTR_0069c69c), iVar1 != 0)) {
    iVar2 = (*DAT_00721ecc)(iVar1,L"oListbox");
    if (iVar2 != 0) {
      (*DAT_00721ee8)(iVar2,0x180,0,text);
      (*DAT_00721ee8)(iVar2,0x115,2,0);
      (*DAT_00721ed0)(iVar1);
    }
    (*DAT_00721ec8)(iVar1);
  }
  QueryPerformanceCounter(&local_8);
  uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  iVar1 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
  (&DAT_006b3a20)[DAT_00719424] = iVar1 + 8000;
  DAT_00719424 = DAT_00719424 + 1;
  return;
}
#endif
