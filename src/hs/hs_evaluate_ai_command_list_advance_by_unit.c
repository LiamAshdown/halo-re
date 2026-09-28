// hs_evaluate_ai_command_list_advance_by_unit  (not a Ghidra function; the evaluate handler of hs function 211 "ai_command_list_advance_by_unit" (unit -> void))
// address 0x47de80, size 123 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47de80 trapped.
// WRITTEN 2026-09-28 from objdump 0x47de80..0x47defa: for a live unit (object_try_and_get mask 3): steps the swarm
//   components of its actor (+0x1f4) or, without one, of +0x1f8 (0x407240). Returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "units.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX, stack
extern void actor_swarm_for_each_component_thunk(uint32_t actor_index); // 0x407240, blam-cc: EAX

void hs_evaluate_ai_command_list_advance_by_unit(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1) {
            uint8_t *unit = (uint8_t *)object_try_and_get((datum_index)arguments[0], 3);

            if (unit != 0) {
                if (*(int32_t *)&((unit_object *)unit)->unit.actor_index != -1) {
                    actor_swarm_for_each_component_thunk(*(uint32_t *)&((unit_object *)unit)->unit.actor_index);
                } else if (*(int32_t *)&((unit_object *)unit)->unit.swarm_actor_index != -1) {
                    actor_swarm_for_each_component_thunk(*(uint32_t *)&((unit_object *)unit)->unit.swarm_actor_index);
                }
            }
        }
        hs_thread_return(0, thread_index);
    }
}
