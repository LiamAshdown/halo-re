// hs_evaluate_sound_get_gain  (not a Ghidra function; the evaluate handler of hs function 55 "sound_get_gain" (string -> real))
// address 0x47ac70, size 95 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47ac70 trapped.
// WRITTEN 2026-09-28 from objdump 0x47ac70..0x47acce: returns the named gain (0x488b10) as a real, 0.0 when
//   unknown.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_sound_get_gain(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        float *gain = hs_sound_get_gain_reference((char *)arguments[0]);

        hs_thread_return(gain != 0 ? *(int32_t *)gain : 0, thread_index);
    }
}
