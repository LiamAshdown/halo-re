// server_list_scroll_page_down  (Ghidra: server_list_scroll_page_down, already named)
// address 0x4b7bb0, size 138 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: mirrors server_list_scroll_page_up.c exactly (same globals, same clamp shape),
// stepping the other direction.
// register convention: jump-to-bottom flag in AL (in_AL).
// UNSURE: see server_list_scroll_page_up.c for DAT_00719484/server_browser_skip_reselect.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern int32_t server_list_scroll_offset; // 0x00719478
extern int32_t server_browser_selected_index; // 0x006953f4
extern int32_t DAT_00719484; // see server_list_scroll_page_up.c UNSURE
extern uint8_t server_browser_skip_reselect; // 0x00719480, byte-sized at every access // see server_list_scroll_page_up.c UNSURE

extern int32_t server_list_result_count_get(void); // 0x4ba820, this module

// blam-cc: jump-to-bottom flag in AL (in_AL)
void server_list_scroll_page_down(uint8_t jump_to_bottom)
{
    int32_t old_offset;
    int32_t count;
    int32_t max_scroll;

    old_offset = server_list_scroll_offset;
    if (jump_to_bottom == 0) {
        server_list_scroll_offset = server_list_scroll_offset + 0xe;
    } else {
        count = server_list_result_count_get();
        server_list_scroll_offset = count - 0xf;
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
        DAT_00719484 = 0x10;
        if (server_list_scroll_offset <= server_browser_selected_index &&
            server_browser_selected_index < server_list_scroll_offset + 0xf) {
            server_browser_skip_reselect = 0;
            return;
        }
        server_browser_skip_reselect = 1;
    }
}

#if 0
Original Ghidra decompilation (0x4b7bb0):

void server_list_scroll_page_down(void)

{
  uint uVar1;
  char in_AL;
  uint uVar2;

  uVar1 = DAT_00719478;
  if (in_AL == '\0') {
    DAT_00719478 = DAT_00719478 + 0xe;
  }
  else {
    uVar2 = server_list_result_count_get();
    DAT_00719478 = uVar2 - 0xf;
  }
  uVar2 = server_list_result_count_get();
  uVar2 = ((int)(uVar2 - 0xf) < 0) - 1 & uVar2 - 0xf;
  if ((int)DAT_00719478 < 0) {
    uVar2 = 0;
  }
  else if ((int)DAT_00719478 <= (int)uVar2) goto LAB_004b7bfd;
  DAT_00719478 = uVar2;
LAB_004b7bfd:
  if (uVar1 != DAT_00719478) {
    DAT_00719484 = 0x10;
    if (((int)DAT_00719478 <= DAT_006953f4) && (DAT_006953f4 < (int)(DAT_00719478 + 0xf))) {
      DAT_00719480 = 0;
      return;
    }
    DAT_00719480 = 1;
  }
  return;
}
#endif
