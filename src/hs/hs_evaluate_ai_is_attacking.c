// hs_evaluate_ai_is_attacking  (not a Ghidra function; the evaluate handler of hs function 213 "ai_is_attacking" (ai -> boolean))
// address 0x47e830, size 82 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47e830 trapped.
// WRITTEN 2026-09-28 from objdump 0x47e830..0x47e881: returns ai_platoon_range_has_available (0x433180) for the ai
//   as a boolean.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_is_attacking(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_thread_return((int32_t)(uint8_t)(ai_platoon_range_has_available((uint32_t)arguments[0])), thread_index);
    }
}
