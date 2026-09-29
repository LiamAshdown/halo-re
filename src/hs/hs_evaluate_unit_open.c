// hs_evaluate_unit_open  (not a Ghidra function; the evaluate handler of hs function 94 "unit_open" (unit -> void))
// address 0x47b8a0, size 74 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b8a0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b8a0..0x47b8ea: unit_try_set_animation_state(stack: unit, 0x25) for a unit other than -1, returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90

void hs_evaluate_unit_open(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if ((datum_index)arguments[0] != k_datum_index_none) {
        unit_try_set_animation_state((uint32_t)arguments[0], 0x25);
    }
    hs_thread_return(0, thread_index);
    }
}
