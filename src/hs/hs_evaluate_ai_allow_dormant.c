// hs_evaluate_ai_allow_dormant  (not a Ghidra function; the evaluate handler of hs "ai_allow_dormant" (ai, boolean -> void))
// address 0x47e7e0, size 72 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47e7e0, only reachable through that pointer.
//   Campaign track: 9 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47e7e0: EAX = the ai reference, stack the boolean (zero-extended byte); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_allow_dormant(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_reference_set_squads_unknown_14((uint32_t)arguments[0], *(char *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
