// hs_evaluate_units_set_desired_flashlight_state  (not a Ghidra function; the evaluate handler of hs "units_set_desired_flashlight_state" (object_list, boolean -> void))
// address 0x47c750, size 72 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47c750, only reachable through that pointer.
//   Campaign track: 4 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47c750: EAX = the object list, stack the boolean (zero-extended byte); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern void unit_mark_zone_list_alt_flag(uint32_t zone_list_index, uint8_t use_second_bit); // 0x56c1d0, EAX, stack

void hs_evaluate_units_set_desired_flashlight_state(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        unit_mark_zone_list_alt_flag((uint32_t)arguments[0], *(uint8_t *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
