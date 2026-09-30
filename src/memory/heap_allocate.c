// heap_allocate
// address 0x4d1f10, size 103 bytes
// name confidence: 0.85 (already a real Ghidra name; module summary: "Public allocation entry
// point for a bucketed heap: allocates a raw block and updates the heap's usage statistics")
// rewrite confidence: 0.6
// evidence: types/memory.h heap/heap_block layout, matched field-for-field against every
// statistic this function updates (bytes_allocated, peak_bytes_allocated, allocation_count,
// peak_allocation_count, peak_allocation_size).
// register convention: heap* in ECX (in_ECX). `size` is required by the elided heap_allocate_raw()
// call but is never referenced directly in this function's own body; reconstructed as EAX,
// matching heap_allocate_raw's own established parameter register -- UNSURE, not independently
// confirmed (same chain-elision pattern as heap_resize_block.c).
// blam-cc: EAX -> size, ECX -> self
// FIXED (register inputs, objdump): this file had no parseable blam-cc note at all; EAX is read
// live (call 0x4d2180 at 0x4d1f16 with no EAX setup) and forwarded as heap_allocate_raw's size.

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"


void *heap_allocate(uint32_t size, heap *self)
{
    heap_block *block = (heap_block *)heap_allocate_raw(size, self);
    void *payload;
    uint32_t allocation_count;

    if (block == 0) {
        return 0;
    }

    payload = (uint8_t *)block + 0x10;
    block->size = block->size | 0x80000000;
    allocation_count = (uint32_t)self->allocation_count + 1;
    self->bytes_allocated += (int32_t)(block->size & k_heap_block_size_mask);
    self->allocation_count = (int32_t)allocation_count;

    if (self->peak_bytes_allocated < self->bytes_allocated) {
        self->peak_bytes_allocated = self->bytes_allocated;
    }
    if ((uint32_t)self->peak_allocation_count < allocation_count) {
        self->peak_allocation_count = (int32_t)allocation_count;
    }
    if ((uint32_t)self->peak_allocation_size < (block->size & k_heap_block_size_mask)) {
        self->peak_allocation_size = (int32_t)(block->size & k_heap_block_size_mask);
    }
    return payload;
}

#if 0
Original Ghidra decompilation (0x4d1f10):

uint * heap_allocate(void)

{
  uint uVar1;
  uint *puVar2;
  uint *puVar3;
  int in_ECX;
  uint uVar4;

  puVar2 = (uint *)FUN_004d2180();
  if (puVar2 == (uint *)0x0) {
    puVar3 = (uint *)0x0;
  }
  else {
    uVar1 = *puVar2;
    *puVar2 = uVar1 | 0x80000000;
    uVar4 = *(int *)(in_ECX + 0x1c) + 1;
    *(uint *)(in_ECX + 0x14) = *(int *)(in_ECX + 0x14) + (uVar1 & 0x7fffffff);
    *(uint *)(in_ECX + 0x1c) = uVar4;
    puVar3 = puVar2 + 4;
    if (*(int *)(in_ECX + 0x18) < *(int *)(in_ECX + 0x14)) {
      *(int *)(in_ECX + 0x18) = *(int *)(in_ECX + 0x14);
    }
    if (*(uint *)(in_ECX + 0x20) < uVar4) {
      *(uint *)(in_ECX + 0x20) = uVar4;
    }
    if (*(uint *)(in_ECX + 0x24) < (*puVar2 & 0x7fffffff)) {
      *(uint *)(in_ECX + 0x24) = *puVar2 & 0x7fffffff;
      return puVar3;
    }
  }
  return puVar3;
}
#endif
