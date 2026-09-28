// hs_evaluate_ai_timer_start  (not a Ghidra function; the evaluate handler of hs function 176 "ai_timer_start" (ai -> void))
// address 0x47d660, size 63 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47d660 trapped.
// WRITTEN 2026-09-28 from objdump 0x47d660..0x47d69e: evaluates the arguments and calls
//   ai_reference_mark_squads_unknown_11; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void ai_reference_mark_squads_unknown_11(uint32_t packed_reference); // 0x432f10, blam-cc: EAX

void hs_evaluate_ai_timer_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_reference_mark_squads_unknown_11((uint32_t)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}
