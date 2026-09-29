// hs_evaluate_unit_kill_silent  (not a Ghidra function; the evaluate handler of hs function 97 "unit_kill_silent" (unit -> void))
// address 0x47b9a0, size 86 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47b9a0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47b9a0..0x47b9f5: sets bit 6 of the unit's byte +0x106 (die silently; no check
//   for none); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern data_array *object_data; // 0x008603b0

void hs_evaluate_unit_kill_silent(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[arguments[0] & 0xffff].data;

        unit[0x106] |= 0x40;
        hs_thread_return(0, thread_index);
    }
}
