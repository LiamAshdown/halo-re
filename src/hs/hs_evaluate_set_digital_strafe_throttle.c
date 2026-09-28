// hs_evaluate_set_digital_strafe_throttle  (not a Ghidra function; the evaluate handler of hs function 460 "set_digital_strafe_throttle" (short, real -> void))
// address 0x481d40, size 97 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_481d40 trapped.
// WRITTEN 2026-09-28 from objdump 0x481d40..0x481da0: for a controller 0..3 sets the digital strafe throttle
//   (+0x814) through input_clamp_unit_float; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t input_globals[]; // 0x00710328, player_control_settings, stride 0x85c
extern float input_clamp_unit_float(float value); // 0x48c8a0

void hs_evaluate_set_digital_strafe_throttle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            *(float *)(input_globals + slot * 0x85c + 0x814) = input_clamp_unit_float(*(float *)&arguments[1]);
        }
        hs_thread_return(0, thread_index);
    }
}
