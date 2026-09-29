// hs_evaluate_variadic_arguments  (Ghidra: hs_evaluate_variadic_arguments, already named)
// address 0x48ad60, size 338 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/hs.h hs_variadic_arguments_state (evaluated_count 0x00, values[32] 0x04,
//   argument_count 0x84, next_node 0x86; scratch bumps 4+0x80+2+4 match exactly).
// register convention: none (void); all four parameters are ordinary recognized stack
//   parameters. `value` (param_2) doubles as the "first call" flag via its low byte, and on the
//   first call is still stored into values[0] (same off-by-one quirk as
//   hs_evaluate_argument_list.c, preserved rather than "fixed").
// UNSURE: FUN_0048a560 (hs_thread_push) is called here with zero visible arguments; bindings
// follow the pattern established in hs_evaluate_random.c.

// FIXED (verified against the retail bytes at 0x48ae4a..0x48ae7a): hs_thread_push's third
// argument (EBX) is `&value` -- the address of this function's own second stack parameter -- not
// frame->result_address. `value` carries the `first` flag in, and the evaluated child's result
// back out; the slot is re-read immediately after the push and copied into values[].
// OPEN QUESTION (hook verification): that address is on the C stack, so it is only live while
// this call frame is. It works because the call chain hs_thread_evaluate_step -> the built-in's
// evaluate handler -> this collector has a fixed frame layout, so the slot lands at the same
// address on every resumption, and hs_thread_return's write (through the parent hs frame's
// result_address, recorded by push) is the last thing the child's handler does before unwinding
// back to that same depth. Worth confirming with a hook before relying on it.
// FIXED (verified against the retail bytes): an evaluated call node's first child is the function-name
//   node; the original starts at its next_node ([first_child*0x14 + 8]), so arguments begin at the second
//   child. The draft started at the name node itself (scripts ran with misaligned arguments).
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


    // this module, 0x48a560; UNSURE, see header note

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474

// Generic variadic argument collector, called once per evaluated child. Accumulates up to 32
// values; once the child list is exhausted (or the limit is hit), writes the final count and
// values pointer to `*out_count`/`*out_values` and returns 1. Otherwise pushes the next child for
// evaluation and returns 0.
char hs_evaluate_variadic_arguments(uint32_t thread_index, int32_t value, uint32_t *out_count,
    int32_t **out_values)
{
    hs_thread *thread;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    int32_t *evaluated_count;
    int32_t *values;
    int16_t *argument_count;
    datum_index *next_node_slot;
    int i;

    thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    frame = thread->stack;
    evaluated_count = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread->stack;
    values = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 0x80;

    frame = thread->stack;
    argument_count = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;

    frame = thread->stack;
    next_node_slot = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    if ((char)value != 0) {
        *argument_count = 0;
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
            (frame->syntax_node & 0xffff) * 0x14);
        *next_node_slot = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & 0xffff) * 0x14))->next_node;
        *evaluated_count = 0;
        for (i = 0; i < 0x20; i++) {
            values[i] = 0;
        }
    }

    if (*next_node_slot != k_datum_index_none && *argument_count < 0x20) {
        /* 0x48ae4e: `lea ebx,[esp+0x20]` -- the address of this function's OWN second stack
           parameter, i.e. the `value` slot, which has already been consumed as the `first` flag.
           The child's result is read back out of it immediately after the push. See the
           stack-hand-off note at the top of the file. */
        hs_thread_push(*next_node_slot, thread_index, &value);
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (*next_node_slot & 0xffff) * 0x14);
        *next_node_slot = node->next_node;
        values[*evaluated_count] = value; /* re-read after the push (0x48ae6d) */
        *argument_count = *argument_count + 1;
        *evaluated_count = *evaluated_count + 1;
        return 0;
    }

    *out_count = *evaluated_count;
    *out_values = values;
    return 1;
}

#if 0
Original Ghidra decompilation (0x48ad60):

uint hs_evaluate_variadic_arguments
               (uint param_1,undefined4 param_2,uint *param_3,undefined4 *param_4)

{
  int *piVar1;
  uint *puVar2;
  undefined4 *puVar3;
  short *psVar4;
  uint *puVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  undefined4 *puVar10;

  iVar7 = DAT_0087a470;
  iVar8 = (param_1 & 0xffff) * 0x218;
  piVar1 = (int *)(*(int *)(DAT_0087a470 + 0x34) + 0x10 + iVar8);
  iVar9 = *piVar1;
  puVar2 = (uint *)(*(short *)(iVar9 + 0xc) + 0xe + iVar9);
  *(short *)(iVar9 + 0xc) = *(short *)(iVar9 + 0xc) + 4;
  iVar9 = *(int *)(*(int *)(iVar7 + 0x34) + 0x10 + iVar8);
  puVar3 = (undefined4 *)(*(short *)(iVar9 + 0xc) + 0xe + iVar9);
  *(short *)(iVar9 + 0xc) = *(short *)(iVar9 + 0xc) + 0x80;
  iVar9 = *(int *)(*(int *)(iVar7 + 0x34) + 0x10 + iVar8);
  psVar4 = (short *)(*(short *)(iVar9 + 0xc) + 0xe + iVar9);
  *(short *)(iVar9 + 0xc) = *(short *)(iVar9 + 0xc) + 2;
  iVar9 = *(int *)(*(int *)(iVar7 + 0x34) + 0x10 + iVar8);
  puVar5 = (uint *)(*(short *)(iVar9 + 0xc) + 0xe + iVar9);
  *(short *)(iVar9 + 0xc) = *(short *)(iVar9 + 0xc) + 4;
  iVar9 = DAT_0087a474;
  if ((char)param_2 != '\0') {
    *psVar4 = 0;
    *puVar5 = *(uint *)(*(int *)(iVar9 + 0x34) + 8 +
                       (*(uint *)(*(int *)(iVar9 + 0x34) + 0x10 +
                                 (*(uint *)(*piVar1 + 4) & 0xffff) * 0x14) & 0xffff) * 0x14);
    *puVar2 = 0;
    puVar10 = puVar3;
    for (iVar9 = 0x20; iVar9 != 0; iVar9 = iVar9 + -1) {
      *puVar10 = 0;
      puVar10 = puVar10 + 1;
    }
  }
  if ((*puVar5 != 0xffffffff) && (*psVar4 < 0x20)) {
    FUN_0048a560();
    *puVar5 = *(uint *)(*(int *)(DAT_0087a474 + 0x34) + 8 + (*puVar5 & 0xffff) * 0x14);
    puVar3[*puVar2] = param_2;
    *psVar4 = *psVar4 + 1;
    uVar6 = *puVar2;
    *puVar2 = uVar6 + 1;
    return uVar6 + 1 & 0xffffff00;
  }
  *param_3 = *puVar2;
  *param_4 = puVar3;
  return CONCAT31((int3)((uint)puVar3 >> 8),1);
}
#endif
