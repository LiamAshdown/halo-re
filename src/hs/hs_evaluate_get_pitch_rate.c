// hs_evaluate_get_pitch_rate  (not a Ghidra function; the evaluate handler of hs function 454 "get_pitch_rate" (short -> real))
// address 0x481a80, size 75 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_481a80 trapped.
// WRITTEN 2026-09-28 from objdump 0x481a80..0x481aca: returns the pitch rate of the controller (0x0070facc +0x4).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t player_control_look_rates_0070facc[]; // 0x0070facc, UNSURE: indexed by slot * 0x85c, one record before player_control_settings_cache

void hs_evaluate_get_pitch_rate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_thread_return(*(int32_t *)&*(float *)(player_control_look_rates_0070facc + (int16_t)arguments[0] * 0x85c + 0x4), thread_index);
    }
}
