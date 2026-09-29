// hs_evaluate_ai_command_list_status  (not a Ghidra function; the evaluate handler of hs function 212 "ai_command_list_status" (object_list -> short))
// address 0x47e890, size 83 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47e890, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47e890..0x47e8e3: ai_object_list_max_flee_grade(EAX list) as a word in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_command_list_status(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)(uint16_t)ai_object_list_max_flee_grade((datum_index)arguments[0]), thread_index);
    }
}
