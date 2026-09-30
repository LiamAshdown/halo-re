// hs_evaluate_unit_enter_vehicle  (not a Ghidra function; the evaluate handler of hs function 107 "unit_enter_vehicle" (unit, vehicle, string -> void))
// address 0x47be40, size 75 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47be40, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47be40..0x47be8b: unit_detach_and_enter_named_seat(stack: unit, vehicle, seat name), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_units.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


    // 0x569d40

void hs_evaluate_unit_enter_vehicle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    unit_detach_and_enter_named_seat((uint32_t)arguments[0], (uint32_t)arguments[1], (char *)arguments[2]);
    hs_thread_return(0, thread_index);
    }
}
