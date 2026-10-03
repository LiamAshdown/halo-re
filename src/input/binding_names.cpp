/**
 * Binding display names and the name to index parsers used by the bind console commands and the controls menu.
 */

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <wchar.h>
#include "crt.h"
#include <string.h>

#include "halo/input/binding_names.hpp"
#include "halo/input/api.hpp"

extern "C" { extern tag_instance *tag_instances; }
extern "C" { extern datum_index tag_lookup(tag_group group, char *path); }
extern "C" { extern uint16_t missing_string_text[]; }
extern "C" { extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); }
namespace halo::input {

/**
 * Builds "<gamepad axis name><axis_index + 1> <direction>" (e.g. "Axis1 +") into out_text (25
 * wide characters, always null-terminated), using entry 1 of the controls_gamepad_names tag as
 * the axis name.
 *
 * @address 0x491340
 */
void BindingNames::chimera__axis_text(int16_t axis_index, uint8_t direction, uint16_t *out_text)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;
    uint16_t direction_name[9];

    tag_id = tag_lookup(0x75737472,
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_gamepad_names");
    halo::input::input_get_axis_direction_name(direction == 0 ? 1 : 0, direction_name);

    source = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if ((int32_t)list->strings.count > 1) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[1];
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    string_format_wide_va_bounded(0x18, out_text, (const uint16_t *)L"%s%d %s", source, axis_index + 1, direction_name);
    out_text[0x18] = 0;
}

}

namespace halo::input {

/**
 * Builds "<gamepad button name><button_index + 1>" (e.g. "Button1") into out_text (24 wide
 * characters, always null-terminated), using entry 0 of the controls_gamepad_names tag as the
 * name.
 *
 * Original register convention: button_index on the stack, out_text in EBX.
 *
 * @address 0x491270
 */
void BindingNames::chimera__button_text(int16_t button_index, uint16_t *out_text)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;

    tag_id = tag_lookup(0x75737472,
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_gamepad_names");
    source = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if ((int32_t)list->strings.count > 0) {
            entry = (UnicodeStringListString *)list->strings.pointer;
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    string_format_wide_va_bounded(0x17, out_text, (const uint16_t *)L"%s%d", source, button_index + 1);
    out_text[0x17] = 0;
}

}

namespace halo::input {

/**
 * Builds "<gamepad pov name><pov_index + 1> <gamepad direction name>" (e.g. "Pov1 north") into
 * out_text (14 wide characters, always null-terminated), using controls_gamepad_names entry 2 as
 * the pov name and entry (direction_index + 3) as the direction name.
 *
 * @address 0x4914c0
 */
void BindingNames::chimera__pov_text(int16_t pov_index, int16_t direction_index, uint16_t *out_text)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *pov_name;
    uint16_t *direction_name;
    int16_t direction_entry_index;

