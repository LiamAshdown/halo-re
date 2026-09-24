// cache_io_request_find_free_slot
// address 0x443270, size 51 bytes
// name confidence: 0.7 (already named by Ghidra; out/phase4/cache_functions.md: "Busy-scans the
// async cache-IO request queue for a free (unused) request slot")
// rewrite confidence: 0.75
// evidence: types/cache.h cache_io_request (pending at 0x1d) and k_cache_io_request_count.
// register convention: no parameters; returns the slot index. Ghidra lost track of the return
// value (renders the function as void with a bare `return;`), but every caller (cache_io_
// request_new) reads it out of AX immediately after the call.
// UNSURE: the `retried` flag (bVar1 in the original) is set once after a full failed pass over
// all 0x200 slots but never read again, so it has no effect on control flow; preserved as dead
// state rather than dropped, since dropping it would be an "improvement".

#include "tags.h"
#include "cache.h"

extern cache_io_request *cache_io_requests; // 0x006ac4a0

// Busy-scans the fixed 0x200-entry cache_io_requests queue, in slot order, for the first entry
// whose pending flag is clear, and returns its index. Never gives up: if every slot is pending it
// keeps re-scanning until one frees up.
int16_t cache_io_request_find_free_slot(void)
{
    uint8_t retried; // UNSURE, see file header
    short slot_index;

    retried = 0;
    for (;;) {
        for (slot_index = 0; slot_index < (short)k_cache_io_request_count; slot_index++) {
            if (cache_io_requests[slot_index].pending == 0) {
                return slot_index;
            }
        }
        if (!retried) {
            retried = 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x443270):

void cache_io_request_find_free_slot(void)

{
  bool bVar1;
  short sVar2;

  bVar1 = false;
  do {
    sVar2 = 0;
    do {
      if (*(char *)(sVar2 * 0x30 + 0x1d + DAT_006ac4a0) == '\0') {
        return;
      }
      sVar2 = sVar2 + 1;
    } while (sVar2 < 0x200);
    if (!bVar1) {
      bVar1 = true;
    }
  } while( true );
}
#endif
