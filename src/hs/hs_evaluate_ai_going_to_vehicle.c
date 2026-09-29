// hs_evaluate_ai_going_to_vehicle  (not a Ghidra function; the evaluate handler of hs "ai_going_to_vehicle" (unit -> short))
// address 0x47e8f0, size 87 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47e8f0, only reachable through that pointer.
//   Campaign track: 5 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47e8f0: stack (the unit +0x0); the short count is returned zero-extended.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern int16_t ai_count_actors_in_mode9_group(int32_t group_id); // 0x433e20, stack

void hs_evaluate_ai_going_to_vehicle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t count = ai_count_actors_in_mode9_group((int32_t)arguments[0]);
        hs_thread_return((int32_t)(uint16_t)count, thread_index);
    }
}
