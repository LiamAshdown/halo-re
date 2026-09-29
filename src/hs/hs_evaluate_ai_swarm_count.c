// hs_evaluate_ai_swarm_count  (not a Ghidra function; the evaluate handler of hs function 191 "ai_swarm_count" (ai -> short))
// address 0x47eaa0, size 98 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47eaa0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47eaa0..0x47eb01: returns ai_reference_get_stat_pair (0x432f90, stat kind 1, no
//   outputs) for the ai as a short.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_swarm_count(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_thread_return((int32_t)(uint16_t)(ai_reference_get_stat_pair((uint32_t)arguments[0], 1, 0, 0)), thread_index);
    }
}
