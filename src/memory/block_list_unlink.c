// block_list_unlink
// address 0x4d1e70, size 55 bytes
// name confidence: 0.8 (module summary; called by block_list_reallocate)
// rewrite confidence: 0.6
// evidence: types/memory.h memory_pool_block; out/phase4/memory_types_notes.md "block_list_unlink
// @0x4d1e70 ... address the same block through the payload pointer: payload-0x14=size,
// payload-0xc=next, payload-8=previous".
// register convention: EAX holds a pointer to the block's *own* payload-pointer variable --
// i.e. `*in_EAX` is the block's payload address, not the block address itself (this matches how
// block_list_reallocate.c calls it, passing the address of its own saved `old_payload` local);
// memory_pool* in EDX (in_EDX).
// FIXED (register inputs, objdump): EAX/EDX (read at 0x4d1e70/0x4d1e79) were already C
//   parameters (payload_ptr/arena) but had no machine-checked "blam-cc" line at all; added.
//   // blam-cc: EAX -> payload_ptr, EDX -> arena

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"

// blam-cc: EAX -> payload_ptr, EDX -> arena
void block_list_unlink(void **payload_ptr, memory_pool *arena)
{
    memory_pool_block *block = (memory_pool_block *)((uint8_t *)*payload_ptr - 0x18);

    arena->free_bytes += block->size;
    if (block->previous == 0) {
        arena->first_block = block->next;
    } else {
        block->previous->next = block->next;
    }
    if (block->next != 0) {
        block->next->previous = block->previous;
        return;
    }
    arena->last_block = block->previous;
}

#if 0
Original Ghidra decompilation (0x4d1e70):

void block_list_unlink(void)

{
  int iVar1;
  int *in_EAX;
  int in_EDX;

  iVar1 = *in_EAX;
  *(int *)(in_EDX + 0x2c) = *(int *)(in_EDX + 0x2c) + *(int *)(iVar1 + -0x14);
  if (*(int *)(iVar1 + -8) == 0) {
    *(undefined4 *)(in_EDX + 0x30) = *(undefined4 *)(iVar1 + -0xc);
  }
  else {
    *(undefined4 *)(*(int *)(iVar1 + -8) + 0xc) = *(undefined4 *)(iVar1 + -0xc);
  }
  if (*(int *)(iVar1 + -0xc) != 0) {
    *(undefined4 *)(*(int *)(iVar1 + -0xc) + 0x10) = *(undefined4 *)(iVar1 + -8);
    return;
  }
  *(undefined4 *)(in_EDX + 0x34) = *(undefined4 *)(iVar1 + -8);
  return;
}
#endif
