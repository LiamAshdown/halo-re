#include "halo/hs/hs2_commands.hpp"


#ifdef __cplusplus
extern "C" {
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first);
extern void hs_thread_return(int32_t value, uint32_t thread_index);
extern int32_t input_device_count;
extern uint8_t input_device_to_slot[];
extern int32_t joystick_slot_devices[4];
#ifdef __cplusplus
}
#endif

namespace halo::hs {

/**
 * Evaluate handler of hs function "input_deactivate_joy"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481920
 */
void InputDeviceCommands::evaluate_input_deactivate_joy(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int32_t device = (int16_t)arguments[0];

        if (device < input_device_count) {
            int32_t *slot = (int32_t *)(input_device_to_slot + device * 0x240);

            if (*slot != -1) {
                int32_t old_slot = *slot;

                *slot = -1;
                joystick_slot_devices[old_slot] = -1;
            }
        }
        hs_thread_return(0, thread_index);
    }
}

}
