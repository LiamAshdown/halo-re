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
#include "halo/input/api.hpp"
#include "halo/main/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/devices.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"

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

    if (halo::input::Bindings::parse_device_binding_string((char *)device_class_name, (char *)input_name, &binding) != 0) {
        action_index = halo::input::BindingNames::action_name_to_index((char *)action_name);
        if (action_index != (int16_t)k_input_unbound) {
            if (halo::input::input_apply_control_binding(&binding, action_index) != 0) {
                halo::main::console_out_printf(0, "bound %s %s to game control %s", device_class_name, input_name, action_name);
            }
        }
    }
}

}

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

    if (halo::input::Bindings::parse_device_binding_string((char *)device_class_name, (char *)input_name, &binding) == 0) {
        return;
    }

    halo::input::Bindings::clear_control_binding(&binding);

    switch (binding.device_type) {
    case _control_device_keyboard:
        halo::main::console_out_printf(0, "unbound %s key", input_name);
        break;

    case _control_device_mouse:

        halo::main::console_out_printf(0, "unbound %s key", input_name);
        if (binding.input_kind == _control_input_axis) {
            halo::main::console_out_printf(0, "unbound mouse axis %s", input_name);
        } else {
            halo::main::console_out_printf(0, "unbound %s mouse button", input_name);
        }
        break;

    case _control_device_gamepad:
        if (binding.input_kind == _control_input_axis) {
            halo::main::console_out_printf(0, "unbound axis %s on gamepad %d", input_name, binding.device_index);
        } else {
            halo::main::console_out_printf(0, "unbound %s on gamepad %d", input_name, binding.device_index);
        }
        break;

    default:
        break;
    }
}

}
