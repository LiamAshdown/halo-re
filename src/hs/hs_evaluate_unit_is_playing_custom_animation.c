// hs_evaluate_unit_is_playing_custom_animation  (not a Ghidra function; the evaluate handler of hs function 103 "unit_is_playing_custom_animation" (unit -> boolean))
// address 0x47bc20, size 133 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47bc20 trapped.
// WRITTEN 2026-09-28 from objdump 0x47bc20..0x47bca4: true when the unit's animation state byte (+0x2a3) is 0x1c
//   (custom animation); false for none.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern data_array *object_data; // 0x008603b0

void hs_evaluate_unit_is_playing_custom_animation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t playing = 0;

        if (arguments[0] != -1) {
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[arguments[0] & 0xffff].data;

            playing = (uint8_t)(unit[0x2a3] == 0x1c);
        }
        hs_thread_return((int32_t)(uint8_t)(playing), thread_index);
    }
}
