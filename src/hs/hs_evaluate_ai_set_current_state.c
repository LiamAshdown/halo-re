// hs_evaluate_ai_set_current_state  (not a Ghidra function; the evaluate handler of hs function 217 "ai_set_current_state" (ai, ai_default_state -> void))
// address 0x47e020, size 69 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47e020 trapped.
// WRITTEN 2026-09-28 from objdump 0x47e020..0x47e064: squad_members_request_order (0x435630) with the ai and the
//   state; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_set_current_state(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        squad_members_request_order((uint32_t)arguments[0], (int16_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
