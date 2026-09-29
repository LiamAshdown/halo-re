// hs_evaluate_get_digital_forward_throttle  (not a Ghidra function; the evaluate handler of hs function 457 "get_digital_forward_throttle" (short -> real))
// address 0x481b90, size 147 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_481b90 trapped.
// WRITTEN 2026-09-28 from objdump 0x481b90..0x481c22: returns the digital forward throttle of the controller
//   (player_control_settings +0x810) clamped to 0..1 (NaN passes).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint8_t input_globals[]; // 0x00710328, player_control_settings, stride 0x85c

void hs_evaluate_get_digital_forward_throttle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        float value = *(float *)(input_globals + (int16_t)arguments[0] * 0x85c + 0x810);

        if (value < 0.0f) {
            value = 0.0f;
        } else if (value > 1.0f) {
            value = 1.0f;
        }
        hs_thread_return(*(int32_t *)&value, thread_index);
    }
}
