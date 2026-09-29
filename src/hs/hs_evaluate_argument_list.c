// hs_evaluate_argument_list  (Ghidra: FUN_00489d50)
// address 0x489d50, size 278 bytes
// name confidence: 0.35 (out/phase4/hs_functions.md: "Collects up to 32 evaluated argument
//   values into a fixed-size array, one value per reentrant call, for a variadic HS operation")
// rewrite confidence: 0.85
// evidence: types/hs.h hs_argument_list_state (next_node 0x00, count 0x04, values[32] 0x08;
//   scratch bumps 4+4+0x80 match exactly).
// register convention: unused function-index-ish param_1; thread index as the recognized stack
//   parameter (param_2); the evaluated child's value (param_3) doubles as the "first call" flag
//   via its low byte, exactly as decompiled -- on the first call this value (typically 1) is
//   still stored into values[0] and count becomes 1, which looks like an off-by-one quirk but is
//   preserved exactly rather than "fixed".
// UNSURE: FUN_0048a560/FUN_0048a640 argument reconstruction, see hs_evaluate_random.c.

// FIXED (verified against the retail bytes at 0x489e12..0x489e5b): hs_thread_push's third
// argument (EBX) is `&value`, this function's own third stack parameter, re-read immediately
// after the push -- not frame->result_address. And the terminal hs_thread_return takes -1
// (`or eax,0xffffffff`, 0x489e58), not 0.
// OPEN QUESTION (hook verification): see hs_evaluate_variadic_arguments.c -- the result address
// handed to push lives on the C stack, which only works because the call chain's frame layout is
// fixed across resumptions.
// FIXED (verified against the retail bytes): an evaluated call node's first child is the function-name
//   node; the original starts at its next_node ([first_child*0x14 + 8]), so arguments begin at the second
//   child. The draft started at the name node itself (scripts ran with misaligned arguments).
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


    // this module, 0x48a560; see hs_evaluate_random.c


extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474

void hs_evaluate_argument_list(uint32_t unused_param_1, uint32_t thread_index, int32_t value)
{
    hs_thread *thread_record;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    datum_index *next_node_slot;
    int32_t *count;
    int32_t *values;
    int32_t i;

    thread_record = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    frame = thread_record->stack;
    next_node_slot = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    count = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    values = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 0x80;

    if ((char)value != 0) {
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
            (frame->syntax_node & 0xffff) * 0x14);
        *next_node_slot = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & 0xffff) * 0x14))->next_node;
        *count = 0;
        for (i = 0; i < 0x20; i++) {
            values[i] = 0;
        }
    }

    if (*next_node_slot != k_datum_index_none && *count < 0x20) {
        /* 0x489e18: `lea ebx,[esp+0x20]` -- the address of this function's own third stack
           parameter (the `value` slot, already consumed as the `first` flag). Same C-stack
           hand-off as hs_evaluate_variadic_arguments; see the note at the top of the file. */
        hs_thread_push(*next_node_slot, thread_index, &value);
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (*next_node_slot & 0xffff) * 0x14);
        *next_node_slot = node->next_node;
        values[*count] = value; /* re-read after the push (0x489e40) */
        *count = *count + 1;
        return;
    }
    hs_thread_return(-1, thread_index); // `or eax,0xffffffff` at 0x489e58, not 0
}

#if 0
Original Ghidra decompilation (0x489d50):

void FUN_00489d50(undefined4 param_1,uint param_2,undefined4 param_3)

{
  int *piVar1;
  uint *puVar2;
  int *piVar3;
  undefined4 *puVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;

  iVar5 = DAT_0087a470;
  iVar6 = (param_2 & 0xffff) * 0x218;
  piVar1 = (int *)(*(int *)(DAT_0087a470 + 0x34) + 0x10 + iVar6);
  iVar7 = *piVar1;
  puVar2 = (uint *)(*(short *)(iVar7 + 0xc) + 0xe + iVar7);
  *(short *)(iVar7 + 0xc) = *(short *)(iVar7 + 0xc) + 4;
  iVar7 = *(int *)(*(int *)(iVar5 + 0x34) + 0x10 + iVar6);
  piVar3 = (int *)(iVar7 + 0xe + (int)*(short *)(iVar7 + 0xc));
  *(short *)(iVar7 + 0xc) = *(short *)(iVar7 + 0xc) + 4;
  iVar7 = *(int *)(*(int *)(iVar5 + 0x34) + 0x10 + iVar6);
  puVar4 = (undefined4 *)(*(short *)(iVar7 + 0xc) + 0xe + iVar7);
  *(short *)(iVar7 + 0xc) = *(short *)(iVar7 + 0xc) + 0x80;
  if ((char)param_3 != '\0') {
    *puVar2 = *(uint *)(*(int *)(DAT_0087a474 + 0x34) + 8 +
                       (*(uint *)(*(int *)(DAT_0087a474 + 0x34) + 0x10 +
                                 (*(uint *)(*piVar1 + 4) & 0xffff) * 0x14) & 0xffff) * 0x14);
    *piVar3 = 0;
    puVar8 = puVar4;
    for (iVar7 = 0x20; iVar7 != 0; iVar7 = iVar7 + -1) {
      *puVar8 = 0;
      puVar8 = puVar8 + 1;
    }
  }
  if ((*puVar2 != 0xffffffff) && (*piVar3 < 0x20)) {
    FUN_0048a560();
    *puVar2 = *(uint *)(*(int *)(DAT_0087a474 + 0x34) + 8 + (*puVar2 & 0xffff) * 0x14);
    puVar4[*piVar3] = param_3;
    *piVar3 = *piVar3 + 1;
    return;
  }
  FUN_0048a640();
  return;
}
#endif