    tag_id = tag_lookup(0x75737472,
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_gamepad_names");

    pov_name = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if ((int32_t)list->strings.count > 2) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[2];
            if ((int32_t)entry->string.size > 0) {
                pov_name = (uint16_t *)entry->string.pointer;
                pov_name[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }

    direction_entry_index = direction_index + 3;
    direction_name = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if (direction_entry_index >= 0 && direction_entry_index < (int32_t)list->strings.count) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[direction_entry_index];
            if ((int32_t)entry->string.size > 0) {
                direction_name = (uint16_t *)entry->string.pointer;
                direction_name[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }

    string_format_wide_va_bounded(0xe, out_text, (const uint16_t *)L"%s%d %s", pov_name, pov_index + 1, direction_name);
    out_text[0xd] = 0;
}

}

extern "C" { extern char input_action_names[k_input_action_count][0x10]; }
extern "C" { extern int32_t _stricmp(const char *a, const char *b); }
namespace halo::input {

/**
 * Resolves a game-control/action name (case-insensitive) to its input_action index, or
 * k_control_binding_unbound (0x7fff) if none match.
 *
 * Original register convention: name in EBX.
 *
 * @address 0x48fe60
 */
int16_t BindingNames::action_name_to_index(char *name)
{
    char *entry;
    int16_t index;

    index = 0;
    entry = input_action_names[0];
    do {
        if (_stricmp(name, entry) == 0) {
            return index;
        }
        entry = entry + 0x10;
        index = index + 1;
    } while (index < k_input_action_count);
    return k_control_binding_unbound;
}

}

namespace halo::input {

/**
 * Resolves an axis-direction display name string (ASCII, case-insensitive) back to its numeric
 * direction index (0 or 1), or 0xffff if neither matches.
 *
 * @address 0x4911f0
 */
int16_t BindingNames::axis_direction_name_to_index(char *name)
{
    uint32_t direction_index;
    uint32_t length;
    uint32_t i;
    char ascii[12];
    uint16_t wide[9];

    direction_index = 0;
    for (;;) {
        halo::input::input_get_axis_direction_name((int16_t)direction_index, wide);
        length = (uint32_t)wcslen((const wchar_t *)wide);
        if (length < 9) {
            for (i = 0; i < length; i++) {
                ascii[i] = ((uint8_t *)wide)[i * 2 + 1] == 0 ? ((char *)wide)[i * 2] : ' ';
            }
            ascii[i] = '\0';
        }
        if (_stricmp(name, ascii) == 0) {
            break;
        }
        direction_index = direction_index + 1;
        if ((int32_t)direction_index > 1) {
            return -1;
        }
    }
    return (int16_t)direction_index;
}

}

namespace halo::input {

/**
 * Fetches the display name of axis direction direction_index (0 or 1) from the
 * controls_axis_direction_names UnicodeStringList tag into out_name (9 wide characters, always
 * null-terminated).
 *
 * Original register convention: direction_index in EAX, out_name in EBX.
 *
 * @address 0x491180
 */
void BindingNames::get_axis_direction_name(int16_t direction_index, uint16_t *out_name)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;

    tag_id = tag_lookup(0x75737472,
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_axis_direction_names");
    source = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if (direction_index >= 0 && direction_index < (int32_t)list->strings.count) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[direction_index];
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    wcsncpy((wchar_t *)out_name, (const wchar_t *)source, 9);
    out_name[8] = 0;
}

}

namespace halo::input {

/**
 * Given a device-input descriptor, dispatches to the correct per-device name/text formatting
 * routine (keyboard key, mouse button/axis, or joystick button/axis/pov), writing the result
 * into out_text. Does nothing for a device_type/input_kind combination that is not recognized
 * (keyboard with input_kind != button, or a device_type outside 1..3).
 *
 * @address 0x48c7f0
 */
void BindingNames::get_binding_display_name(control_binding_descriptor *binding, uint16_t *out_text)
{
    switch (binding->device_type) {
    case _control_device_keyboard:
        if (binding->input_kind == _control_input_button) {
            halo::input::input_get_keyboard_key_name(binding->input_index, out_text);
        }
        break;

    case _control_device_mouse:
        if (binding->input_kind == _control_input_button) {
            halo::input::input_get_mouse_button_name(binding->input_index, out_text);
        } else if (binding->input_kind == _control_input_axis) {
            halo::input::input_get_mouse_axis_name(binding->input_index, binding->direction == 1, out_text);
        }
        break;

    case _control_device_gamepad:
        switch (binding->input_kind) {
        case _control_input_button:
            halo::input::chimera__button_text(binding->input_index, out_text);
            break;
        case _control_input_axis:
            halo::input::chimera__axis_text(binding->input_index, binding->direction == 1, out_text);
            break;
        case _control_input_pov:
            halo::input::chimera__pov_text(binding->input_index, (int16_t)binding->direction, out_text);
            break;
        }
        break;

    default:
        break;
    }
}

}

namespace halo::input {

/**
 * Fetches the display name of keyboard key key_index from the
 * controls_keyboard_button_names UnicodeStringList tag into out_name (24 wide characters,
 * always null-terminated). Falls back to missing_string_text when the tag is missing or
 * key_index is out of range.
 *
 * Original register convention: key_index in EAX, out_name in EBX.
 *
 * @address 0x490e30
 */
void BindingNames::get_keyboard_key_name(int16_t key_index, uint16_t *out_name)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;

    tag_id = tag_lookup(0x75737472,
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_keyboard_button_names");
    source = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if (key_index >= 0 && key_index < (int32_t)list->strings.count) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[key_index];
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    wcsncpy((wchar_t *)out_name, (const wchar_t *)source, 0x18);
    out_name[0x17] = 0;
}

}

namespace halo::input {

/**
 * Builds the display name for mouse axis axis_index (0..2) by combining the tag-provided axis
 * name (controls_mouse_button_names entries 8+) with a trailing direction suffix ("+"/"-"),
 * into out_name (33 wide characters, always null-terminated).
 *
 * Original register convention: axis_index and direction on the stack, out_name in ESI.
 *
 * @address 0x491010
 */
void BindingNames::get_mouse_axis_name(int16_t axis_index, uint8_t direction, uint16_t *out_name)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;
    int16_t lookup_index;
    uint16_t direction_name[9];

    tag_id = tag_lookup(0x75737472,
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_mouse_button_names");
    lookup_index = axis_index + 8;
    source = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if (lookup_index >= 0 && lookup_index < (int32_t)list->strings.count) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[lookup_index];
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    wcsncpy((wchar_t *)out_name, (const wchar_t *)source, 0x21);
    halo::input::input_get_axis_direction_name(direction != 0, direction_name);
    wcscat((wchar_t *)out_name, L" ");
    wcscat((wchar_t *)out_name, (const wchar_t *)direction_name);
    out_name[0x20] = 0;
}

}

namespace halo::input {

/**
 * Fetches the display name of mouse button button_index from the
 * controls_mouse_button_names UnicodeStringList tag into out_name (24 wide characters, always
 * null-terminated). Falls back to missing_string_text when the tag is missing or button_index
 * is out of range.
 *
 * Original register convention: button_index in EAX, out_name in EBX.
 *
 * @address 0x490f20
 */
void BindingNames::get_mouse_button_name(int16_t button_index, uint16_t *out_name)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;

    tag_id = tag_lookup(0x75737472,
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_mouse_button_names");
    source = missing_string_text;
    if (tag_id != (datum_index)0xffffffff) {
        list = (UnicodeStringList *)tag_instances[(uint16_t)tag_id].data;
        if (button_index >= 0 && button_index < (int32_t)list->strings.count) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[button_index];
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    wcsncpy((wchar_t *)out_name, (const wchar_t *)source, 0x18);
    out_name[0x17] = 0;
}

}

extern "C" { extern char pov_direction_names[8][10]; }
namespace halo::input {

/**
 * Resolves a POV-hat compass direction name (case-insensitive) back to its numeric direction
 * index (0 north .. 7 northwest), or -1 if none match.
 *
 * Original register convention: name in EBX.
 *
 * @address 0x491480
 */
int16_t BindingNames::joystick_pov_direction_name_to_index(char *name)
{
    char *entry;
    int16_t index;

    index = 0;
    entry = pov_direction_names[0];
    while (index < 8) {
        if (_stricmp(name, entry) == 0) {
            return index;
        }
        entry = entry + 10;
        index = index + 1;
    }
    return -1;
}

}

namespace halo::input {

/**
 * Resolves a keyboard key display name string (ASCII, case-insensitive) back to its numeric key
 * index (0 .. k_control_keyboard_key_count - 1), or 0xffff if none match.
 *
 * @address 0x490ea0
 */
uint32_t BindingNames::keyboard_key_name_to_index(char *name)
{
    uint32_t key_index;
    uint32_t length;
    uint32_t i;
    char ascii[24];
    uint16_t wide[24];

    key_index = 0;
    for (;;) {
        halo::input::input_get_keyboard_key_name((int16_t)key_index, wide);
        length = (uint32_t)wcslen((const wchar_t *)wide);
        if (length < 0x18) {
            for (i = 0; i < length; i++) {
                ascii[i] = ((uint8_t *)wide)[i * 2 + 1] == 0 ? ((char *)wide)[i * 2] : ' ';
            }
            ascii[i] = '\0';
        }
        if (_stricmp(name, ascii) == 0) {
            break;
        }
        key_index = key_index + 1;
        if ((int32_t)key_index > (int32_t)k_control_keyboard_key_count - 1) {
            return 0xffff;
        }
    }
    return key_index & 0xffff;
}

}

namespace halo::input {

/**
 * Resolves a mouse axis-plus-direction display name string (ASCII, case-insensitive) back to
 * its axis index (0 .. k_control_mouse_axis_count - 1) and direction (1 or 0, written to
 * *out_direction), or 0xffff if none match.
 *
 * @address 0x4910c0
 */
uint32_t BindingNames::mouse_axis_name_to_index(char *name, uint8_t *out_direction)
{
    static const uint8_t k_directions[2] = { 1, 0 };
    uint32_t axis_index;
    int32_t dir;
    uint32_t length;
    uint32_t i;
    char ascii[36];
    uint16_t wide[33];

    for (axis_index = 0; axis_index <= (uint32_t)k_control_mouse_axis_count - 1; axis_index++) {
        for (dir = 0; dir < 2; dir++) {
            halo::input::input_get_mouse_axis_name((int16_t)axis_index, k_directions[dir], wide);
            length = (uint32_t)wcslen((const wchar_t *)wide);
            if (length < 0x21) {
                for (i = 0; i < length; i++) {
                    ascii[i] = ((uint8_t *)wide)[i * 2 + 1] == 0 ? ((char *)wide)[i * 2] : ' ';
                }
                ascii[i] = '\0';
            }
            if (_stricmp(name, ascii) == 0) {
                *out_direction = k_directions[dir];
                return axis_index & 0xffff;
            }
        }
    }
    return 0xffff;
}

}

namespace halo::input {

/**
 * Resolves a mouse button display name string (ASCII, case-insensitive) back to its numeric
 * button index (0 .. k_control_mouse_button_count - 1), or 0xffff if none match.
 *
 * @address 0x490f90
 */
uint32_t BindingNames::mouse_button_name_to_index(char *name)
{
    uint32_t button_index;
    uint32_t length;
    uint32_t i;
    char ascii[24];
    uint16_t wide[24];

    button_index = 0;
    for (;;) {
        halo::input::input_get_mouse_button_name((int16_t)button_index, wide);
        length = (uint32_t)wcslen((const wchar_t *)wide);
        if (length < 0x18) {
            for (i = 0; i < length; i++) {
                ascii[i] = ((uint8_t *)wide)[i * 2 + 1] == 0 ? ((char *)wide)[i * 2] : ' ';
            }
            ascii[i] = '\0';
        }
        if (_stricmp(name, ascii) == 0) {
            break;
        }
        button_index = button_index + 1;
        if ((int32_t)button_index > (int32_t)k_control_mouse_button_count - 1) {
            return 0xffff;
        }
    }
    return button_index & 0xffff;
}

}
