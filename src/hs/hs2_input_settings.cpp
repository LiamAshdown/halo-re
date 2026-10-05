#include "halo/hs/records.hpp"
#include "halo/core/bit_cast.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "halo/hs/hs2_commands.hpp"
#include "halo/input/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/devices.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/main/api.hpp"


static auto &player_control_look_rates_0070facc = halo::link::ref<uint8_t []>(halo::hs::vars().player_control_look_rates_0070facc);
static auto &input_globals = halo::link::ref<uint8_t []>(halo::main::vars().input_globals);
static auto &profile_globals_block = halo::link::ref<saved_player_profile>(halo::ui::vars().profile_globals_block);

static constexpr float k_turn_rate_display_scale = halo::bit_cast<float>(0x431f27aaU);

static player_control_settings &control_settings(int16_t slot)
{
    return reinterpret_cast<player_control_settings *>(input_globals)[slot];
}

static player_control_settings &look_rate_settings(int16_t slot)
{
    return reinterpret_cast<player_control_settings *>(player_control_look_rates_0070facc)[slot];
}

namespace halo::hs {

/**
 * Evaluate handler of hs function "get_digital_forward_throttle"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481b90
 */
void InputSettingsCommands::evaluate_get_digital_forward_throttle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        float value = control_settings((int16_t)arguments[0]).forward_rate;

