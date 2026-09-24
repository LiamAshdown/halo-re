// block_list_reallocate
// address 0x4d1de0, size 144 bytes
// name confidence: 0.8 (module summary: "Grows a block from an arena in place if possible, or
// allocates a new block and migrates the data, similar to realloc()")
// rewrite confidence: 0.45
// evidence: types/memory.h memory_pool/memory_pool_block; out/phase4/memory_types_notes.md's
// block_list_* paragraph.
// register convention: new requested payload size in EDX (in_EDX); pointer to the caller's "data"
// pointer variable in EBX (unaff_EBX, matching block_list_allocate's `owner` parameter exactly --
// this function passes it straight through); memory_pool* on the stack (param_1).
// UNSURE: the "must move" path reads the copy source via what Ghidra shows as a second
// dereference of `*unaff_EBX`, but by that point block_list_allocate has already overwritten
// *owner with the *new* payload address (its own documented side effect), so a literal re-read
// there would copy the new block over itself. This is reconstructed using the block's payload
// pointer saved at function entry (`old_payload`, Ghidra's own `iVar1`) as the copy source
// instead, which is the only reading consistent with "preserve the old block's contents" and with
// the very first line of the decompiled function. Likewise `in_ECX` in the original (address-field
// and owner-pointer touch-ups after the copy) is reconstructed as the freshly-allocated payload,
// re-read from *owner_cell right after block_list_allocate returns, rather than trusting an
// unexplained register -- those two touch-ups are then provably redundant with what
// block_list_allocate already wrote, so this reconstruction changes nothing observable either way.

#include "tags.h"
#include "memory.h"
#include <string.h>

extern int32_t block_list_allocate(memory_pool *arena, int32_t requested_size, void **owner); // this batch
extern void block_list_unlink(void **payload_ptr, memory_pool *arena); // this batch

int32_t block_list_reallocate(void **owner_cell, int32_t new_size, memory_pool *arena)
{
    void *old_payload = *owner_cell;
    memory_pool_block *old_block = (memory_pool_block *)((uint8_t *)old_payload - 0x18);
    int32_t block_size = new_size + 0x18;
    uint32_t boundary;

    if ((block_size & 3) != 0) {
        block_size = (block_size | 3) + 1;
    }

    boundary = old_block->next != 0 ? (uint32_t)old_block->next :
        (uint32_t)((uint8_t *)arena->base + arena->size);

    if ((uint32_t)((uint8_t *)old_block + block_size) <= boundary) {
        arena->free_bytes += old_block->size - block_size;
        old_block->size = block_size;
        return 1;
    }

    if (block_list_allocate(arena, new_size, owner_cell)) {
        void *new_payload = *owner_cell;
        memory_pool_block *new_block = (memory_pool_block *)((uint8_t *)new_payload - 0x18);
        uint32_t old_payload_size = (uint32_t)old_block->size - 0x18;

        memcpy(new_payload, old_payload, old_payload_size);
        block_list_unlink(&old_payload, arena);

        new_block->address = owner_cell; // redundant with what block_list_allocate already set;
        *owner_cell = new_payload;       // preserved for exact fidelity, see file header.
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4d1de0):

undefined4 block_list_reallocate(int param_1)

{
  int iVar1;
  char cVar2;
  uint uVar3;
  undefined4 *in_ECX;
  uint uVar4;
  int in_EDX;
  int *unaff_EBX;
  undefined4 *puVar5;
  undefined4 *puVar6;

  iVar1 = *unaff_EBX;
  uVar3 = in_EDX + 0x18;
  if ((uVar3 & 3) != 0) {
    uVar3 = (uVar3 | 3) + 1;
  }
  uVar4 = *(uint *)(iVar1 + -0xc);
  if (uVar4 == 0) {
    uVar4 = *(int *)(param_1 + 0x28) + *(int *)(param_1 + 0x24);
  }
  if (uVar3 + iVar1 + -0x18 <= uVar4) {
    *(uint *)(param_1 + 0x2c) = *(int *)(param_1 + 0x2c) + (*(int *)(iVar1 + -0x14) - uVar3);
    *(uint *)(iVar1 + -0x14) = uVar3;
    return 1;
  }
  cVar2 = block_list_allocate();
  if (cVar2 != '\0') {
    uVar4 = *(int *)(iVar1 + -0x14) - 0x18;
    puVar5 = (undefined4 *)*unaff_EBX;
    puVar6 = in_ECX;
    for (uVar3 = uVar4 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
      *(undefined1 *)puVar6 = *(undefined1 *)puVar5;
      puVar5 = (undefined4 *)((int)puVar5 + 1);
      puVar6 = (undefined4 *)((int)puVar6 + 1);
    }
    block_list_unlink();
    in_ECX[-4] = unaff_EBX;
    *unaff_EBX = (int)in_ECX;
    return 1;
  }
  return 0;
}
#endif
