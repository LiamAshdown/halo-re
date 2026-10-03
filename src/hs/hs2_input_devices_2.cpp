#include "halo/hs/hs2_commands.hpp"
#include "halo/input/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/saved_games/vars.hpp"
#include "halo/saved_games/api.hpp"


static auto &input_device_count = halo::link::ref<int32_t>(halo::ui::vars().input_device_count);
static auto &input_device_to_slot = halo::link::ref<uint8_t []>(halo::saved_games::vars().input_device_to_slot);

namespace halo::hs {

/**
 * Evaluate handler of hs function "input_deactivate_joy"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481920
 */
void InputDeviceCommands::evaluate_input_deactivate_joy(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int32_t device = (int16_t)arguments[0];

        if (device < input_device_count) {
            int32_t *slot = (int32_t *)(input_device_to_slot + device * 0x240);

            if (*slot != -1) {
                int32_t old_slot = *slot;

                *slot = -1;
                halo::input::globals().joystick_slot_devices[old_slot] = -1;
            }
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

}
