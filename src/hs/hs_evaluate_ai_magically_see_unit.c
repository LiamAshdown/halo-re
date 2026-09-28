// hs_evaluate_ai_magically_see_unit  (not a Ghidra function; the evaluate handler of hs function 175 "ai_magically_see_unit" (ai, unit -> void))
// address 0x47d5c0, size 68 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47d5c0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47d5c0..0x47d603: ai_reference_respawn_member (0x432df0) with the ai and the
//   unit; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void ai_reference_respawn_member(uint32_t packed_reference, datum_index unit_index); // 0x432df0, blam-cc: EAX, EDI

void hs_evaluate_ai_magically_see_unit(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_reference_respawn_member((uint32_t)arguments[0], (datum_index)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
