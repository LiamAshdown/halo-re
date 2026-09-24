// heap_resize_block  (Ghidra: FUN_004d2020)
// address 0x4d2020, size 114 bytes
// name confidence: 0.7 (out/phase4/memory_types_notes.md: "heap_resize_block @0x4d2020")
// rewrite confidence: 0.45
// evidence: types/memory.h heap_block layout (payload at header+0x10, matching the -0x10/+4-uint
// arithmetic here exactly); `old_block` is confirmed to be a block *header* pointer (not payload)
// by heap_reallocate.c, its only caller, which computes `old_payload - 0x10` before calling this.
// register convention: new payload size in EAX (in_EAX); old block header, or NULL for a fresh
// allocation, in EBX (unaff_EBX).
// UNSURE: `heap *self` is required by the two elided heap_allocate_raw() calls and the elided
// heap_unlink_block() call, but is never referenced directly in this function's own decompiled
// body (Ghidra shows no unaff_/in_ hint for it at all -- the same whole-chain elision seen in
// heap_reallocate.c and data_packet_group_encode_packet.c). Reconstructed as EDI, matching
// heap_allocate_raw's own established register for the same parameter, on the assumption it is
// passed through unchanged; this could not be independently confirmed.

#include "tags.h"
#include "memory.h"
#include <string.h>

extern uint32_t heap_allocate_raw(uint32_t size, heap *self); // this batch
extern void heap_unlink_block(heap_block *block, heap *self); // this batch

void *heap_resize_block(uint32_t new_size, heap_block *old_block, heap *self)
{
    void *new_block;

    if (new_size == 0) {
        return 0;
    }
    if (old_block == 0) {
        return (void *)heap_allocate_raw(new_size, self);
    }
    if (new_size <= (old_block->size & k_heap_block_size_mask) - 0x10) {
        return old_block;
    }
    new_block = (void *)heap_allocate_raw(new_size, self);
    if (new_block != 0) {
        uint32_t old_payload_size = (old_block->size & k_heap_block_size_mask) - 0x10;
        uint8_t *old_payload = (uint8_t *)old_block + 0x10;
        uint8_t *new_payload = (uint8_t *)new_block + 0x10;

        memcpy(new_payload, old_payload, old_payload_size);
        heap_unlink_block(old_block, self);
        return new_block;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4d2020):

uint * FUN_004d2020(void)

{
  uint in_EAX;
  uint *puVar1;
  uint uVar2;
  uint uVar3;
  uint *unaff_EBX;
  uint *puVar4;
  uint *puVar5;

  if (in_EAX == 0) {
    return (uint *)0x0;
  }
  if (unaff_EBX == (uint *)0x0) {
    puVar1 = (uint *)FUN_004d2180();
    return puVar1;
  }
  if (in_EAX <= (*unaff_EBX & 0x7fffffff) - 0x10) {
    return unaff_EBX;
  }
  puVar1 = (uint *)FUN_004d2180();
  if (puVar1 != (uint *)0x0) {
    uVar2 = (*unaff_EBX & 0x7fffffff) - 0x10;
    puVar4 = unaff_EBX + 4;
    puVar5 = puVar1 + 4;
    for (uVar3 = uVar2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
      *(char *)puVar5 = (char)*puVar4;
      puVar4 = (uint *)((int)puVar4 + 1);
      puVar5 = (uint *)((int)puVar5 + 1);
    }
    FUN_004d20a0();
    return puVar1;
  }
  return (uint *)0x0;
}
#endif