        if (value < 0.0f) {
            value = 0.0f;
        } else if (value > 1.0f) {
            value = 1.0f;
        }
        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(value), thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_digital_pitch_increment"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481e80
 */
void InputSettingsCommands::evaluate_get_digital_pitch_increment(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        float value = control_settings((int16_t)arguments[0]).look_y_rate * k_turn_rate_display_scale;

        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(value), thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_digital_strafe_throttle"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481ca0
 */
void InputSettingsCommands::evaluate_get_digital_strafe_throttle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        float value = control_settings((int16_t)arguments[0]).strafe_rate;

        if (value < 0.0f) {
            value = 0.0f;
        } else if (value > 1.0f) {
            value = 1.0f;
        }
        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(value), thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_digital_yaw_increment"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481db0
 */
void InputSettingsCommands::evaluate_get_digital_yaw_increment(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        float value = control_settings((int16_t)arguments[0]).look_x_rate * k_turn_rate_display_scale;

        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(value), thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_gamepad_forward_threshold"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482250
 */
void InputSettingsCommands::evaluate_get_gamepad_forward_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(control_settings((int16_t)arguments[0]).gamepad_axis_scale_x), thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_gamepad_strafe_threshold"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4822f0
 */
void InputSettingsCommands::evaluate_get_gamepad_strafe_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(control_settings((int16_t)arguments[0]).gamepad_axis_scale_y), thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_gamepad_yaw_scale"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482390
 */
void InputSettingsCommands::evaluate_get_gamepad_yaw_scale(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_mouse_forward_threshold"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481f50
 */
void InputSettingsCommands::evaluate_get_mouse_forward_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(control_settings((int16_t)arguments[0]).mouse_forward_scale), thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_mouse_pitch_scale"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482180
 */
void InputSettingsCommands::evaluate_get_mouse_pitch_scale(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        float value = control_settings((int16_t)arguments[0]).mouse_look_y_sensitivity * k_turn_rate_display_scale;

        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(value), thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_mouse_strafe_threshold"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482000
 */
void InputSettingsCommands::evaluate_get_mouse_strafe_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(control_settings((int16_t)arguments[0]).mouse_strafe_scale), thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_mouse_yaw_scale"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4820b0
 */
void InputSettingsCommands::evaluate_get_mouse_yaw_scale(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        float value = control_settings((int16_t)arguments[0]).mouse_look_x_sensitivity * k_turn_rate_display_scale;

        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(value), thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_pitch_rate"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481a80
 */
void InputSettingsCommands::evaluate_get_pitch_rate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(look_rate_settings((int16_t)arguments[0]).look_rate_40), thread_index);
    }
}

/**
 * Evaluate handler of hs function "get_yaw_rate"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481a30
 */
void InputSettingsCommands::evaluate_get_yaw_rate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_thread_return(halo::bit_cast<int32_t>(look_rate_settings((int16_t)arguments[0]).look_rate_80), thread_index);
    }
}

/**
 * Evaluate handler of hs function "player0_joystick_set_is_normal"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4814a0
 */
void InputSettingsCommands::evaluate_player0_joystick_set_is_normal(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t joystick_set = profile_globals_block.joystick_set;

    halo::hs::hs_thread_return((int32_t)(joystick_set == 0 || joystick_set == 1), thread_index);
}

/**
 * Evaluate handler of hs function "player0_look_invert_pitch"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481440
 */
void InputSettingsCommands::evaluate_player0_look_invert_pitch(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::interface::player_profile_save_495fb0(halo::hs::argument_byte(arguments[0]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "player0_look_pitch_is_inverted"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481480
 */
void InputSettingsCommands::evaluate_player0_look_pitch_is_inverted(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_thread_return((int32_t)profile_globals_block.look_inverted, thread_index);
}

/**
 * Evaluate handler of hs function "set_digital_forward_throttle"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481c30
 */
void InputSettingsCommands::evaluate_set_digital_forward_throttle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            control_settings(slot).forward_rate = halo::input::GameActions::clamp_unit_float(halo::hs::argument_real(arguments[1]));
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_digital_pitch_increment"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481ee0
 */
void InputSettingsCommands::evaluate_set_digital_pitch_increment(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            control_settings(slot).look_y_rate = halo::input::GameActions::sensitivity_to_turn_rate(halo::hs::argument_real(arguments[1]));
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_digital_strafe_throttle"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481d40
 */
void InputSettingsCommands::evaluate_set_digital_strafe_throttle(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            control_settings(slot).strafe_rate = halo::input::GameActions::clamp_unit_float(halo::hs::argument_real(arguments[1]));
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_digital_yaw_increment"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481e10
 */
void InputSettingsCommands::evaluate_set_digital_yaw_increment(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            control_settings(slot).look_x_rate = halo::input::GameActions::sensitivity_to_turn_rate(halo::hs::argument_real(arguments[1]));
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_gamepad_forward_threshold"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4822a0
 */
void InputSettingsCommands::evaluate_set_gamepad_forward_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::input::GameActions::joystick_set_axis_scale_x((int16_t)arguments[0], halo::hs::argument_real(arguments[1]));
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_gamepad_strafe_threshold"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482340
 */
void InputSettingsCommands::evaluate_set_gamepad_strafe_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::input::GameActions::joystick_set_axis_scale_y((int16_t)arguments[0], halo::hs::argument_real(arguments[1]));
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_mouse_forward_threshold"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481fa0
 */
void InputSettingsCommands::evaluate_set_mouse_forward_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            control_settings(slot).mouse_forward_scale = halo::hs::argument_real(arguments[1]);
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_mouse_pitch_scale"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4821e0
 */
void InputSettingsCommands::evaluate_set_mouse_pitch_scale(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            control_settings(slot).mouse_look_y_sensitivity = halo::input::GameActions::sensitivity_to_turn_rate(halo::hs::argument_real(arguments[1]));
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_mouse_strafe_threshold"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482050
 */
void InputSettingsCommands::evaluate_set_mouse_strafe_threshold(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            control_settings(slot).mouse_strafe_scale = halo::hs::argument_real(arguments[1]);
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_mouse_yaw_scale"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x482110
 */
void InputSettingsCommands::evaluate_set_mouse_yaw_scale(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            control_settings(slot).mouse_look_x_sensitivity = halo::input::GameActions::sensitivity_to_turn_rate(halo::hs::argument_real(arguments[1]));
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_pitch_rate"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481b30
 */
void InputSettingsCommands::evaluate_set_pitch_rate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            look_rate_settings(slot).look_rate_40 = halo::hs::argument_real(arguments[1]);
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set_yaw_rate"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x481ad0
 */
void InputSettingsCommands::evaluate_set_yaw_rate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        int16_t slot = (int16_t)arguments[0];

        if (slot >= 0 && slot < 4) {
            look_rate_settings(slot).look_rate_80 = halo::hs::argument_real(arguments[1]);
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Table of the hs functions handled by InputSettingsCommands, in source order.
 */
EvaluateCommandTable InputSettingsCommands::commands() noexcept
{
    static constexpr EvaluateFn k_commands[] = {
        &InputSettingsCommands::evaluate_get_digital_forward_throttle,
        &InputSettingsCommands::evaluate_get_digital_pitch_increment,
        &InputSettingsCommands::evaluate_get_digital_strafe_throttle,
        &InputSettingsCommands::evaluate_get_digital_yaw_increment,
        &InputSettingsCommands::evaluate_get_gamepad_forward_threshold,
        &InputSettingsCommands::evaluate_get_gamepad_strafe_threshold,
        &InputSettingsCommands::evaluate_get_gamepad_yaw_scale,
        &InputSettingsCommands::evaluate_get_mouse_forward_threshold,
        &InputSettingsCommands::evaluate_get_mouse_pitch_scale,
        &InputSettingsCommands::evaluate_get_mouse_strafe_threshold,
        &InputSettingsCommands::evaluate_get_mouse_yaw_scale,
        &InputSettingsCommands::evaluate_get_pitch_rate,
        &InputSettingsCommands::evaluate_get_yaw_rate,
        &InputSettingsCommands::evaluate_player0_joystick_set_is_normal,
        &InputSettingsCommands::evaluate_player0_look_invert_pitch,
        &InputSettingsCommands::evaluate_player0_look_pitch_is_inverted,
        &InputSettingsCommands::evaluate_set_digital_forward_throttle,
        &InputSettingsCommands::evaluate_set_digital_pitch_increment,
        &InputSettingsCommands::evaluate_set_digital_strafe_throttle,
        &InputSettingsCommands::evaluate_set_digital_yaw_increment,
        &InputSettingsCommands::evaluate_set_gamepad_forward_threshold,
        &InputSettingsCommands::evaluate_set_gamepad_strafe_threshold,
        &InputSettingsCommands::evaluate_set_mouse_forward_threshold,
        &InputSettingsCommands::evaluate_set_mouse_pitch_scale,
        &InputSettingsCommands::evaluate_set_mouse_strafe_threshold,
        &InputSettingsCommands::evaluate_set_mouse_yaw_scale,
        &InputSettingsCommands::evaluate_set_pitch_rate,
        &InputSettingsCommands::evaluate_set_yaw_rate,
    };
    return {k_commands, static_cast<uint32_t>(sizeof(k_commands) / sizeof(k_commands[0]))};
}

}
