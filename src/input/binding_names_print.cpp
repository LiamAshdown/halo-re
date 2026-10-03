/**
 * Binding display names and the name to index parsers used by the bind console commands and the controls menu.
 */

#include "tags.h"
#include "halo/text/api.hpp"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "cache.h"
#include "input.h"
#include <wchar.h>
#include <string.h>

#include "halo/input/binding_names.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "link/calls.hpp"
#include "halo/input/state.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/core/datum.hpp"

#define k_gamepad_names_tag_path \
    "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_gamepad_names"
#define k_mouse_button_names_tag_path \
    "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_mouse_button_names"
#define k_axis_direction_names_tag_path \
    "ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_axis_direction_names"
static uint16_t *lookup_named_string(const char *tag_path, int32_t index)
{
    datum_index tag_id;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    uint16_t *source;

    source = halo::input::input_state().missing_string_text;
    tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list,
        (char *)tag_path);
    if (tag_id != halo::k_dword_none) {
        list = (UnicodeStringList *)halo::cache::globals().tag_instances[(uint16_t)tag_id].data;
        if (index >= 0 && index < (int32_t)list->strings.count) {
            entry = &((UnicodeStringListString *)list->strings.pointer)[index];
            if ((int32_t)entry->string.size > 0) {
                source = (uint16_t *)entry->string.pointer;
                source[(entry->string.size >> 1) - 1] = 0;
            }
        }
    }
    return source;
}

static void narrow_copy(char *out, const uint16_t *wide, uint32_t max_len)
{
    uint32_t length;
    uint32_t i;

    length = (uint32_t)wcslen((const wchar_t *)wide);
    if (length < max_len) {
        for (i = 0; i < length; i++) {
            out[i] = ((const uint8_t *)wide)[i * 2 + 1] == 0 ? ((const char *)wide)[i * 2] : ' ';
        }
        out[i] = '\0';
    }
}

static void action_name_copy(char *action_name, int16_t action_index)
{
    const char *name = (action_index == (int16_t)k_input_unbound) ? "none" : halo::input::input_state().input_action_names[action_index];
    strncpy(action_name, name, 0x10);
}

