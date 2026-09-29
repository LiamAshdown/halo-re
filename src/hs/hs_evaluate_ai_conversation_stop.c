// hs_evaluate_ai_conversation_stop  (not a Ghidra function; the evaluate handler of hs function 236 "ai_conversation_stop" (conversation -> void))
// address 0x47e5f0, size 66 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47e5f0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47e5f0..0x47e632: ai_conversation_stop_all(SI conversation), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void ai_conversation_stop_all(int16_t conversation_definition_index); // 0x4309c0, blam-cc: SI

void hs_evaluate_ai_conversation_stop(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    ai_conversation_stop_all(*(int16_t *)&arguments[0]);
    hs_thread_return(0, thread_index);
    }
}
