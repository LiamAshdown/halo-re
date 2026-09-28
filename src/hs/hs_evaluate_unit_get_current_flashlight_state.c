// hs_evaluate_unit_get_current_flashlight_state  (not a Ghidra function; the evaluate handler of hs function 135 "unit_get_current_flashlight_state" (unit -> boolean))
// address 0x47c830, size 117 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47c830 trapped.
// WRITTEN 2026-09-28 from objdump 0x47c830..0x47c8a4: returns unit flag bit 19 (+0x204, flashlight on); false for
//   none.
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

void hs_evaluate_unit_get_current_flashlight_state(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t on = 0;

        if (arguments[0] != -1) {
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[arguments[0] & 0xffff].data;

            on = (uint8_t)((((unit_object *)unit)->unit.flags >> 0x13) & 1);
        }
        hs_thread_return((int32_t)(uint8_t)(on), thread_index);
    }
}
