// server_browser_ui_refresh  (Ghidra: FUN_004b73a0, still unnamed -> renamed)
// address 0x4b73a0, size 51 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("refreshes the server-browser's
// on-screen list/header/status UI elements without altering the underlying list state");
// identical ticker_text_buffer_append tail to server_list_reset.c, minus that function's
// state-clearing globals -- exactly matching "without altering the underlying list state".
// register convention: __cdecl, no arguments.
// UNSURE: DAT_00719498's exact contents; see server_list_reset.c.
// FIXED in the review pass: the `self` (EDI) pointer is not visible in Ghidra's decompile,
// and an earlier draft of this file passed one shared singleton to every call. The
// disassembly of each call site shows two distinct instances -- see types/networking.h's
// server_browser_player_ticker / server_browser_variant_ticker.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern wchar_t DAT_00719498[0x100]; // see server_list_reset.c UNSURE
extern ticker_text_buffer server_browser_player_ticker;  // 0x006b5e58
extern ticker_text_buffer server_browser_variant_ticker; // 0x006b5e74
extern void ticker_text_buffer_append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self); // 0x4b8a60, this module

// blam-cc: __cdecl, no arguments
void server_browser_ui_refresh(void)
{
    ticker_text_buffer_append(0, 2, &server_browser_player_ticker);
    ticker_text_buffer_append(0, 1, &server_browser_variant_ticker);
    ticker_text_buffer_append(DAT_00719498, 0, &server_browser_player_ticker);
}

#if 0
Original Ghidra decompilation (0x4b73a0):

void FUN_004b73a0(void)

{
  ticker_text_buffer_append(0,2);
  ticker_text_buffer_append(0,1);
  ticker_text_buffer_append(&DAT_00719498,0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
