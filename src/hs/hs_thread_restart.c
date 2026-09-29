// hs_thread_restart  (Ghidra: FUN_0048a790; named per out/phase4/hs_types_notes.md:
// "hs_thread_restart (0x48a790) copies it back into +8 when flag bit 1 is set, then clears the
// bit")
// address 0x48a790, size 192 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: types/hs.h hs_thread (wake_tick 0x08, saved_wake_tick 0x0c, stack 0x10, flags 0x03),
//   hs_stack_frame (previous 0x00, syntax_node 0x04), hs_syntax_node (index_union 0x02),
//   hs_special_function_index (_hs_function_sleep_until = 0x14); hs_thread_pop_frame (this
//   module, 0x48a770).
// register convention: none (void); thread index is the recognized stack parameter (param_1).
// VERIFIED instruction by instruction against 0x48a790..0x48a84f. Note the asymmetry in the two
// sleep_until cases, which is real: when the TOP frame is the sleep_until call, the thread pops
// one frame by writing thread->stack = thread->stack->previous inline (0x48a80a); when it is the
// PARENT frame, it calls hs_thread_pop_frame twice and then clears flag bit 0 (0x48a839..0x48a847).
// UNSURE: why a restarting thread needs that specific unwind when it is sitting on a sleep_until
// call is still not understood beyond what types/hs.h documents about the wake/saved_wake pair.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


    // blam-cc: EAX -> thread_index; this module, 0x48a770

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474

// Restarts or resumes an existing (possibly dormant) HS thread. If its wake/saved-wake flag is
// set, restores wake_tick from saved_wake_tick and clears the flag. Otherwise, if the thread's
// current (or parent) frame is evaluating a call to sleep_until, pops one or two frames off its
// stack so the next evaluate step re-enters the sleep_until call correctly.
void hs_thread_restart(uint32_t thread_index)
{
    hs_thread *thread;
    hs_syntax_node *node;
    hs_stack_frame *parent_frame;
    datum_index syntax_node;

    thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    if (thread->wake_tick == -1) {
        return;
    }

    thread->wake_tick = 0;
    if ((thread->flags & 2) != 0) {
        thread->wake_tick = thread->saved_wake_tick;
        thread->flags = thread->flags & 0xfd;
        return;
    }

    syntax_node = thread->stack->syntax_node;
    if (syntax_node != k_datum_index_none) {
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (syntax_node & 0xffff) * 0x14);
        if (node->index_union == _hs_function_sleep_until) {
            thread->stack = thread->stack->previous;
            return;
        }
    }

    parent_frame = thread->stack->previous;
    if (parent_frame != 0) {
        syntax_node = parent_frame->syntax_node;
        if (syntax_node != k_datum_index_none) {
            node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
                (syntax_node & 0xffff) * 0x14);
            if (node->index_union == _hs_function_sleep_until) {
                hs_thread_pop_frame(thread_index);
                hs_thread_pop_frame(thread_index);
                thread->flags = thread->flags & 0xfe;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x48a790):

void FUN_0048a790(uint param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;

  iVar2 = DAT_0087a470;
  iVar3 = (param_1 & 0xffff) * 0x218;
  iVar4 = *(int *)(DAT_0087a470 + 0x34) + iVar3;
  if (*(int *)(*(int *)(DAT_0087a470 + 0x34) + 8 + iVar3) != -1) {
    *(undefined4 *)(iVar4 + 8) = 0;
    if ((*(byte *)(iVar4 + 3) & 2) != 0) {
      *(undefined4 *)(iVar4 + 8) = *(undefined4 *)(iVar4 + 0xc);
      *(byte *)(iVar4 + 3) = *(byte *)(iVar4 + 3) & 0xfd;
      return;
    }
    uVar1 = (*(int **)(iVar4 + 0x10))[1];
    if ((uVar1 != 0xffffffff) &&
       (*(short *)(*(int *)(DAT_0087a474 + 0x34) + 2 + (uVar1 & 0xffff) * 0x14) == 0x14)) {
      *(undefined4 *)(*(int *)(iVar2 + 0x34) + iVar3 + 0x10) =
           **(undefined4 **)(*(int *)(iVar2 + 0x34) + 0x10 + iVar3);
      return;
    }
    iVar2 = **(int **)(iVar4 + 0x10);
    if (((iVar2 != 0) && (uVar1 = *(uint *)(iVar2 + 4), uVar1 != 0xffffffff)) &&
       (*(short *)(*(int *)(DAT_0087a474 + 0x34) + 2 + (uVar1 & 0xffff) * 0x14) == 0x14)) {
      FUN_0048a770();
      FUN_0048a770();
      *(byte *)(iVar4 + 3) = *(byte *)(iVar4 + 3) & 0xfe;
    }
  }
  return;
}
#endif
