// hs_evaluate_unit_set_desired_flashlight_state  (not a Ghidra function; the evaluate handler of hs function 134 "unit_set_desired_flashlight_state" (unit, boolean -> void))
// address 0x47c7a0, size 130 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47c7a0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47c7a0..0x47c821: requests the flashlight on (unit flag 0x10000000, +0x204) or
//   off (flag 0x20000000); a none unit is ignored. Returns 0.
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
extern data_array *object_data; // 0x008603b0

void hs_evaluate_unit_set_desired_flashlight_state(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1) {
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[arguments[0] & 0xffff].data;

            ((unit_object *)unit)->unit.flags |= (uint8_t)arguments[1] ? 0x10000000 : 0x20000000;
        }
        hs_thread_return(0, thread_index);
    }
}
