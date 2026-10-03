/**
 * Binding display names and the name to index parsers used by the bind console commands and the controls menu.
 */

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <string.h>

#include "halo/input/binding_names.hpp"
#include "halo/input/api.hpp"
#include "halo/input/state.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"

namespace halo::input {

/**
 * Resolves a joystick axis-plus-direction name string (e.g. "axis3 +") back to its numeric axis
 * index (0..31, written to the return value) and direction (1 or 0, written to *out_direction),
 * or -1 if the name doesn't contain the "axis" prefix or no suffix/direction combination matches.
 *
 * Original register convention: name in EAX.
 *
 * @address 0x4913e0
 */
int16_t BindingNames::joystick_axis_name_to_index(char *name, uint8_t *out_direction)
{
    char *after_prefix;
    char *suffix;
    char *match;
    char *rest;
    int32_t axis_index;
    int16_t direction_index;

    after_prefix = strstr(name, input_state().joystick_axis_prefix);
    if (after_prefix == (char *)0) {
        return -1;
    }

    axis_index = 0;
    suffix = input_state().decimal_suffixes[0];
    while (axis_index < 0x20) {
        match = strstr(after_prefix, suffix);
        if (match != (char *)0) {
            rest = match + strlen(suffix);
            direction_index = halo::input::BindingNames::axis_direction_name_to_index(rest);
            if (direction_index == 0) {
                *out_direction = 1;
                return (int16_t)axis_index;
            }
            if (direction_index == 1) {
                *out_direction = 0;
                return (int16_t)axis_index;
            }
            return -1;
        }
        suffix = suffix + 3;
        axis_index = axis_index + 1;
    }
    return -1;
}

}
