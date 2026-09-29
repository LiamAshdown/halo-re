// hs_evaluate_typed_arguments  (Ghidra: FUN_0048a850)
// address 0x48a850, size 265 bytes
// name confidence: 0.45 (out/phase4/hs_functions.md: "Evaluates a fixed-arity, statically typed
//   argument list one argument per call, validating each against an expected-type array before
//   returning the completed buffer")
// rewrite confidence: 0.85
// evidence: types/hs.h hs_typed_arguments_state (results[parameter_count] at 0x00, then index and
//   next_node implicitly following at 4*parameter_count and 4*parameter_count+2; scratch bumps
//   parameter_count*4 + 2 + 4 match exactly).
// register convention: none (void); all four parameters are ordinary recognized stack parameters.
// UNSURE: FUN_0048a560 (hs_thread_push) is called here with zero visible arguments; the node and
// thread-index bindings below follow the same pattern established in hs_evaluate_random.c.

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
#include "fn_hs.h"


    // this module, 0x48a560; UNSURE, see header note

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474

// Evaluates `parameter_count` fixed-position arguments of the syntax node being evaluated by
// `thread_index`, one per call, validating each child's declared type against
// `expected_types[index]` before pushing it. Returns the completed `int32_t[parameter_count]`
// results buffer once every argument has been evaluated (or the next one fails its type check),
// or NULL if there is still more to evaluate.
int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first)
{
    hs_thread *thread;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    int32_t *results;
    int16_t *index;
    datum_index *next_node_slot;
    int32_t *done;

    thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    frame = thread->stack;
    results = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    done = results;
    frame->size = frame->size + parameter_count * 4;

    frame = thread->stack;
    index = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;

    frame = thread->stack;
    next_node_slot = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    if (first != 0) {
        *index = 0;
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
            (frame->syntax_node & 0xffff) * 0x14);
        *next_node_slot = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & 0xffff) * 0x14))->next_node;
    }

    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (*next_node_slot & 0xffff) * 0x14);
    if (*index < parameter_count && node->type == expected_types[*index]) {
        hs_thread_push(*next_node_slot, thread_index, &results[*index]);
            // EBX = lea [results + index*4] (0x48a928)
        *next_node_slot = node->next_node;
        *index = *index + 1;
        done = 0;
    }
    return done;
}

#if 0
Original Ghidra decompilation (0x48a850):

int FUN_0048a850(uint param_1,short param_2,int param_3,char param_4)

{
  short *psVar1;
  uint *puVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;

  iVar5 = DAT_0087a470;
  iVar8 = (param_1 & 0xffff) * 0x218;
  iVar3 = *(int *)(DAT_0087a470 + 0x34);
  iVar4 = *(int *)(iVar3 + 0x10 + iVar8);
  iVar7 = *(short *)(iVar4 + 0xc) + 0xe + iVar4;
  *(short *)(iVar4 + 0xc) = *(short *)(iVar4 + 0xc) + param_2 * 4;
  iVar4 = *(int *)(*(int *)(iVar5 + 0x34) + 0x10 + iVar8);
  psVar1 = (short *)(iVar4 + 0xe + (int)*(short *)(iVar4 + 0xc));
  *(short *)(iVar4 + 0xc) = *(short *)(iVar4 + 0xc) + 2;
  iVar6 = DAT_0087a474;
  iVar4 = *(int *)(*(int *)(iVar5 + 0x34) + 0x10 + iVar8);
  puVar2 = (uint *)(*(short *)(iVar4 + 0xc) + 0xe + iVar4);
  *(short *)(iVar4 + 0xc) = *(short *)(iVar4 + 0xc) + 4;
  if (param_4 != '\0') {
    *psVar1 = 0;
    iVar4 = *(int *)(iVar6 + 0x34);
    *puVar2 = *(uint *)(iVar4 + 8 +
                       (*(uint *)(iVar4 + 0x10 +
                                 (*(uint *)(*(int *)(iVar3 + 0x10 + iVar8) + 4) & 0xffff) * 0x14) &
                       0xffff) * 0x14);
  }
  if ((*psVar1 < param_2) &&
     (*(short *)(*(int *)(iVar6 + 0x34) + 4 + (*puVar2 & 0xffff) * 0x14) ==
      *(short *)(param_3 + *psVar1 * 2))) {
    FUN_0048a560();
    *puVar2 = *(uint *)(*(int *)(DAT_0087a474 + 0x34) + 8 + (*puVar2 & 0xffff) * 0x14);
    *psVar1 = *psVar1 + 1;
    iVar7 = 0;
  }
  return iVar7;
}
#endif
