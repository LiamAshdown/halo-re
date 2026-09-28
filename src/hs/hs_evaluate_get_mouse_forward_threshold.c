// hs_evaluate_get_mouse_forward_threshold  (not a Ghidra function; the evaluate handler of hs function 465 "get_mouse_forward_threshold" (short -> real))
// address 0x481f50, size 75 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_481f50 trapped.
// WRITTEN 2026-09-28 from objdump 0x481f50..0x481f9a: returns the mouse forward threshold of the controller
//   (player_control_settings +0x820).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t player_control_settings_cache[]; // 0x00710328, player_control_settings, stride 0x85c

void hs_evaluate_get_mouse_forward_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        hs_thread_return(*(int32_t *)&*(float *)(player_control_settings_cache + (int16_t)arguments[0] * 0x85c + 0x820), thread_index);
    }
}