namespace halo::input {

/**
 * Console/debug routine that lists every currently bound keyboard, mouse, and joystick input
 * alongside the game control it triggers, one console_printf_verbose line per bound input.
 *
 * @address 0x48bea0
 */
void BindingNames::print_bound_controls(void)
{
    int16_t key_index;
    int16_t button_index;
    int16_t axis_index;
    int32_t slot;
    int32_t device;
    int32_t button_count;
    int32_t axis_count;
    int32_t pov_count;
    int32_t i;
    int32_t octant;
    char action_name[0x10];
    uint16_t wide_name[0x21];
    uint16_t wide_name2[0x21];
    wchar_t formatted[0x19];
    char ascii[0x21];

    for (key_index = 0; key_index < (int16_t)k_control_keyboard_key_count; key_index++) {
        if (input_state().keyboard_bindings[key_index] != k_input_unbound) {
            action_name_copy(action_name, input_state().keyboard_bindings[key_index]);
            halo::input::input_get_keyboard_key_name(key_index, wide_name);
            narrow_copy(ascii, wide_name, 0x18);
            console_printf_verbose(0, (char *)"%s key bound to %s", ascii, action_name);
        }
    }

    for (button_index = 0; button_index < k_control_mouse_button_count; button_index++) {
        if (input_state().mouse_button_bindings[button_index] != k_input_unbound) {
            action_name_copy(action_name, input_state().mouse_button_bindings[button_index]);

            wcsncpy((wchar_t *)wide_name, (const wchar_t *)lookup_named_string(k_mouse_button_names_tag_path, button_index), 0x18);
            wide_name[0x17] = 0;
            narrow_copy(ascii, wide_name, 0x18);
            console_printf_verbose(0, (char *)"%s mouse button bound to %s", ascii, action_name);
        }
    }

    for (axis_index = 0; axis_index < k_control_mouse_axis_count; axis_index++) {
        if (input_state().mouse_axis_bindings[axis_index][0] != k_input_unbound) {
            action_name_copy(action_name, input_state().mouse_axis_bindings[axis_index][0]);
            halo::input::input_get_mouse_axis_name(axis_index, 1, wide_name);
            narrow_copy(ascii, wide_name, 0x21);
            console_printf_verbose(0, (char *)"%s mouse axis bound to %s", ascii, action_name);
        }
        if (input_state().mouse_axis_bindings[axis_index][1] != k_input_unbound) {
            action_name_copy(action_name, input_state().mouse_axis_bindings[axis_index][1]);
            halo::input::input_get_mouse_axis_name(axis_index, 0, wide_name);
            narrow_copy(ascii, wide_name, 0x21);
            console_printf_verbose(0, (char *)"%s mouse axis bound to %s", ascii, action_name);
        }
    }

    for (slot = 0; slot < k_control_gamepad_count; slot++) {
        device = globals().joystick_slot_devices[slot];
        button_count = 0;
        axis_count = 0;
        pov_count = 0;
        if (device != -1) {
            button_count = input_state().input_devices[device].button_count;
            axis_count = input_state().input_devices[device].axis_count;
            pov_count = input_state().input_devices[device].pov_count;
        }

        for (i = 0; i < button_count; i++) {
            if (input_state().gamepad_button_bindings[slot][i] != k_input_unbound) {
                action_name_copy(action_name, input_state().gamepad_button_bindings[slot][i]);
                halo::text::string_format_wide_va_bounded(0x17, wide_name, reinterpret_cast<const uint16_t *>(L"%s%d"),
                                               lookup_named_string(k_gamepad_names_tag_path, 0), i + 1);
                wide_name[0x17] = 0;
                narrow_copy(ascii, wide_name, 0x18);
                console_printf_verbose(0, (char *)"%s on gamepad %d bound to %s", ascii, slot, action_name);
            }
        }

        for (i = 0; i < axis_count; i++) {
            if (input_state().gamepad_axis_bindings[slot][i][0] != k_input_unbound) {
                action_name_copy(action_name, input_state().gamepad_axis_bindings[slot][i][0]);
                wcsncpy((wchar_t *)wide_name2, (const wchar_t *)lookup_named_string(k_axis_direction_names_tag_path, 0), 9);
                wide_name2[8] = 0;
                halo::text::string_format_wide_va_bounded(0x18, reinterpret_cast<uint16_t *>(formatted), reinterpret_cast<const uint16_t *>(L"%s%d %s"),
                                               lookup_named_string(k_gamepad_names_tag_path, 1), i + 1, wide_name2);
                formatted[0x18] = 0;
                narrow_copy(ascii, (const uint16_t *)formatted, 0x19);
                console_printf_verbose(0, (char *)"%s on gamepad %d bound to %s", ascii, slot, action_name);
            }
            if (input_state().gamepad_axis_bindings[slot][i][1] != k_input_unbound) {
                action_name_copy(action_name, input_state().gamepad_axis_bindings[slot][i][1]);
                wcsncpy((wchar_t *)wide_name2, (const wchar_t *)lookup_named_string(k_axis_direction_names_tag_path, 1), 9);
                wide_name2[8] = 0;
                halo::text::string_format_wide_va_bounded(0x18, reinterpret_cast<uint16_t *>(formatted), reinterpret_cast<const uint16_t *>(L"%s%d %s"),
                                               lookup_named_string(k_gamepad_names_tag_path, 1), i + 1, wide_name2);
                formatted[0x18] = 0;
                narrow_copy(ascii, (const uint16_t *)formatted, 0x19);
                console_printf_verbose(0, (char *)"%s on gamepad %d bound to %s", ascii, slot, action_name);
            }
        }

        for (i = 0; i < pov_count; i++) {
            for (octant = 0; octant < k_control_gamepad_pov_direction_count; octant++) {
                if (input_state().gamepad_pov_bindings[slot][i][octant] != k_input_unbound) {
                    action_name_copy(action_name, input_state().gamepad_pov_bindings[slot][i][octant]);
                    halo::input::chimera__pov_text((int16_t)i, (int16_t)octant, wide_name);
                    narrow_copy(ascii, wide_name, 0xe);
                    console_printf_verbose(0, (char *)"%s on gamepad %d bound to %s", ascii, slot, action_name);
                }
            }
        }
    }
}

}

#undef k_gamepad_names_tag_path
#undef k_mouse_button_names_tag_path
#undef k_axis_direction_names_tag_path
