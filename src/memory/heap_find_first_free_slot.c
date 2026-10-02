// heap_find_first_free_slot  (Ghidra: FUN_004d2110)
// address 0x4d2110, size 37 bytes
// name confidence: 0.9 (out/phase4/memory_types_notes.md: "Suggested renames: FUN_004d2110 ->
// heap_find_first_free_slot")
// rewrite confidence: 0.85
// evidence: types/memory.h heap (maximum_blocks at +0x0c, blocks[] at +0x34); notes.md's
// correction that this trailing array is "an allocation slot table with one entry per live
// block", not size-class free lists.
// register convention: heap* in EAX (in_EAX).

#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

uint32_t heap_find_first_free_slot(heap *self)
{
    uint32_t slot = 0xffffffff;

    if (self->maximum_blocks != 0) {
        heap_block **entry = &self->blocks[0];
        slot = 0;
        while (*entry != 0) {
            slot = slot + 1;
            entry = entry + 1;
            if ((uint32_t)self->maximum_blocks <= slot) {
                return 0xffffffff;
            }
        }
    }
    return slot;
}

#if 0
Original Ghidra decompilation (0x4d2110):

uint FUN_004d2110(void)

{
  int in_EAX;
  uint uVar1;
  int *piVar2;

  uVar1 = 0xffffffff;
  if (*(uint *)(in_EAX + 0xc) != 0) {
    piVar2 = (int *)(in_EAX + 0x34);
    uVar1 = 0;
    while (*piVar2 != 0) {
      uVar1 = uVar1 + 1;
      piVar2 = piVar2 + 1;
      if (*(uint *)(in_EAX + 0xc) <= uVar1) {
        return 0xffffffff;
      }
    }
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
