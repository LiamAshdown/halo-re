/**
 * Binding display names and the name to index parsers used by the bind console commands and the controls menu.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include "halo/input/binding_names.hpp"

extern "C" { extern char joystick_button_prefix[0x18]; }
extern "C" { extern char decimal_suffixes[0x20][3]; }
extern "C" { extern char *strstr(const char *haystack, const char *needle); }
extern "C" { extern int32_t _stricmp(const char *a, const char *b); }
namespace halo::input {

/**
 * Resolves a "buttonN" style joystick input name string (e.g. from a 'bind' command) back to
 * its numeric button index (0..31), or -1 if name doesn't contain the "button" prefix or its
 * suffix isn't one of the decimal strings "0".."31".
 *
 * Original register convention: name in EAX.
 *
 * @address 0x4912e0
 */
int16_t BindingNames::joystick_button_name_to_index(char *name)
{
    char *suffix;
    char *table_entry;
    int16_t index;

    suffix = strstr(name, joystick_button_prefix);
    if (suffix == (char *)0) {
        return -1;
    }
    suffix = suffix + 6;
    index = 0;
    table_entry = decimal_suffixes[0];
    while (index < 0x20) {
        if (_stricmp(suffix, table_entry) == 0) {
            return index;
        }
        table_entry = table_entry + 3;
        index = index + 1;
    }
    return -1;
}

}

extern "C" { extern char joystick_pov_prefix[0x18]; }
extern "C" { extern int16_t input_joystick_pov_direction_name_to_index(char *name); }
namespace halo::input {

/**
 * Resolves a "povN direction" style joystick input name string (e.g. "pov2 north") back to its
 * numeric POV-hat index (0..15, the return value) and direction (0..7, written to
 * *out_direction), or -1 if the name doesn't contain the "pov" prefix, no suffix matches, or the
 * matched suffix's remainder isn't a valid direction name.
 *
 * Original register convention: name in EAX, out_direction on the stack.
 *
 * @address 0x491590
 */
int16_t BindingNames::joystick_pov_name_to_index(char *name, int16_t *out_direction)
{
    char *after_prefix;
    char *suffix;
    char *match;
    char *rest;
    int32_t pov_index;
    int32_t length;
    int16_t direction_index;

    after_prefix = strstr(name, joystick_pov_prefix);
    if (after_prefix == (char *)0) {
        return -1;
    }

    match = after_prefix + 1;
    pov_index = 0;
    suffix = decimal_suffixes[0];
    while (pov_index < 0x10) {
        rest = strstr(match, suffix);
        if (rest != (char *)0) {
            length = 0;
            while (suffix[length] != '\0') {
                length = length + 1;
            }

            rest = match + length;
            direction_index = input_joystick_pov_direction_name_to_index(rest);
            *out_direction = direction_index;
            if (direction_index != -1) {
                return (int16_t)pov_index;
            }
            return -1;
        }
        suffix = suffix + 3;
        pov_index = pov_index + 1;
    }
    return -1;
}

}
