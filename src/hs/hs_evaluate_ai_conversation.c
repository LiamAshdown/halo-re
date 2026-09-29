// hs_evaluate_ai_conversation  (not a Ghidra function; the evaluate handler of hs function 235 "ai_conversation" (conversation -> boolean))
// address 0x47ec30, size 88 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47ec30, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47ec30..0x47ec88: ai_conversation_activate(AX conversation, stack 1) into a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_conversation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)ai_conversation_activate(*(int16_t *)&arguments[0], 1), thread_index);
    }
}
