// block_list_allocate
// address 0x4d1d60, size 128 bytes
// name confidence: 0.8 (module summary, cross-checked against block_list_reallocate/_unlink/
// _compact which all address the same memory_pool_block layout)
// rewrite confidence: 0.7
// evidence: types/memory.h memory_pool/memory_pool_block layout, confirmed field-for-field by
// out/phase4/memory_types_notes.md's "block_list_allocate @0x4d1d60 stamps +0x00='head' ...
// +0x14='tail', and returns block+0x18 as the payload" paragraph.
// register convention: memory_pool* in ECX (in_ECX), requested payload size in EDX (in_EDX),
// pointer to the caller's own "data" pointer variable in EDI (unaff_EDI, written on success).
// blam-cc: ECX -> arena, EDX -> requested_size, EDI -> owner
// FIXED (register inputs, objdump): this file had no parseable blam-cc note (prose only); added.

#include "tags.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t block_list_allocate(memory_pool *arena, int32_t requested_size, void **owner)
{
    int32_t block_size = requested_size + 0x18;
    memory_pool_block *block;

    if ((block_size & 3) != 0) {
        block_size = (block_size | 3) + 1;
    }

    if (arena->last_block == 0) {
        block = (memory_pool_block *)arena->base;
    } else {
        block = (memory_pool_block *)((uint8_t *)arena->last_block + arena->last_block->size);
    }

    if ((uint8_t *)block + block_size <= (uint8_t *)arena->base + arena->size && block != 0) {
        block->head_signature = k_memory_pool_block_head_signature;
        block->size = block_size;
        block->address = owner;
        block->next = 0;
        block->previous = arena->last_block;
        block->tail_signature = k_memory_pool_block_tail_signature;
        if (arena->first_block == 0) {
            arena->first_block = block;
        }
        if (arena->last_block != 0) {
            arena->last_block->next = block;
        }
        arena->last_block = block;
        arena->free_bytes -= block->size;
        *owner = (uint8_t *)block + 0x18;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4d1d60):

uint block_list_allocate(void)

{
  int iVar1;
  undefined4 *puVar2;
  int in_ECX;
  int in_EDX;
  uint uVar3;
  undefined4 *unaff_EDI;

  uVar3 = in_EDX + 0x18;
  if ((uVar3 & 3) != 0) {
    uVar3 = (uVar3 | 3) + 1;
  }
  iVar1 = *(int *)(in_ECX + 0x34);
  if (iVar1 == 0) {
    puVar2 = *(undefined4 **)(in_ECX + 0x24);
  }
  else {
    puVar2 = (undefined4 *)(*(int *)(iVar1 + 4) + iVar1);
  }
  if (((int)puVar2 + uVar3 <= (uint)(*(int *)(in_ECX + 0x28) + *(int *)(in_ECX + 0x24))) &&
     (puVar2 != (undefined4 *)0x0)) {
    *puVar2 = 0x68656164;
    puVar2[1] = uVar3;
    puVar2[2] = unaff_EDI;
    puVar2[3] = 0;
    puVar2[4] = *(undefined4 *)(in_ECX + 0x34);
    puVar2[5] = 0x7461696c;
    if (*(int *)(in_ECX + 0x30) == 0) {
      *(undefined4 **)(in_ECX + 0x30) = puVar2;
    }
    if (*(int *)(in_ECX + 0x34) != 0) {
      *(undefined4 **)(*(int *)(in_ECX + 0x34) + 0xc) = puVar2;
    }
    *(undefined4 **)(in_ECX + 0x34) = puVar2;
    *(int *)(in_ECX + 0x2c) = *(int *)(in_ECX + 0x2c) - puVar2[1];
    *unaff_EDI = puVar2 + 6;
    return CONCAT31((int3)((uint)(puVar2 + 6) >> 8),1);
  }
  return (uint)puVar2 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
