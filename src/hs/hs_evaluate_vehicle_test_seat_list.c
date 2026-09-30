// hs_evaluate_vehicle_test_seat_list  (not a Ghidra function; the evaluate handler of hs "vehicle_test_seat_list" (unit, string, object_list -> boolean))
// address 0x47be90, size 94 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47be90, only reachable through that pointer.
//   Campaign track: 87 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47be90: cdecl (unit +0x0, seat label +0x4, object list +0x8); the byte result is returned zero-extended.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_units.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_vehicle_test_seat_list(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t result = unit_named_seat_occupant_in_zone((uint32_t)arguments[0], (char *)arguments[1], (uint32_t)arguments[2]);
        hs_thread_return((int32_t)result, thread_index);
    }
}
