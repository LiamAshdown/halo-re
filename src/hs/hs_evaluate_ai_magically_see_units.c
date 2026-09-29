// hs_evaluate_ai_magically_see_units  (not a Ghidra function; the evaluate handler of the orphan hs function record 0x658c18 "ai_magically_see_units" (ai, object_list -> void), not in hs_function_definitions)
// address 0x47d610, size 70 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47d610 trapped.
// WRITTEN 2026-09-28 from objdump 0x47d610..0x47d655: ai_object_list_respawn_members (0x432e80) with the list
//   (second argument) and the ai (first); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void ai_object_list_respawn_members(datum_index object_list_header_handle, uint32_t packed_reference); // 0x432e80, blam-cc: EAX, EBX

void hs_evaluate_ai_magically_see_units(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_object_list_respawn_members((datum_index)arguments[1], (uint32_t)arguments[0]);
        hs_thread_return(0, thread_index);
    }
}
