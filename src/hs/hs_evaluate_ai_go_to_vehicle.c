// hs_evaluate_ai_go_to_vehicle  (not a Ghidra function; the evaluate handler of hs "ai_go_to_vehicle" (ai, unit, string -> void))
// address 0x47d9d0, size 76 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47d9d0, only reachable through that pointer.
//   Campaign track: 143 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47d9d0: EAX = ai reference (+0x0), stack (vehicle +0x4, seat name +0x8, 0); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void ai_object_process_nearby_actors(uint32_t ai_reference, datum_index vehicle_index, char *seat_name,
    char allow_boarding_actors); // 0x433cc0, EAX ai reference, stack (vehicle, seat name, allow)

void hs_evaluate_ai_go_to_vehicle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_object_process_nearby_actors((uint32_t)arguments[0], (datum_index)arguments[1], (char *)arguments[2], 0);
        hs_thread_return(0, thread_index);
    }
}
