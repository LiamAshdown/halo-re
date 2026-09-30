// heap_get_free_bytes  (Ghidra: FUN_004d20f0)
// address 0x4d20f0, size 31 bytes
// name confidence: 0.85 (out/phase4/memory_types_notes.md names it directly with the exact
// formula reproduced here)
// rewrite confidence: 0.85
// evidence: types/memory.h heap/heap_block layout.
// register convention: heap* in ECX (in_ECX).

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"

int32_t heap_get_free_bytes(heap *self)
{
    int32_t free_bytes = self->size;

    if (self->first_block != 0) {
        free_bytes = ((free_bytes - (int32_t)(self->last_block->size & k_heap_block_size_mask)) +
            (int32_t)self->base) - (int32_t)self->last_block;
    }
    return free_bytes;
}

#if 0
Original Ghidra decompilation (0x4d20f0):

int FUN_004d20f0(void)

{
  int iVar1;
  int in_ECX;

  iVar1 = *(int *)(in_ECX + 8);
  if (*(int *)(in_ECX + 0x2c) != 0) {
    iVar1 = ((iVar1 - (**(uint **)(in_ECX + 0x30) & 0x7fffffff)) + *(int *)(in_ECX + 4)) -
            (int)*(uint **)(in_ECX + 0x30);
  }
  return iVar1;
}
#endif
