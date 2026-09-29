// hs_evaluate_object_set_shield  (not a Ghidra function; the evaluate handler of hs function 36 "object_set_shield" (object, real -> void))
// address 0x47a860, size 70 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47a860 trapped.
// WRITTEN 2026-09-28 from objdump 0x47a860..0x47a8a5: hs_object_set_health_fraction (0x488600) with the object and
//   the real.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_object_set_shield(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_object_set_health_fraction((datum_index)arguments[0], *(float *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
