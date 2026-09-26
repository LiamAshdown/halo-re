// hs_evaluate_if  (not a Ghidra function; the evaluate handler of hs function 2 "if")
// address 0x488e60, size 363 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[2] -> record 0x657698, name "if", evaluate (+0xc)
//   0x488e60; only reachable through that pointer. First-boot track: the UI map's scripts run it.
// objdump 0x488e60..0x488fcf: three 4-byte scratch slots (condition, branch, result). The first
//   call zeroes the condition, sets the branch to -1 and pushes the condition (the call node's
//   second child) into the condition slot. Once a branch is chosen (branch != -1) the result is
//   returned. Otherwise the condition byte picks the then-expression (condition's next_node) or
//   the else-expression (then's next_node); a missing else returns 0, a chosen branch is pushed
//   into the result slot.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address); // 0x48a560
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474

static hs_syntax_node *syntax_get(datum_index node)
{
    return (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node & 0xffff) * 0x14);
}

void hs_evaluate_if(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread *thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    hs_stack_frame *frame;
    int32_t *condition;
    datum_index *branch;
    int32_t *result;
    datum_index condition_node;

    frame = thread->stack;
    condition = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread->stack;
    branch = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread->stack;
    result = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    condition_node = syntax_get(syntax_get(thread->stack->syntax_node)->data.first_child)->next_node;
    if (first != 0) {
        *condition = 0;
        *branch = k_datum_index_none;
        hs_thread_push(condition_node, thread_index, condition);
        return;
    }
    if (*branch != k_datum_index_none) {
        hs_thread_return(*result, thread_index);
        return;
    }
    if (*(uint8_t *)condition != 0) {
        *branch = syntax_get(condition_node)->next_node;
    } else {
        *branch = syntax_get(syntax_get(condition_node)->next_node)->next_node;
        if (*branch == k_datum_index_none) {
            hs_thread_return(0, thread_index);
            return;
        }
    }
    hs_thread_push(*branch, thread_index, result);
}
