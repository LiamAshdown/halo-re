// hs_evaluate_get_mouse_yaw_scale  (not a Ghidra function; the evaluate handler of hs function 469 "get_mouse_yaw_scale" (short -> real))
// address 0x4820b0, size 83 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4820b0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4820b0..0x482102: returns the mouse yaw scale of the controller
//   (player_control_settings +0x828) times 1000/2pi (0x00672c10).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint8_t input_globals[]; // 0x00710328, player_control_settings, stride 0x85c
static const uint32_t k_turn_rate_display_bits = 0x431f27aa; // 0x00672c10, about 159.155 (1000 / 2pi)

void hs_evaluate_get_mouse_yaw_scale(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        float value = *(float *)(input_globals + (int16_t)arguments[0] * 0x85c + 0x828) * *(const float *)&k_turn_rate_display_bits;

        hs_thread_return(*(int32_t *)&value, thread_index);
    }
}
