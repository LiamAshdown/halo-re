// hs_evaluate_set_digital_yaw_increment  (not a Ghidra function; the evaluate handler of hs function 462 "set_digital_yaw_increment" (short, real -> void))
// address 0x481e10, size 97 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_481e10 trapped.
// WRITTEN 2026-09-28 from objdump 0x481e10..0x481e70: for a controller 0..3 sets the digital yaw increment (+0x818)
//   through input_sensitivity_to_turn_rate; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint8_t input_globals[]; // 0x00710328, player_control_settings, stride 0x85c
extern float input_sensitivity_to_turn_rate(float sensitivity); // 0x48c8e0

void hs_evaluate_set_digital_yaw_increment(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            *(float *)(input_globals + slot * 0x85c + 0x818) = input_sensitivity_to_turn_rate(*(float *)&arguments[1]);
        }
        hs_thread_return(0, thread_index);
    }
}
