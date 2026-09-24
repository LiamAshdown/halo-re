// server_list_scroll_clamp  (Ghidra: FUN_004b7360, still unnamed -> renamed)
// address 0x4b7360, size 58 bytes
// name confidence: 0.45   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("clamps the server-browser list's
// current scroll offset to stay within the valid range for the current server count");
// server_list_result_count_get is this module's own rewrite; DAT_00719478 is the scroll-offset
// global server_list_reset.c already zeroes.
// register convention: EAX -> results, an optional results-snapshot pointer: NULL uses the live
// shared server_list via server_list_result_count_get, non-NULL reads a result_count field at
// +0x04 directly (the same offset as server_list_globals.result_count).
// UNSURE: the branchless `(count - 15) & mask` clamp is folded into an equivalent `max(0, count
// - 15)` ternary; the two are numerically identical for every value of count.
// FIXED (register inputs, objdump): blam-cc wording ("results-snapshot pointer in EAX")
// didn't parse as a register mapping; reworded to "EAX -> results" (read at 0x4b7360,
// test eax,eax). Behaviour unchanged, results was already the intended parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t server_list_scroll_offset; // 0x00719478

extern int32_t server_list_result_count_get(void); // 0x4ba820, this module

// blam-cc: EAX -> results
void server_list_scroll_clamp(server_list_globals *results)
{
    int32_t count;
    int32_t max_scroll;

    if (results == 0) {
        count = server_list_result_count_get();
    } else {
        count = results->result_count;
    }
    max_scroll = (count - 0xf < 0) ? 0 : (count - 0xf);
    if (server_list_scroll_offset < 0) {
        server_list_scroll_offset = 0;
        return;
    }
    if (max_scroll < server_list_scroll_offset) {
        server_list_scroll_offset = max_scroll;
    }
}

#if 0
Original Ghidra decompilation (0x4b7360):

void FUN_004b7360(void)

{
  int in_EAX;
  uint uVar1;

  if (in_EAX == 0) {
    uVar1 = server_list_result_count_get();
  }
  else {
    uVar1 = *(uint *)(in_EAX + 4);
  }
  uVar1 = uVar1 - 0xf & ((int)(uVar1 - 0xf) < 0) - 1;
  if ((int)DAT_00719478 < 0) {
    DAT_00719478 = 0;
    return;
  }
  if ((int)uVar1 < (int)DAT_00719478) {
    DAT_00719478 = uVar1;
  }
  return;
}
#endif
