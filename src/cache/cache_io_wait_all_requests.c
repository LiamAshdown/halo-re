// cache_io_wait_all_requests
// address 0x4432b0, size 52 bytes
// name confidence: 0.7 (already named by Ghidra; out/phase4/cache_functions.md: "Blocks until
// every pending entry in the async cache-IO request queue has completed, used before tearing the
// queue down")
// rewrite confidence: 0.85
// evidence: types/cache.h cache_io_request (pending at 0x1d) and k_cache_io_request_count.
// register convention: no parameters.

#include "tags.h"
#include "cache.h"

extern void __stdcall Sleep(uint32_t milliseconds);

extern cache_io_request *cache_io_requests; // 0x006ac4a0

// Walks the whole cache_io_requests queue in slot order and, for each entry, spins on Sleep(0)
// until its pending flag clears before moving to the next slot.
void cache_io_wait_all_requests(void)
{
    int32_t slot_index;

    for (slot_index = 0; slot_index < (int32_t)k_cache_io_request_count; slot_index++) {
        while (cache_io_requests[slot_index].pending != 0) {
            Sleep(0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4432b0):

void __cdecl cache_io_wait_all_requests(void)

{
  char *pcVar1;
  char cVar2;
  int iVar3;
  int iVar4;

  iVar4 = 0;
  iVar3 = 0x200;
  do {
    pcVar1 = (char *)(iVar4 + 0x1d + DAT_006ac4a0);
    cVar2 = *pcVar1;
    while (cVar2 != '\0') {
      Sleep(0);
      cVar2 = *pcVar1;
    }
    iVar4 = iVar4 + 0x30;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  return;
}
#endif
