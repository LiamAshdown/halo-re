// cache_io_request_new
// address 0x442b20, size 135 bytes
// name confidence: 0.75 (already named by Ghidra; out/phase4/cache_functions.md: "Allocates a
// free slot in the async cache-IO request queue, populates it with a file read request (offset,
// size, destination, flags), and signals the IO worker thread")
// rewrite confidence: 0.90
// evidence: types/cache.h cache_io_request / cache_io_completion layouts, both recovered
// specifically from this function's field writes (see the struct comments in cache.h).
// register convention: cache_io_completion *completion in ESI (unaff_ESI); offset, size,
// destination, priority and data_file_index are ordinary (stack) parameters, matching every
// caller in this module, which builds a completion record on its own stack and calls this
// function with the other five as normal arguments.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "win32.h"
#include "tags.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


extern cache_io_request *cache_io_requests; // 0x006ac4a0
extern void *cache_io_event;                // 0x006ac498

extern int16_t cache_io_request_find_free_slot(void); // this module, 0x443270

// blam-cc: completion in ESI
// Claims a free slot in the 0x200-entry async IO request queue, fills it in as a pending read of
// `size` bytes at `offset` into `destination` from `data_file_index`'s file, copies `*completion`
// into the request's own completion record, and wakes the IO worker thread. Returns the claimed
// slot index.
int16_t cache_io_request_new(cache_io_completion *completion, int32_t offset, uint32_t size,
    void *destination, uint8_t priority, uint8_t data_file_index)
{
    short slot_index;
    cache_io_request *request;

    slot_index = cache_io_request_find_free_slot();
    request = &cache_io_requests[slot_index];

    *completion->flag = 0;
    request->internal = 0;
    request->internal_high = 0;
    request->offset = 0;
    request->offset_high = 0;
    request->event = 0;
    request->size = size;
    request->offset = offset;
    request->event = 0;
    request->offset_high = 0;
    request->destination = destination;
    request->priority = priority;
    request->pending = 1;
    request->started = 0;
    request->data_file_index = data_file_index;
    request->completion = *completion;

    SetEvent(cache_io_event);
    return slot_index;
}

#if 0
Original Ghidra decompilation (0x442b20):

short cache_io_request_new
                (undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined1 param_4,
                undefined1 param_5)

{
  short sVar1;
  undefined4 *puVar2;
  undefined4 *unaff_ESI;

  sVar1 = cache_io_request_find_free_slot();
  puVar2 = (undefined4 *)(sVar1 * 0x30 + DAT_006ac4a0);
  *(undefined1 *)*unaff_ESI = 0;
  *puVar2 = 0;
  puVar2[1] = 0;
  puVar2[2] = 0;
  puVar2[3] = 0;
  puVar2[4] = 0;
  puVar2[5] = param_2;
  puVar2[2] = param_1;
  puVar2[4] = 0;
  puVar2[3] = 0;
  puVar2[6] = param_3;
  *(undefined1 *)((int)puVar2 + 0x1e) = 0;
  *(undefined1 *)((int)puVar2 + 0x1d) = 1;
  *(undefined1 *)(puVar2 + 7) = param_4;
  *(undefined1 *)(puVar2 + 8) = param_5;
  puVar2[9] = *unaff_ESI;
  puVar2[10] = unaff_ESI[1];
  puVar2[0xb] = unaff_ESI[2];
  SetEvent(DAT_006ac498);
  return sVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
