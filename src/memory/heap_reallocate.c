// heap_reallocate
// address 0x4d1f80, size 145 bytes
// name confidence: 0.85 (already a real Ghidra name; module summary: "Public reallocation entry
// point for a bucketed heap: resizes/moves a block and updates the heap's usage statistics
// accordingly")
// rewrite confidence: 0.55
// evidence: types/memory.h heap/heap_block layout, matched field-for-field.
// register convention: old payload pointer (or NULL) in EAX (in_EAX); heap* in ESI (unaff_ESI).
// UNSURE: the new requested size is required by the elided heap_resize_block() call but is never
// referenced directly in this function's own body. Reconstructed as ECX (the next unused slot in
// the EAX,ECX,EDX,EBX,ESI,EDI priority order) -- not independently confirmed.

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"


void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self)
{
    heap_block *old_block = old_payload == 0 ? 0 : (heap_block *)((uint8_t *)old_payload - 0x10);
    uint32_t old_size = 0;
    heap_block *block;
    void *payload;
    int32_t bytes_allocated;
    uint32_t allocation_count;

    if (old_block != 0) {
        old_size = old_block->size & k_heap_block_size_mask;
    }

    block = (heap_block *)heap_resize_block(new_size, old_block, self);
    if (block == 0) {
        return 0;
    }

    if (0 <= (int32_t)block->size) { // only stamp the in-use bit if it isn't already set (a
        // block returned unchanged by an in-place grow already has it set).
        block->size = block->size | 0x80000000;
    }
    bytes_allocated = self->bytes_allocated + (int32_t)((block->size & k_heap_block_size_mask) - old_size);
    self->bytes_allocated = bytes_allocated;
    payload = (uint8_t *)block + 0x10;
    allocation_count = (uint32_t)self->allocation_count + (old_size == 0 ? 1u : 0u);
    self->allocation_count = (int32_t)allocation_count;

    if (self->peak_bytes_allocated < bytes_allocated) {
        self->peak_bytes_allocated = bytes_allocated;
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
Original Ghidra decompilation (0x4d1f80):

uint * heap_reallocate(void)

{
  int in_EAX;
  uint *puVar1;
  uint *puVar2;
  int iVar3;
  int unaff_ESI;
  uint uVar4;

  if (in_EAX == 0) {
    puVar1 = (uint *)0x0;
  }
  else {
    puVar1 = (uint *)(in_EAX + -0x10);
  }
  uVar4 = 0;
  if (puVar1 != (uint *)0x0) {
    uVar4 = *puVar1 & 0x7fffffff;
  }
  puVar1 = (uint *)FUN_004d2020();
  if (puVar1 == (uint *)0x0) {
    puVar2 = (uint *)0x0;
  }
  else {
    if (-1 < (int)*puVar1) {
      *puVar1 = *puVar1 | 0x80000000;
    }
    iVar3 = *(int *)(unaff_ESI + 0x14) + ((*puVar1 & 0x7fffffff) - uVar4);
    *(int *)(unaff_ESI + 0x14) = iVar3;
    puVar2 = puVar1 + 4;
    uVar4 = *(int *)(unaff_ESI + 0x1c) + (uint)(uVar4 == 0);
    *(uint *)(unaff_ESI + 0x1c) = uVar4;
    if (*(int *)(unaff_ESI + 0x18) < iVar3) {
      *(int *)(unaff_ESI + 0x18) = iVar3;
    }
    if (*(uint *)(unaff_ESI + 0x20) < uVar4) {
      *(uint *)(unaff_ESI + 0x20) = uVar4;
    }
    if (*(uint *)(unaff_ESI + 0x24) < (*puVar1 & 0x7fffffff)) {
      *(uint *)(unaff_ESI + 0x24) = *puVar1 & 0x7fffffff;
      return puVar2;
    }
  }
  return puVar2;
}
#endif
