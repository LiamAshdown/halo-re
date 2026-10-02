// ticker_text_buffer_reset  (Ghidra: FUN_004b8a00)
// address 0x4b8a00, size 81 bytes
// name confidence: 0.30 (parked pass-1 proposal in out/phase2/p2_networking/00.md; no direct
// evidence of the real name, chosen to match the sibling ticker_text_buffer_append/advance
// functions and the module summary "resetting it to an empty state with a default 100ms
// scroll delay")
// rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary; types/memory.h heap/heap_block layout
// for the heap_unlink_block call and the bytes_allocated/allocation_count bookkeeping that
// follows it; the widget_memory_pool global identified from out/phase2/interface/01.md
// (widget_memory_pool_initialize @ 0x4979b0 constructs the heap at 0x006926c4).
// register convention: this-pointer (ticker_text_buffer *) in EDI (unaff_EDI).
// ticker_text_buffer now lives in types/networking.h; its layout was pinned jointly from this
// function, ticker_text_buffer_append (0x4b8a60) and ticker_text_buffer_advance (0x4b8b40),
// and folded into the header during the review pass.
// FIXED (register inputs, objdump): EDI (read at 0x4b8a00) was already a C parameter (self)
//   but the "blam-cc" note said "this-pointer as EDI", which the checker's "REG -> name"
//   parser could not read; reworded.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern heap *widget_memory_pool; //  0x006926c4, "widget_memory_pool" (built by 0x4979b0) -- the global holds a POINTER to the heap (mov esi,[0x6926c4] at every call site)

extern void heap_unlink_block(heap_block *block, heap *self); // 0x4d20a0

// blam-cc: EDI -> self
// Frees the buffer's current heap allocation (if any), updating widget_memory_pool's usage
// totals, then resets every field to an empty buffer with the default 100 ms scroll delay.
void ticker_text_buffer_reset(ticker_text_buffer *self)
{
    if (self->text != 0) {
        heap_block *block = (heap_block *)((uint8_t *)self->text - 0x10);
        uint32_t size = block->size & k_heap_block_size_mask;

        heap_unlink_block(block, widget_memory_pool);
        widget_memory_pool->bytes_allocated -= (int32_t)size;
        widget_memory_pool->allocation_count -= 1;
    }
    self->text = 0;
    self->capacity = 0;
    self->length = 0;
    self->scroll_cursor = 0;
    self->scroll_delay_ms = 100;
}

#if 0
Original Ghidra decompilation (0x4b8a00):

void FUN_004b8a00(void)

{
  uint uVar1;
  int extraout_ECX;
  int *unaff_EDI;

  if (*unaff_EDI != 0) {
    uVar1 = *(uint *)(*unaff_EDI + -0x10);
    heap_unlink_block();
    *(uint *)(extraout_ECX + 0x14) = *(int *)(extraout_ECX + 0x14) - (uVar1 & 0x7fffffff);
    *(int *)(extraout_ECX + 0x1c) = *(int *)(extraout_ECX + 0x1c) + -1;
  }
  *unaff_EDI = 0;
  unaff_EDI[4] = 0;
  unaff_EDI[3] = 0;
  unaff_EDI[2] = 0;
  unaff_EDI[5] = 100;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
