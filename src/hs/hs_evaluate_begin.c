// hs_evaluate_begin  (not a Ghidra function; the evaluate handler of hs function 0 "begin")
// address 0x488b90, size 197 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[0] -> record 0x657660, name "begin", evaluate
//   (+0xc) 0x488b90; only reachable through that pointer (hs_thread_evaluate_step 0x48a370
//   calls definition->evaluate). First-boot track: the UI map's scripts run it.
// objdump 0x488b90..0x488c54: two 4-byte scratch slots (next expression, result). On the first
//   call the next expression is the call node's second child (the first is the function name)
//   and the result 0; then while an expression is left it is pushed with the result slot as its
//   result address (EBX) and the cursor moves to its next_node; once none is left the result
//   is returned (hs_thread_return, ECX = thread).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address); // 0x48a560
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474

static hs_syntax_node *syntax_get(datum_index node)
{
    return (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node & 0xffff) * 0x14);
}

void hs_evaluate_begin(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread *thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    hs_stack_frame *frame;
    datum_index *next_expression;
    int32_t *result;

    frame = thread->stack;
    next_expression = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread->stack;
    result = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    if (first != 0) {
        *next_expression = syntax_get(syntax_get(thread->stack->syntax_node)->data.first_child)->next_node;
        *result = 0;
    }
    if (*next_expression != k_datum_index_none) {
        hs_thread_push(*next_expression, thread_index, result);
        *next_expression = syntax_get(*next_expression)->next_node;
        return;
    }
    hs_thread_return(*result, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
