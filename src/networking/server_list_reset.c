// server_list_reset  (Ghidra: server_list_reset, already named)
// address 0x4b65f0, size 103 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("clears the server-browser's list
// state (selection, scroll position) and refreshes its associated UI display elements");
// 0x006953f4 reset to -1 here matches master_server_process_pending_requests.c's
// server_browser_selected_index (also compared against -1 there); server_list_result_reset
// (0x4ba7c0) and ticker_text_buffer_append (0x4b8a60) are this module's own rewrites.
// register convention: EAX -> entry, forwarded unmodified to server_list_result_reset (its own
// EAX -> entry). Different call sites (objdump: 0x4b5e2d, 0x4b7862, 0x4b83ed, 0x4bab02) load
// distinct values (a global address, 0, ebp, esi) into EAX right before calling this function, so
// it is a genuine forwarded argument, not always NULL.
// FIXED (register inputs, objdump): EAX is read live at the `call 0x4ba7c0` (server_list_result_
// reset) with no local setup; added as a forwarded `entry` parameter instead of the hardcoded 0.
// Callers (disassembly): 0x4b5e32 / 0x4b5e9a load EAX = 0x7196bc, 0x4b7957 uses EAX = 0, 0x4b8403 and 0x4bab04
// pass a register value (ebp / esi); server_browser_open.c, master_server_process_pending_requests.c and
// join_game_server_browser_tick.c still declare it with no argument (see the audit report).
// 0x00719478/0x0071947c/0x00719480/0x00719481/0x00719484 are unnamed server-browser scalars; declared by address.
// FIXED in the review pass: the `self` (EDI) pointer is not visible in Ghidra's decompile,
// and an earlier draft of this file passed one shared singleton to every call. The
// disassembly of each call site shows two distinct instances -- see types/networking.h's
// server_browser_player_ticker / server_browser_variant_ticker.
// DAT_00719498 (the third append's text argument) is a static wide-character label buffer.

// VERIFIED against disassembly 0x4b65f0..0x4b6657 (2026-09-30): stores, EAX forwarded to server_list_result_reset, three ticker appends (text, column) with EDI = ticker
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

extern int32_t server_browser_total_players; // 0x00719474
extern int32_t server_list_scroll_offset; // 0x00719478
extern int32_t server_browser_selected_index; // 0x006953f4
extern int32_t server_browser_last_click_ms; // 0x0071947c
extern uint8_t server_browser_skip_reselect; // 0x00719480, byte-sized at every access // see UNSURE
extern uint8_t server_browser_player_list_ready; // 0x00719481
extern int32_t DAT_00719484; // 0x00719484
extern wchar_t DAT_00719498[0x100]; // 0x00719498

extern void server_list_result_reset(uint8_t *entry); // 0x4ba7c0, this module (entry == NULL here)
extern ticker_text_buffer server_browser_player_ticker;  // 0x006b5e58
extern ticker_text_buffer server_browser_variant_ticker; // 0x006b5e74
extern void ticker_text_buffer_append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self); // 0x4b8a60, this module

// blam-cc: EAX -> entry
void server_list_reset(uint8_t *entry)
{
    server_list_scroll_offset = 0;
    server_browser_selected_index = -1;
    server_browser_last_click_ms = 0;
    server_browser_player_list_ready = 0;
    server_browser_skip_reselect = 0;
    DAT_00719484 = 0;
    server_browser_total_players = 0;
    server_list_result_reset(entry);
    ticker_text_buffer_append(0, 2, &server_browser_player_ticker);
    ticker_text_buffer_append(0, 1, &server_browser_variant_ticker);
    ticker_text_buffer_append(DAT_00719498, 0, &server_browser_player_ticker);
}

#if 0
Original Ghidra decompilation (0x4b65f0):

void __cdecl server_list_reset(void)

{
  DAT_00719478 = 0;
  DAT_006953f4 = 0xffffffff;
  DAT_0071947c = 0;
  DAT_00719481 = 0;
  DAT_00719480 = 0;
  DAT_00719484 = 0;
  DAT_00719474 = 0;
  FUN_004ba7c0();
  ticker_text_buffer_append(0,2);
  ticker_text_buffer_append(0,1);
  ticker_text_buffer_append(&DAT_00719498,0);
  return;
}
#endif
