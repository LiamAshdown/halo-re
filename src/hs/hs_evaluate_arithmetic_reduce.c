// hs_evaluate_arithmetic_reduce  (Ghidra: hs_evaluate_arithmetic_reduce, already named)
// address 0x489250, size 373 bytes
// name confidence: 0.55 (out/phase4/hs_functions.md: "Per-call reducer implementing the HS
//   numeric '+','-','*','/','min' and 'max' special forms over a variadic argument list")
// rewrite confidence: 0.85
// evidence: types/hs.h hs_arithmetic_state (term_count 0x00, next_node 0x02, child_value 0x06,
//   accumulator 0x0a; scratch bumps 2+4+4+4 match exactly, with child_value captured only as an
//   offset (sVar6) rather than a live pointer, exactly as decompiled).
// register convention: opcode in AX (param_1: 7 + - 8 - * 9, 10 /, 0xb min, 0xc max); thread
//   index as the recognized stack parameter (param_2); `first` as param_3.
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
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address);
    // this module, 0x48a560; see hs_evaluate_random.c
extern void hs_thread_return(int32_t value, uint32_t thread_index); // this module, 0x48a640

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474

// Evaluate handler shared by the '+', '-', '*', '/', 'min' and 'max' special forms. Left-folds
// the special form's child list: the first child's value seeds the accumulator, and each
// subsequent child combines into it per `opcode`.
void hs_evaluate_arithmetic_reduce(int16_t opcode, uint32_t thread_index, char first)
{
    hs_thread *thread_record;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    int16_t *term_count;
    datum_index *next_node_slot;
    float *child_value;
    float *accumulator;
    float value;

    thread_record = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    frame = thread_record->stack;
    term_count = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;

    frame = thread_record->stack;
    next_node_slot = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    child_value = (float *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    accumulator = (float *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    if (first != 0) {
        *term_count = 0;
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
            (frame->syntax_node & 0xffff) * 0x14);
        *next_node_slot = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & 0xffff) * 0x14))->next_node;
        goto check_continue;
    }

    value = *child_value;
    if (*term_count == 0) {
        *accumulator = value;
    } else {
        switch (opcode) {
        case 7: *accumulator = value + *accumulator; break;
        case 8: *accumulator = *accumulator - value; break;
        case 9: *accumulator = value * *accumulator; break;
        case 10: *accumulator = *accumulator / value; break;
        case 0xb: if (value < *accumulator) *accumulator = value; break;
        case 0xc: if (*accumulator < value) *accumulator = value; break;
        }
    }
    *term_count = *term_count + 1;

check_continue:
    if (*next_node_slot == k_datum_index_none) {
        hs_thread_return(*(int32_t *)accumulator, thread_index);
        return;
    }
    hs_thread_push(*next_node_slot, thread_index, child_value); // EBX = scratch 3 (0x489359)
    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (*next_node_slot & 0xffff) * 0x14);
    *next_node_slot = node->next_node;
}

#if 0
Original Ghidra decompilation (0x489250):

void hs_evaluate_arithmetic_reduce(undefined2 param_1,uint param_2,char param_3)

{
  int *piVar1;
  short *psVar2;
  uint *puVar3;
  float *pfVar4;
  float fVar5;
  short sVar6;
  int iVar7;
  int iVar8;
  int iVar9;

  iVar8 = DAT_0087a470;
  iVar9 = (param_2 & 0xffff) * 0x218;
  piVar1 = (int *)(*(int *)(DAT_0087a470 + 0x34) + 0x10 + iVar9);
  iVar7 = *piVar1;
  psVar2 = (short *)(*(short *)(iVar7 + 0xc) + 0xe + iVar7);
  *(short *)(iVar7 + 0xc) = *(short *)(iVar7 + 0xc) + 2;
  iVar7 = *(int *)(*(int *)(iVar8 + 0x34) + 0x10 + iVar9);
  puVar3 = (uint *)(*(short *)(iVar7 + 0xc) + 0xe + iVar7);
  *(short *)(iVar7 + 0xc) = *(short *)(iVar7 + 0xc) + 4;
  iVar7 = *(int *)(*(int *)(iVar8 + 0x34) + 0x10 + iVar9);
  sVar6 = *(short *)(iVar7 + 0xc);
  *(short *)(iVar7 + 0xc) = sVar6 + 4;
  iVar8 = *(int *)(*(int *)(iVar8 + 0x34) + 0x10 + iVar9);
  pfVar4 = (float *)(*(short *)(iVar8 + 0xc) + 0xe + iVar8);
  *(short *)(iVar8 + 0xc) = *(short *)(iVar8 + 0xc) + 4;
  iVar8 = DAT_0087a474;
  if (param_3 != '\0') {
    *psVar2 = 0;
    *puVar3 = *(uint *)(*(int *)(iVar8 + 0x34) + 8 +
                       (*(uint *)(*(int *)(iVar8 + 0x34) + 0x10 +
                                 (*(uint *)(*piVar1 + 4) & 0xffff) * 0x14) & 0xffff) * 0x14);
    goto LAB_0048934d;
  }
  fVar5 = *(float *)(sVar6 + 0xe + iVar7);
  if (*psVar2 == 0) {
    *pfVar4 = fVar5;
    goto switchD_00489335_default;
  }
  switch(param_1) {
  case 7:
    *pfVar4 = fVar5 + *pfVar4;
    break;
  case 8:
    fVar5 = *pfVar4 - fVar5;
    goto LAB_00489346;
  case 9:
    *pfVar4 = fVar5 * *pfVar4;
    break;
  case 10:
    fVar5 = *pfVar4 / fVar5;
LAB_00489346:
    *pfVar4 = fVar5;
    break;
  case 0xb:
    if (fVar5 < *pfVar4) goto LAB_004893a9;
    *pfVar4 = *pfVar4;
    break;
  case 0xc:
    if (fVar5 < *pfVar4) {
      fVar5 = *pfVar4;
    }
LAB_004893a9:
    *pfVar4 = fVar5;
  }
switchD_00489335_default:
  *psVar2 = *psVar2 + 1;
LAB_0048934d:
  if (*puVar3 == 0xffffffff) {
    FUN_0048a640();
    return;
  }
  FUN_0048a560();
  *puVar3 = *(uint *)(*(int *)(DAT_0087a474 + 0x34) + 8 + (*puVar3 & 0xffff) * 0x14);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
