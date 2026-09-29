// hs_evaluate_ai_look_at_object  (not a Ghidra function; the evaluate handler of hs function 227 "ai_look_at_object" (unit, object -> void))
// address 0x47e2f0, size 66 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47e2f0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47e2f0..0x47e331: ai_unit_dispatch_actor_event_d (0x435a00) with the unit and
//   the object; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void ai_unit_dispatch_actor_event_d(datum_index unit_index, int32_t unused); // 0x435a00, blam-cc: EAX, ECX

void hs_evaluate_ai_look_at_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_unit_dispatch_actor_event_d((datum_index)arguments[0], arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
