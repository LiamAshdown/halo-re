// ui_chat_window_reset_position  (Ghidra: FUN_004aa6b0, renamed)
// address 0x4aa6b0, size 80 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.4
// evidence: phase-4 summary "Resets the multiplayer chat window's screen position and clears
// its message listbox, used on UI (re)initialization"; types/interface.h's own note that the
// chat listbox is an embedded GUI-library "oListbox" control, not an interface-module struct
// ("only the expiry timestamps at 0x006b3a20 and the count at 0x00719424" belong to this
// module) -- kept as raw offsets on the external control rather than guessing its layout.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t chat_window_default_x; // 0x00692d80, UNSURE name
extern int32_t chat_window_default_y; // 0x00692d84, UNSURE name
extern int32_t chat_window_default_width;  // 0x00692d88, UNSURE name
extern int32_t chat_window_default_height; // 0x00692d8c, UNSURE name

extern int32_t chat_listbox_x;      // 0x006b38e4, UNSURE: external GUI listbox field, see header
extern int32_t chat_listbox_y;      // 0x006b38e8
extern int32_t chat_listbox_width;  // 0x006b38ec
extern int32_t chat_listbox_height; // 0x006b38f0
extern int32_t chat_dialog_open;    // UNSURE
extern int32_t unknown_006b3914;    // UNSURE
extern int32_t unknown_006b38f4;    // UNSURE
extern int32_t chat_scope_active;    // UNSURE

extern void hud_chat_listbox_clear(void); // 0x4ab400

// Resets the multiplayer chat window's screen position/size to its defaults and clears its
// message listbox.
void ui_chat_window_reset_position(void)
{
    chat_listbox_x = chat_window_default_x;
    chat_listbox_y = chat_window_default_y;
    chat_dialog_open = 0;
    unknown_006b3914 = 0;
    chat_listbox_width = chat_window_default_width;
    chat_listbox_height = chat_window_default_height;
    unknown_006b38f4 = 0;
    chat_scope_active = -1;
    hud_chat_listbox_clear();
}

#if 0
Original Ghidra decompilation (0x4aa6b0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004aa6b0(void)

{
  _DAT_006b38e4 = DAT_00692d80;
  _DAT_006b38e8 = DAT_00692d84;
  DAT_006b3858 = 0;
  DAT_006b3914 = 0;
  _DAT_006b38ec = DAT_00692d88;
  _DAT_006b38f0 = DAT_00692d8c;
  DAT_006b38f4 = 0;
  DAT_006b385c = 0xffffffff;
  hud_chat_listbox_clear();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
