// hs_thread_pop_frame  (Ghidra: FUN_0048a770; named per out/phase4/hs_types_notes.md:
// "hs_thread_pop_frame (0x48a770) does thread->stack = *thread->stack")
// address 0x48a770, size 31 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: types/hs.h hs_stack_frame (previous at 0x00) and hs_thread (stack at 0x10).
// register convention: thread index in EAX (in_EAX).
//   // blam-cc: EAX -> thread_index

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern data_array *hs_thread_data; // 0x0087a470

// Pops the current frame off `thread_index`'s evaluation stack, returning to its parent.
void hs_thread_pop_frame(uint32_t thread_index)
{
    hs_thread *thread;

    thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    thread->stack = thread->stack->previous;
}

#if 0
Original Ghidra decompilation (0x48a770):

void FUN_0048a770(void)

{
  uint in_EAX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x218 + *(int *)(DAT_0087a470 + 0x34);
  *(undefined4 *)(iVar1 + 0x10) = **(undefined4 **)(iVar1 + 0x10);
  return;
}
#endif
