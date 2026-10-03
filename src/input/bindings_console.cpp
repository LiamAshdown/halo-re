/**
 * Control binding table, last-used-binding cache, bind/unbind commands, rebind capture and device default profiles.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include "halo/input/bindings.hpp"

extern "C" { extern uint8_t input_parse_device_binding_string(const char *device_class_name, const char *input_name, control_binding_descriptor *out_binding); }
extern "C" { extern int16_t input_action_name_to_index(const char *action_name); }
extern "C" { extern uint8_t input_apply_control_binding(control_binding_descriptor *binding, int32_t action_index); }
extern "C" { extern void console_out_printf(uint8_t unknown, const char *format, ...); }
namespace halo::input {

/**
 * Console/script 'bind' command: parses <device_class_name, input_name> into a binding
 * descriptor, resolves <action_name> to a game control index, writes the binding, and echoes a
 * confirmation line on success. Any failed step is silently ignored.
 *
 * @address 0x48b750
 */
void Bindings::hs_bind_control(const char *device_class_name, const char *input_name, const char *action_name)
{
    control_binding_descriptor binding;
    int16_t action_index;

    if (input_parse_device_binding_string(device_class_name, input_name, &binding) != 0) {
        action_index = input_action_name_to_index(action_name);
        if (action_index != (int16_t)k_input_unbound) {
            if (input_apply_control_binding(&binding, action_index) != 0) {
                console_out_printf(0, "bound %s %s to game control %s", device_class_name, input_name, action_name);
            }
        }
    }
}

}

extern "C" { extern void input_clear_control_binding(control_binding_descriptor *binding); }
namespace halo::input {

/**
 * Console/script 'unbind' command: parses <device_class_name, input_name> into a binding
 * descriptor, clears whatever game control it was assigned to, and prints a device-appropriate
 * 'unbound ...' confirmation. Does nothing if the descriptor fails to parse.
 *
 * @address 0x48b8d0
 */
void Bindings::hs_unbind_control(const char *device_class_name, const char *input_name)
{
    control_binding_descriptor binding;

    if (input_parse_device_binding_string(device_class_name, input_name, &binding) == 0) {
        return;
    }

    input_clear_control_binding(&binding);

    switch (binding.device_type) {
    case _control_device_keyboard:
        console_out_printf(0, "unbound %s key", input_name);
        break;

    case _control_device_mouse:

        console_out_printf(0, "unbound %s key", input_name);
        if (binding.input_kind == _control_input_axis) {
            console_out_printf(0, "unbound mouse axis %s", input_name);
        } else {
            console_out_printf(0, "unbound %s mouse button", input_name);
        }
        break;

    case _control_device_gamepad:
        if (binding.input_kind == _control_input_axis) {
            console_out_printf(0, "unbound axis %s on gamepad %d", input_name, binding.device_index);
        } else {
            console_out_printf(0, "unbound %s on gamepad %d", input_name, binding.device_index);
        }
        break;

    default:
        break;
    }
}

}
