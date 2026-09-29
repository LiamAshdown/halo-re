// hs_evaluate_set_yaw_rate  (not a Ghidra function; the evaluate handler of hs function 455 "set_yaw_rate" (short, real -> void))
// address 0x481ad0, size 88 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_481ad0 trapped.
// WRITTEN 2026-09-28 from objdump 0x481ad0..0x481b27: for a controller 0..3 sets the yaw rate (+0x0); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint8_t player_control_look_rates_0070facc[]; // 0x0070facc, UNSURE: indexed by slot * 0x85c, one record before player_control_settings_cache

void hs_evaluate_set_yaw_rate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            *(float *)(player_control_look_rates_0070facc + slot * 0x85c + 0x0) = *(float *)&arguments[1];
        }
        hs_thread_return(0, thread_index);
    }
}
