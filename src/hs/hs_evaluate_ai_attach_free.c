// hs_evaluate_ai_attach_free  (not a Ghidra function; the evaluate handler of hs function 160 "ai_attach_free" (unit, actor_variant -> void))
// address 0x47d0f0, size 70 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47d0f0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47d0f0..0x47d136: ai_unit_create_actor(EAX actor variant, stack unit), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_ai.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_ai_attach_free(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    ai_unit_create_actor((datum_index)arguments[1], (datum_index)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}
