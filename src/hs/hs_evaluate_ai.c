// hs_evaluate_ai  (not a Ghidra function; the evaluate handler of hs function 284 "ai" (boolean -> void))
// address 0x482850, size 66 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482850 trapped.
// WRITTEN 2026-09-28 from objdump 0x482850..0x482891: stores the boolean in the ai enabled byte (ai_globals +0);
//   returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t *ai_global_data; // 0x00880354 (ai_globals *; +0 enabled, +1 encounters live)

void hs_evaluate_ai(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_global_data[0] = (uint8_t)arguments[0];
        hs_thread_return(0, thread_index);
    }
}
