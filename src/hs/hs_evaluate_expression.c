// hs_evaluate_expression  (Ghidra: FUN_0048a250; renamed per out/phase4/hs_types_notes.md:
// "hs_evaluate_expression (0x48a250) creates type 2 and hs_thread_evaluate deletes a type-2
// thread when its stack empties")
// address 0x48a250, size 124 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: types/hs.h hs_thread (flags 0x03, result 0x14) and hs_thread_type
//   (_hs_thread_command = 2).
// register convention: none (void); syntax node index is the recognized stack parameter
//   (param_1).
// UNSURE: the flags-bit check immediately after the push (before any evaluate step has run)
// reads as "if a frame was successfully pushed, step it once and report failure (-1); otherwise
// return the thread's already-final result" -- which is a strange shape for something meant to
// evaluate synchronously "to completion" per its summary, since a freshly pushed frame should
// need many steps, not one, to finish for most expressions. Preserved exactly regardless of
// whether that reading is right.

// VERIFIED instruction by instruction against 0x48a250..0x48a2cb. Confirmations worth noting:
// hs_thread_new is called as (EAX = -1 script index, stack = 2 == _hs_thread_command); the
// result address handed to hs_thread_push really is &thread->result (`lea ebx,[esi+0x14]`,
// 0x48a297) -- this is the one push site in the module that uses that field, because the caller
// is outside the thread system; and a thread that had to push a frame returns -1 rather than a
// value (`or eax,0xffffffff`, 0x48a2b5), so a blocking expression reports no result.
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


    // this module, 0x48a560; UNSURE, see hs_evaluate_random.c


extern uint8_t hs_runtime_active;  // 0x006b15e8
extern data_array *hs_thread_data; // 0x0087a470

int32_t hs_evaluate_expression(datum_index node)
{
    datum_index thread_handle;
    hs_thread *thread;

    if (hs_runtime_active == 0 || node == k_datum_index_none) {
        return -1;
    }
    thread_handle = hs_thread_new(-1, 2); // _hs_thread_command
    if (thread_handle == k_datum_index_none) {
        return -1;
    }
    thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_handle & 0xffff) * 0x218);
    hs_thread_push(node, thread_handle, &thread->result);
    if ((thread->flags & 1) != 0) {
        hs_thread_evaluate_step(thread_handle);
        return -1;
    }
    return thread->result;
}

#if 0
Original Ghidra decompilation (0x48a250):

undefined4 FUN_0048a250(int param_1)

{
  uint uVar1;
  int iVar2;

  if ((DAT_006b15e8 == '\0') || (param_1 == -1)) {
    return 0xffffffff;
  }
  uVar1 = hs_thread_new(2);
  if (uVar1 == 0xffffffff) {
    return 0xffffffff;
  }
  iVar2 = (uVar1 & 0xffff) * 0x218 + *(int *)(DAT_0087a470 + 0x34);
  FUN_0048a560();
  if ((*(byte *)(iVar2 + 3) & 1) != 0) {
    FUN_0048a370(uVar1);
    return 0xffffffff;
  }
  return *(undefined4 *)(iVar2 + 0x14);
}
#endif
