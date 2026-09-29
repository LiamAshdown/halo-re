// hs_evaluate_ai_stop_looking  (not a Ghidra function; the evaluate handler of hs function 228 "ai_stop_looking" (unit -> void))
// address 0x47e340, size 63 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47e340 trapped.
// WRITTEN 2026-09-28 from objdump 0x47e340..0x47e37e: evaluates the arguments and calls
//   ai_unit_clear_actor_vocalization; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_stop_looking(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_unit_clear_actor_vocalization((datum_index)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}
