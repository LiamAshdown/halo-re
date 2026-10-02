// heap_unlink_block  (Ghidra: FUN_004d20a0)
// address 0x4d20a0, size 76 bytes
// name confidence: 0.85 (out/phase4/memory_types_notes.md: "Suggested renames: ... FUN_004d20a0
// -> heap_unlink_block")
// rewrite confidence: 0.75
// evidence: types/memory.h heap/heap_block layout, matched field-for-field.
// register convention: heap_block* in EAX (in_EAX), heap* in ECX (in_ECX).

#include "tags.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void heap_unlink_block(heap_block *block, heap *self)
{
    uint32_t slot = block->slot;

    if (block->previous != 0) {
        block->previous->next = block->next;
    }
    if (block->next != 0) {
        block->next->previous = block->previous;
    }
    if (block == self->first_block) {
        self->first_block = block->next;
    }
    if (block == self->last_block) {
        self->last_block = block->previous;
    }
    self->blocks[slot] = 0;
    self->next_free_slot = -(int32_t)(self->first_block != 0) & (int32_t)slot;
}

#if 0
Original Ghidra decompilation (0x4d20a0):

void FUN_004d20a0(void)

{
  uint uVar1;
  int in_EAX;
  int in_ECX;

  uVar1 = *(uint *)(in_EAX + 4);
  if (*(int *)(in_EAX + 8) != 0) {
    *(undefined4 *)(*(int *)(in_EAX + 8) + 0xc) = *(undefined4 *)(in_EAX + 0xc);
  }
  if (*(int *)(in_EAX + 0xc) != 0) {
    *(undefined4 *)(*(int *)(in_EAX + 0xc) + 8) = *(undefined4 *)(in_EAX + 8);
  }
  if (in_EAX == *(int *)(in_ECX + 0x2c)) {
    *(undefined4 *)(in_ECX + 0x2c) = *(undefined4 *)(in_EAX + 0xc);
  }
  if (in_EAX == *(int *)(in_ECX + 0x30)) {
    *(undefined4 *)(in_ECX + 0x30) = *(undefined4 *)(in_EAX + 8);
  }
  *(undefined4 *)(in_ECX + 0x34 + uVar1 * 4) = 0;
  *(uint *)(in_ECX + 0x10) = -(uint)(*(int *)(in_ECX + 0x2c) != 0) & uVar1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
