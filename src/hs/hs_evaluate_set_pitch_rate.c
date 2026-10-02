// hs_evaluate_set_pitch_rate  (not a Ghidra function; the evaluate handler of hs function 456 "set_pitch_rate" (short, real -> void))
// address 0x481b30, size 88 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_481b30 trapped.
// WRITTEN 2026-09-28 from objdump 0x481b30..0x481b87: for a controller 0..3 sets the pitch rate (+0x4); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t player_control_look_rates_0070facc[]; // 0x0070facc, UNSURE: indexed by slot * 0x85c, one record before player_control_settings_cache

void hs_evaluate_set_pitch_rate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            *(float *)(player_control_look_rates_0070facc + slot * 0x85c + 0x4) = *(float *)&arguments[1];
        }
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
