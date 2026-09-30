// heap_advance_free_slot  (Ghidra: FUN_004d2140)
// address 0x4d2140, size 55 bytes
// name confidence: 0.9 (out/phase4/memory_types_notes.md: "Suggested renames: ... FUN_004d2140 ->
// heap_advance_free_slot")
// rewrite confidence: 0.85
// evidence: types/memory.h heap.
// register convention: heap* in ECX (in_ECX).

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"

void heap_advance_free_slot(heap *self)
{
    int32_t slot;
    heap_block **entry;

    if (self->next_free_slot == -1) {
        return;
    }
    slot = self->next_free_slot + 1;
    self->next_free_slot = -1;
    if (slot < self->maximum_blocks) {
        entry = &self->blocks[slot];
        while (*entry != 0) {
            slot = slot + 1;
            entry = entry + 1;
            if (self->maximum_blocks <= slot) {
                return;
            }
        }
        self->next_free_slot = slot;
    }
}

#if 0
Original Ghidra decompilation (0x4d2140):

void FUN_004d2140(void)

{
  int iVar1;
  int in_ECX;
  int *piVar2;

  if (*(int *)(in_ECX + 0x10) != -1) {
    iVar1 = *(int *)(in_ECX + 0x10) + 1;
    *(undefined4 *)(in_ECX + 0x10) = 0xffffffff;
    if (iVar1 < *(int *)(in_ECX + 0xc)) {
      piVar2 = (int *)(in_ECX + 0x34 + iVar1 * 4);
      while (*piVar2 != 0) {
        iVar1 = iVar1 + 1;
        piVar2 = piVar2 + 1;
        if (*(int *)(in_ECX + 0xc) <= iVar1) {
          return;
        }
      }
      *(int *)(in_ECX + 0x10) = iVar1;
    }
  }
  return;
}
#endif
