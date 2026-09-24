// hs_evaluate_boolean_and_or  (Ghidra: hs_evaluate_boolean_and_or, already named)
// address 0x489120, size 296 bytes
// name confidence: 0.5 (out/phase4/hs_functions.md: "Per-call reducer implementing the HS
//   'and'/'or' special forms, short-circuiting the boolean combination based on the selected
//   opcode")
// rewrite confidence: 0.85
// evidence: types/hs.h hs_boolean_state (next_node 0x00, child_value 0x04 low-byte-only,
//   result 0x08; scratch bumps 4+4+1 match exactly); hs_special_function_index-style opcode
//   (5 == and, matching the enum/or family described in hs.h's hs_function_definition notes).
// register convention: opcode in AX (param_1, 5 for 'and', anything else observed here is 'or');
//   thread index as the recognized stack parameter (param_2); `first` as param_3.
// UNSURE: FUN_0048a560/FUN_0048a640 argument reconstruction, see hs_evaluate_random.c.

// FIXED (verified against the retail bytes): hs_thread_push's third argument (EBX) is NOT
// frame->result_address -- that field belongs to the frame's own parent and is written BY
// push, not read by it. Each evaluate handler hands push the address of its own result slot;
// see the call site below for the instruction that proves which one.
// FIXED (verified against the retail bytes): an evaluated call node's first child is the function-name
//   node; the original starts at its next_node ([first_child*0x14 + 8]), so arguments begin at the second
//   child. The draft started at the name node itself (scripts ran with misaligned arguments).
#include "tags.h"
#include "memory.h"
#include "hs.h"

extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address);
    // this module, 0x48a560; see hs_evaluate_random.c
extern void hs_thread_return(int32_t value, uint32_t thread_index); // this module, 0x48a640

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474

// Evaluate handler for 'and' (opcode 5) and 'or' (any other opcode reaching this function).
// Reduces the special form's child list left to right, short-circuiting as soon as the running
// result can no longer change (false for 'and', true for 'or').
void hs_evaluate_boolean_and_or(int16_t opcode, uint32_t thread_index, char first)
{
    hs_thread *thread_record;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    datum_index *next_node_slot;
    char *child_value; // only the low byte of this 4-byte scratch field is read
    char *result;
    char is_and;
    char child_result;

    thread_record = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    frame = thread_record->stack;
    next_node_slot = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    child_value = (char *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    result = (char *)((uint8_t *)frame + 0x0e + frame->size);
    is_and = (opcode == 5);
    frame->size = frame->size + 1;

    if (first != 0) {
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
            (frame->syntax_node & 0xffff) * 0x14);
        *next_node_slot = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & 0xffff) * 0x14))->next_node;
        *result = is_and;
        goto check_continue;
    }

    child_result = *child_value;
    if (is_and) {
        if (*result != 0 && child_result != 0) {
            child_result = 1;
        } else {
            child_result = 0;
        }
    } else {
        if (*result == 0 && child_result == 0) {
            child_result = 0;
        } else {
            child_result = 1;
        }
    }
    *result = child_result;

check_continue:
    if (*next_node_slot != k_datum_index_none && (*result != 0) == (is_and != 0)) {
        hs_thread_push(*next_node_slot, thread_index, child_value);
            // EBX = [esp+0x10] = scratch 2 (child_value), 0x4891f5
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (*next_node_slot & 0xffff) * 0x14);
        *next_node_slot = node->next_node;
        return;
    }
    hs_thread_return(*result, thread_index);
}

#if 0
Original Ghidra decompilation (0x489120):

void hs_evaluate_boolean_and_or(short param_1,uint param_2,char param_3)

{
  uint *puVar1;
  char *pcVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  char cVar7;
  int iVar8;
  bool bVar9;

  iVar6 = DAT_0087a470;
  iVar8 = (param_2 & 0xffff) * 0x218;
  iVar4 = *(int *)(DAT_0087a470 + 0x34);
  iVar5 = *(int *)(iVar4 + 0x10 + iVar8);
  puVar1 = (uint *)(*(short *)(iVar5 + 0xc) + 0xe + iVar5);
  *(short *)(iVar5 + 0xc) = *(short *)(iVar5 + 0xc) + 4;
  iVar5 = *(int *)(*(int *)(iVar6 + 0x34) + 0x10 + iVar8);
  sVar3 = *(short *)(iVar5 + 0xc);
  *(short *)(iVar5 + 0xc) = sVar3 + 4;
  iVar6 = *(int *)(*(int *)(iVar6 + 0x34) + 0x10 + iVar8);
  pcVar2 = (char *)(*(short *)(iVar6 + 0xc) + 0xe + iVar6);
  bVar9 = param_1 == 5;
  *(short *)(iVar6 + 0xc) = *(short *)(iVar6 + 0xc) + 1;
  if (param_3 != '\0') {
    *puVar1 = *(uint *)(*(int *)(DAT_0087a474 + 0x34) + 8 +
                       (*(uint *)(*(int *)(DAT_0087a474 + 0x34) + 0x10 +
                                 (*(uint *)(*(int *)(iVar4 + 0x10 + iVar8) + 4) & 0xffff) * 0x14) &
                       0xffff) * 0x14);
    *pcVar2 = bVar9;
    goto LAB_004891e9;
  }
  cVar7 = *(char *)(sVar3 + 0xe + iVar5);
  if (bVar9) {
    if ((*pcVar2 != '\0') && (cVar7 != '\0')) goto LAB_004891e2;
LAB_0048922a:
    cVar7 = '\0';
  }
  else {
    if ((*pcVar2 == '\0') && (cVar7 == '\0')) goto LAB_0048922a;
LAB_004891e2:
    cVar7 = '\x01';
  }
  *pcVar2 = cVar7;
LAB_004891e9:
  if ((*puVar1 != 0xffffffff) && ((bool)*pcVar2 == bVar9)) {
    FUN_0048a560();
    *puVar1 = *(uint *)(*(int *)(DAT_0087a474 + 0x34) + 8 + (*puVar1 & 0xffff) * 0x14);
    return;
  }
  FUN_0048a640();
  return;
}
#endif
