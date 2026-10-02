// server_list_scroll_page_up  (Ghidra: server_list_scroll_page_up, already named)
// address 0x4b7b20, size 134 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_types_notes.md's server-browser scroll-offset note
// (0x00719478); server_list_result_count_get is this module's own rewrite; mirrors
// server_list_scroll_clamp.c's `max(0, count - 15)` derivation of the branchless clamp.
// register convention: jump-to-top flag in AL (in_AL).
// UNSURE: DAT_00719484 (the scroll fade/animation counter, also used by
// join_game_server_browser_tick.c) and server_browser_skip_reselect (a "needs re-highlight" style flag) have no
// documented names beyond their shared use across this module's files.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t server_list_scroll_offset; // 0x00719478
extern int32_t server_browser_selected_index; // 0x006953f4
extern int32_t DAT_00719484; // see UNSURE
extern uint8_t server_browser_skip_reselect; // 0x00719480, byte-sized at every access // see UNSURE

extern int32_t server_list_result_count_get(void); // 0x4ba820, this module

// blam-cc: jump-to-top flag in AL (in_AL)
void server_list_scroll_page_up(uint8_t jump_to_top)
{
    int32_t old_offset;
    int32_t count;
    int32_t max_scroll;

    old_offset = server_list_scroll_offset;
    if (jump_to_top == 0) {
        server_list_scroll_offset = server_list_scroll_offset - 0xe;
    } else {
        server_list_scroll_offset = 0;
    }
    count = server_list_result_count_get();
    max_scroll = (count - 0xf < 0) ? 0 : (count - 0xf);
    if (server_list_scroll_offset < 0) {
        max_scroll = 0;
    } else if (server_list_scroll_offset <= max_scroll) {
        goto after_clamp;
    }
    server_list_scroll_offset = max_scroll;
after_clamp:
    if (old_offset != server_list_scroll_offset) {
        DAT_00719484 = 0xfffffff0;
        if (server_list_scroll_offset <= server_browser_selected_index &&
            server_browser_selected_index < server_list_scroll_offset + 0xf) {
            server_browser_skip_reselect = 0;
            return;
        }
        server_browser_skip_reselect = 1;
    }
}

#if 0
Original Ghidra decompilation (0x4b7b20):

void server_list_scroll_page_up(void)

{
  uint uVar1;
  char in_AL;
  uint uVar2;

  uVar1 = DAT_00719478;
  if (in_AL == '\0') {
    DAT_00719478 = DAT_00719478 - 0xe;
  }
  else {
    DAT_00719478 = 0;
  }
  uVar2 = server_list_result_count_get();
  uVar2 = ((int)(uVar2 - 0xf) < 0) - 1 & uVar2 - 0xf;
  if ((int)DAT_00719478 < 0) {
    uVar2 = 0;
  }
  else if ((int)DAT_00719478 <= (int)uVar2) goto LAB_004b7b69;
  DAT_00719478 = uVar2;
LAB_004b7b69:
  if (uVar1 != DAT_00719478) {
    DAT_00719484 = 0xfffffff0;
    if (((int)DAT_00719478 <= DAT_006953f4) && (DAT_006953f4 < (int)(DAT_00719478 + 0xf))) {
      DAT_00719480 = 0;
      return;
    }
    DAT_00719480 = 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
