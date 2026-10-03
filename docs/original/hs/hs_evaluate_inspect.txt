// hs_evaluate_inspect  (not a Ghidra function; the evaluate procedure of hs function "inspect" (record 0x6578c8);
//   no C existed, so a console inspect trapped as unlisted_489b80)
// address 0x489b80, size 248 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x489b80..0x489c77. Reserves a 4-byte result slot in the thread's stack frame;
//   on the first pass pushes the argument expression to be evaluated into it (hs_thread_push: EAX node, EDX
//   thread, EBX result). On the next pass the argument's resolved type (syntax node +0x04) selects a formatter
//   from the per-type table at 0x68bb48, called as (type, value, 1024-byte buffer); when there is one, the text is
//   printed to the console (console_out, EAX 0 = default colour, the buffer as the format) if byte 0x7102fd is
//   set or the debug log level is at least 4. Returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address); // 0x48a560
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void chimera__console_out(void *color, const char *format, ...); // 0x496b50, EAX color
extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474
extern void (*hs_type_inspectors[])(int16_t type, int32_t value, char *buffer); // 0x0068bb48
extern uint8_t hs_preserve_token_case; // 0x007102fd
extern uint8_t debug_log_level;        // 0x0087ac06

static hs_syntax_node *syntax_get(datum_index node)
{
    return (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node & 0xffff) * 0x14);
}

void hs_evaluate_inspect(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread *thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    hs_stack_frame *frame = thread->stack;
    int32_t *result = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    datum_index argument = syntax_get(syntax_get(frame->syntax_node)->data.first_child)->next_node;
    char buffer[0x400];

    frame->size = frame->size + 4;
    if (first != 0) {
        hs_thread_push(argument, thread_index, result);
        return;
    }
    {
        int16_t type = syntax_get(argument)->type;

        if (hs_type_inspectors[type] != 0) {
            hs_type_inspectors[type](type, *result, buffer);
            if (hs_preserve_token_case != 0 || debug_log_level >= 4) {
                chimera__console_out(0, buffer);
            }
        }
    }
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
