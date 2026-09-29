// hs_evaluate_ai_braindead  (not a Ghidra function; the evaluate handler of hs function 198 "ai_braindead" (ai, boolean -> void))
// address 0x47dab0, size 72 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47dab0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47dab0..0x47daf8: ai_reference_reset_or_wake_awareness(EAX ai, stack: the boolean zero-extended), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_braindead(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    ai_reference_reset_or_wake_awareness((uint32_t)arguments[0], *(char *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}
