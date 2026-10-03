#include "halo/hs/hs2_commands.hpp"
#include "halo/input/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/saved_games/vars.hpp"


static auto &input_device_count = halo::link::ref<int32_t>(halo::ui::vars().input_device_count);
static auto &input_device_to_slot = halo::link::ref<uint8_t []>(halo::saved_games::vars().input_device_to_slot);

namespace halo::hs {

/**
 * Evaluate handler of hs function "input_activate_joy"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481890
 */
void InputDeviceCommands::evaluate_input_activate_joy(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    int32_t joystick = *(int16_t *)&arguments[0];
    int32_t player = *(int16_t *)&arguments[1];
    uint8_t bound = 0;

    if (joystick < input_device_count && *(int32_t *)(input_device_to_slot + joystick * 0x240) == -1 &&
        halo::input::globals().joystick_slot_devices[player] == -1) {
        *(int32_t *)(input_device_to_slot + joystick * 0x240) = player;
        halo::input::globals().joystick_slot_devices[player] = joystick;
        bound = 1;
    }
    halo::hs::hs_thread_return((int32_t)bound, thread_index);
    }
}

/**
 * Evaluate handler of hs function "input_find_default"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4819f0
 */
void InputDeviceCommands::evaluate_input_find_default(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::input::test_input_device_defaults_find((char *)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "input_find_joystick"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481990
 */
void InputDeviceCommands::evaluate_input_find_joystick(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_thread_return((int32_t)(uint16_t)(-1), thread_index);
    }
}

/**
 * Evaluate handler of hs function "input_get_joy_count"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4817f0
 */
void InputDeviceCommands::evaluate_input_get_joy_count(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)(uint16_t)input_device_count, thread_index);
}

/**
 * Evaluate handler of hs function "input_is_joy_active"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481820
 */
void InputDeviceCommands::evaluate_input_is_joy_active(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int32_t device = (int16_t)arguments[0];
        uint8_t active = 0;

        if (device < input_device_count) {
            active = (uint8_t)(*(int32_t *)(input_device_to_slot + device * 0x240) != -1);
        }
        halo::hs::hs_thread_return((int32_t)(uint8_t)(active), thread_index);
    }
}

/**
 * Evaluate handler of hs function "input_show_joystick_info"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4819e0
 */
void InputDeviceCommands::evaluate_input_show_joystick_info(int16_t function_index, uint32_t thread_index, char first)
{
    halo::input::DirectInput::device_list_print();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Table of the hs functions handled by InputDeviceCommands, in source order.
 */
EvaluateCommandTable InputDeviceCommands::commands() noexcept
{
    static constexpr EvaluateFn k_commands[] = {
        &InputDeviceCommands::evaluate_input_activate_joy,
        &InputDeviceCommands::evaluate_input_deactivate_joy,
        &InputDeviceCommands::evaluate_input_find_default,
        &InputDeviceCommands::evaluate_input_find_joystick,
        &InputDeviceCommands::evaluate_input_get_joy_count,
        &InputDeviceCommands::evaluate_input_is_joy_active,
        &InputDeviceCommands::evaluate_input_show_joystick_info,
    };
    return {k_commands, static_cast<uint32_t>(sizeof(k_commands) / sizeof(k_commands[0]))};
}

}
