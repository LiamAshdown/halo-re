// hs_evaluate_ai_spawn_actor  (not a Ghidra function; the evaluate handler of hs "ai_spawn_actor" (ai -> void))
// address 0x47d360, size 67 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47d360, only reachable through that pointer.
//   Campaign track: 82 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47d360: stack (ai reference +0x0); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_spawn_actor(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_reference_resolve_squad_datum((uint32_t)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}
