// hs_evaluate_effect_new_on_object_marker  (not a Ghidra function; the evaluate handler of hs function 48 "effect_new_on_object_marker" (effect, object, string -> void))
// address 0x47aa10, size 77 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47aa10, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47aa10..0x47aa5d: hs_effect_spawn_on_marker(ESI object, EDI effect, stack marker name), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


    // 0x4888f0, blam-cc: ESI, EDI, stack

void hs_evaluate_effect_new_on_object_marker(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_effect_spawn_on_marker((datum_index)arguments[1], (datum_index)arguments[0], (char *)arguments[2]);
    hs_thread_return(0, thread_index);
    }
}
