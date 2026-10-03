#include "halo/interface/ifr1_controls_bindings.hpp"
#include <string.h>
#include <wchar.h>
#include "halo/memory/api.hpp"

extern "C" {
extern uint8_t controls_row_device_mask_table[];
extern uint16_t controls_action_name_buffer[];
extern const uint16_t hud_text_unbound[];
extern uint8_t controls_enumerate_next_assignable_action(int32_t device, int16_t *record, const char *action_name,
                                                          uint8_t accept_reserved_on_retry);
extern void input_get_binding_display_name(uint8_t *binding, uint16_t *out_name);
extern uint32_t wcslen_halo(const uint16_t *text);
extern uint8_t controls_menu_list_mode;
extern int32_t selected_saved_item;
extern uint8_t saved_item_working_copy[0x1ffc];
extern uint8_t control_keyboard_scan_table[0xda];
extern uint32_t control_mouse_button_scan_table[7];
extern uint32_t input_default_profile_guid[4];
extern int32_t input_device_default_profile_tag_find(input_guid guid, uint8_t *out_profile);
extern void control_profile_reset_digital_bindings(uint8_t *profile);
extern void control_profile_reset_analog_bindings(uint8_t *profile);
extern uint8_t control_profile_finalize_slot(uint8_t *profile, int32_t preset_index);
extern void widget_play_sound_effect(int16_t effect_id);
extern uint8_t controls_action_table[][0x18];
extern int32_t controls_current_binding_table[][3];
extern int16_t input_action_name_to_index(const char *action_name);
extern void control_profile_clear_binding(const int16_t *record);
extern controls_device_label controls_device_labels[0x10];
extern void controls_binding_row_widget_update(int32_t action_index, widget_instance *row, int32_t device);
extern heap *widget_memory_pool;
extern int32_t controls_capture_row;
extern int32_t controls_selected_device;
extern uint8_t controls_input_capture_flags;
extern int16_t controls_captured_binding[6];
extern uint32_t controls_input_capture_buffer[0xa0];
extern int32_t controls_binding_list_refresh_rows(widget_instance *widget, int32_t page);
extern uint8_t controls_key_is_bindable(int32_t control);
extern uint8_t controls_binding_clear(int32_t action_index, int32_t device);
extern uint8_t controls_action_column_is_bindable(int32_t slot, int32_t action_index);
extern void control_profile_set_binding(const int16_t *record, int32_t action);
extern void input_last_used_binding_copy(const int16_t *record, int32_t action);
extern uint16_t *controls_action_display_name(int32_t device, const char *action_name);
extern uint8_t controls_device_sensitivity_a[];
extern uint8_t controls_device_sensitivity_b[];
extern int32_t controls_device_label_count;
extern tag_instance *tag_instances;
extern datum_index tag_lookup(tag_group group, char *path);
extern void controls_device_label_add(const uint16_t *name, int32_t device_type);
extern int32_t controls_reserved_action_table[9];
extern uint8_t control_profile_find_binding_for_action(const char *action_name, const int16_t *binding);
}

static widget_instance *controls_find_child_of_type(widget_instance *child, int16_t type)
{
    while (child != 0 && child->widget_type != type) {
        child = child->next_sibling;
    }
    return child;
}

static void controls_set_dimmed(widget_instance *widget, uint8_t dimmed)
{
    widget->hidden = dimmed;
    widget->scale = dimmed ? 0.333f : 1.0f;
}

static void controls_set_cell_text(widget_instance *cell, const uint16_t *text)
{
    uint16_t *buffer = (uint16_t *)halo::memory::heap_reallocate(cell->text, 0x40, widget_memory_pool);

    cell->text = buffer;
    if (buffer != 0) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)text, 0x1f);
        buffer[0x1f] = 0;
    }
}

static void controls_set_cell_dimmed(widget_instance *cell, uint8_t dimmed)
{
    cell->hidden = dimmed;
    cell->scale = dimmed ? 0.333f : 1.0f;
}

static widget_instance *controls_find_spinner(widget_instance *child)
{
    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

static void controls_spinner_set_from_byte(widget_instance *spinner, uint8_t value)
{
    if (value == 0 || value > 10) {
        spinner->selection_index = 0;
    } else {
        spinner->selection_index = (int16_t)(value - 1);
    }
}

namespace halo::interface {

/**
 * Original engine function controls_action_column_is_bindable; the author notes are in
 * docs/original/interface/controls_action_column_is_bindable.txt.
 * blam-cc: ECX -> slot, EDX -> action_index
 *
 * @address 0x4b4df0
 */
uint8_t ControlsBindings::action_column_is_bindable(int32_t slot, int32_t action_index)
{
    if (slot == 0) {
        return (controls_row_device_mask_table[action_index * 0x18] & 1) == 0;
    }
    if (slot != 1) {
        return 1;
    }
    return (controls_row_device_mask_table[action_index * 0x18] & 2) == 0;
}

/**
 * Original engine function controls_action_display_name; the author notes are in
 * docs/original/interface/controls_action_display_name.txt.
 * blam-cc: device -> EAX, action_name -> EDI
 *
 * @address 0x4b44c0
 */
uint16_t * ControlsBindings::action_display_name(int32_t device, const char *action_name)
{
    int16_t record[6];

    controls_action_name_buffer[0] = 0;
    if (controls_enumerate_next_assignable_action(device, record, action_name, 1) != 0) {
        input_get_binding_display_name((uint8_t *)record, controls_action_name_buffer);
    }
    if (controls_action_name_buffer[0] == 0) {
        wcscpy((wchar_t *)controls_action_name_buffer, (const wchar_t *)hud_text_unbound);
    }
    return controls_action_name_buffer;
}

/**
 * Original engine function controls_apply_preset; the author notes are in
 * docs/original/interface/controls_apply_preset.txt.
 *
 * @address 0x4b4c50
 */
uint8_t ControlsBindings::apply_preset(widget_instance *widget)
{
    uint8_t result = 0;

    if (controls_menu_list_mode != 0) {
        widget_instance *menu = widget->parent->parent->first_child;
        controls_find_child_of_type(menu->first_child, 2)->selection_index = 2;
        controls_find_child_of_type(menu->next_sibling->first_child, 2)->selection_index = 2;
        result = 1;
    } else if ((selected_saved_item & 0xf) == 0) {
        int32_t selection = widget->parent->parent->first_child->first_child->next_sibling->selection_index;

        if (selection == 1 || selection >= 2) {
            if (selection == 1) {
                selection = 2;
            }
            selection -= 2;
            if (selection >= 0 && selection <= 4) {
                result = control_profile_finalize_slot(saved_item_working_copy, selection);
                widget_play_sound_effect(2);
                return result;
            }
        } else {
            uint8_t profile[0x1ffc - 0x10];
            input_guid guid;

            memcpy(&guid, input_default_profile_guid, sizeof(guid));
            if (input_device_default_profile_tag_find(guid, profile) != -1) {
                memcpy(control_keyboard_scan_table, profile + 0x134, 0xda);
                memcpy(control_mouse_button_scan_table, profile + 0x20e, sizeof(control_mouse_button_scan_table));
            } else {
                control_profile_reset_digital_bindings(saved_item_working_copy);
                control_profile_reset_analog_bindings(saved_item_working_copy);
            }
            result = 1;
        }
    }
    widget_play_sound_effect(2);
    return result;
}

/**
 * Original engine function controls_binding_clear; the author notes are in
 * docs/original/interface/controls_binding_clear.txt.
 * blam-cc: action_index -> EAX
 *
 * @address 0x4b4e20
 */
uint8_t ControlsBindings::binding_clear(int32_t action_index, int32_t device)
{
    uint8_t *entry = controls_action_table[action_index];
    int16_t record[6];

    if (device == 0 && (entry[0x14] & 1) != 0) {
        return 0;
    }
    if (device == 1 && (entry[0x14] & 2) != 0) {
        return 0;
    }
    if (controls_enumerate_next_assignable_action(device, record, (const char *)entry, 0) == 0) {
        return 0;
    }
    if (device >= 2 && (entry[0x14] & 4) != 0) {
        uint8_t *profile = (selected_saved_item & 0xf) != 0 ? (uint8_t *)0 : saved_item_working_copy;
        int16_t gamepad = record[1];
        int16_t control = record[3];

        if (*(int16_t *)(profile + 0x32c + gamepad * 4) == control) {
            *(int16_t *)(profile + 0x32c + gamepad * 4) = -1;
        }
        if (*(int16_t *)(profile + 0x32a + gamepad * 4) == control) {
            *(int16_t *)(profile + 0x32a + gamepad * 4) = -1;
        }
        return 1;
    }
    {
        int32_t *current = controls_current_binding_table[input_action_name_to_index((const char *)entry)];
        if (memcmp(current, record, 12) == 0) {
            current[0] = 0;
            current[1] = 0;
            current[2] = 0;
        }
    }
    control_profile_clear_binding(record);
    return 1;
}

/**
 * Original engine function controls_binding_list_refresh_rows; the author notes are in
 * docs/original/interface/controls_binding_list_refresh_rows.txt.
 * blam-cc: widget -> EAX
 *
 * @address 0x4b4790
 */
int32_t ControlsBindings::binding_list_refresh_rows(widget_instance *widget, int32_t page)
{
    widget_instance *header = widget->first_child;
    widget_instance *row;
    int32_t focus = -1;
    int32_t device_label;
    int32_t i;

    if (header->parent->focused_child == header) {
        focus = 0;
    }
    device_label = header->first_child->next_sibling->selection_index;
    header = header->next_sibling;
    if (header->parent->focused_child == header) {
        focus = 1;
    }
    row = header->next_sibling;
    for (i = 0; i < 8; i++) {
        controls_binding_row_widget_update(page * 8 + i, row, controls_device_labels[device_label].device_type);
        if (row->parent->focused_child == row) {
            focus = 2;
        }
        row = row->next_sibling;
    }
    return focus;
}

/**
 * Original engine function controls_binding_row_handle_input; the author notes are in
 * docs/original/interface/controls_binding_row_handle_input.txt.
 *
 * @address 0x4b4f30
 */
uint8_t ControlsBindings::binding_row_handle_input(widget_instance *screen)
{
    static const uint16_t empty_text[1] = {0};
    widget_instance *header = screen->first_child;
    widget_instance *spinner;
    widget_instance *footer;
    int32_t device;
    int32_t page;
    int32_t focus;
    int32_t part;
    int32_t i;

    controls_set_dimmed(header, controls_capture_row != -1);
    spinner = header->first_child->next_sibling;
    device = spinner->selection_index;
    if (device >= 1) {
        device++;
    }
    controls_selected_device = device;
    spinner->list_render_data = halo::memory::heap_reallocate(spinner->list_render_data, 0x80, widget_memory_pool);
    if (spinner->list_render_data != 0) {
        int32_t label = spinner->selection_index;
        wcsncpy((wchar_t *)spinner->list_render_data,
                (const wchar_t *)(label >= 0 && label < 0x10 ? controls_device_labels[label].name : empty_text), 0x3f);
        ((uint16_t *)spinner->list_render_data)[0x3f] = 0;
    }

    header = header->next_sibling;
    controls_set_dimmed(header, controls_capture_row != -1);
    page = header->first_child->next_sibling->selection_index;
    focus = controls_binding_list_refresh_rows(screen, page);
    part = -1;
    switch (focus + 1) {
    case 1: case 2: part = focus; break;
    case 3: part = (controls_capture_row != -1) + 2; break;
    }
    if (part != -1) {
        screen->extended_description->selection_index = (int16_t)part;
        screen->extended_description->state = 1;
    } else {
        screen->extended_description->state = 0;
    }
    footer = header->next_sibling;
    for (i = 0; i < 8; i++) {
        footer = footer->next_sibling;
    }
    controls_set_dimmed(footer, controls_capture_row != -1);
    controls_set_dimmed(footer->first_child->next_sibling, controls_selected_device < 2);

    if (controls_capture_row == -1 || controls_input_capture_flags == 1 || (controls_input_capture_flags & 8) == 0 ||
        controls_captured_binding[0] == 0) {
        return 1;
    }

    {
        int16_t record[6];
        int32_t action_index = controls_capture_row + page * 8;
        int16_t kind;
        int16_t sound;

        memcpy(record, controls_captured_binding, sizeof(record));
        kind = record[0];
        if (kind == 1) {
            if (record[3] == 0) {
                sound = 3;
                goto finish_capture;
            }
            if (record[3] == 0x1d) {
                controls_binding_clear(action_index, device);
                if (device == 0) {
                    controls_binding_clear(action_index, 1);
                }
                sound = 2;
                goto finish_capture;
            }
            if (device != 0) {
                goto drop_control;
            }
            if (controls_key_is_bindable(record[3]) == 0) {
                widget_play_sound_effect(4);
                goto drop_control;
            }
        } else if (kind == 2) {
            if (device != 0) {
                goto drop_control;
            }
            if (record[2] != 0 && !(record[2] == 1 && record[3] == 2)) {
                goto drop_control;
            }
        } else if (kind == 3) {
            if (record[1] != device - 2) {
                goto drop_control;
            }
        } else {
            goto drop_control;
        }

        {
            const char *action_name = (const char *)controls_action_table[action_index];
            int16_t action = input_action_name_to_index(action_name);

            if (action == 0x7fff || controls_action_column_is_bindable(kind == 2 ? 1 : device, action_index) == 0) {
                sound = 4;
                goto finish_capture;
            }
            if (kind != 3 || (controls_action_table[action_index][0x14] & 4) == 0) {
                int16_t previous[6];
                if (controls_enumerate_next_assignable_action(kind == 2 ? 1 : device, previous, action_name, 0) != 0) {
                    control_profile_clear_binding(previous);
                }
                control_profile_clear_binding(record);
            }
            control_profile_set_binding(record, action);
            input_last_used_binding_copy(record, action);
            sound = 2;
        }

finish_capture:
        widget_play_sound_effect(sound);
        {
            widget_instance *row = screen->first_child->next_sibling->next_sibling;
            widget_instance *cell;

            for (i = controls_capture_row; i != 0; i--) {
                row = row->next_sibling;
            }
            cell = row->first_child->next_sibling;
            *(uint8_t *)&cell->selection_direction = 0;
            cell = cell->next_sibling->first_child;
            *(uint8_t *)&cell->selection_direction = 0;
            *(uint8_t *)&cell->next_sibling->selection_direction = 0;
        }
        controls_input_capture_flags &= 0xf7;
        memset(controls_input_capture_buffer, 0, sizeof(controls_input_capture_buffer));
        controls_capture_row = -1;
drop_control:
        memset(controls_captured_binding, 0, sizeof(controls_captured_binding));
    }
    return 1;
}

/**
 * Original engine function controls_binding_row_widget_update; the author notes are in
 * docs/original/interface/controls_binding_row_widget_update.txt.
 * blam-cc: action_index -> EAX
 *
 * @address 0x4b4520
 */
void ControlsBindings::binding_row_widget_update(int32_t action_index, widget_instance *row, int32_t device)
{
    static const uint16_t empty_text[1] = {0};
    uint8_t *entry = controls_action_table[action_index];
    const char *action_name = (const char *)entry;
    int32_t bindable = *(int32_t *)(entry + 0x10);
    widget_instance *cell = row->first_child->next_sibling;
    uint8_t some_cell_active = 0;

    row->first_child->selection_index = *(int16_t *)(entry + 0x10);
    if (device >= 2) {
        cell->state = 1;
        if (bindable != 0) {
            controls_set_cell_text(cell, controls_action_display_name(device, action_name));
            some_cell_active = 1;
        } else {
            controls_set_cell_text(cell, empty_text);
        }
    } else {
        cell->state = 0;
    }

    cell = cell->next_sibling;
    if (device < 2) {
        widget_instance *keyboard = cell->first_child;
        widget_instance *mouse = keyboard->next_sibling;

        cell->state = 1;
        if (bindable != 0) {
            controls_set_cell_text(keyboard, controls_action_display_name(0, action_name));
            if ((entry[0x14] & 1) != 0) {
                controls_set_cell_dimmed(keyboard, 1);
            } else {
                controls_set_cell_dimmed(keyboard, 0);
                some_cell_active = 1;
            }
            controls_set_cell_text(mouse, controls_action_display_name(1, action_name));
            if ((entry[0x14] & 2) != 0) {
                controls_set_cell_dimmed(mouse, 1);
            } else {
                controls_set_cell_dimmed(mouse, 0);
                controls_set_cell_dimmed(row, 0);
                return;
            }
        } else {
            controls_set_cell_text(keyboard, empty_text);
            controls_set_cell_text(mouse, empty_text);
        }
    } else {
        cell->state = 0;
    }
    controls_set_cell_dimmed(row, some_cell_active ? 0 : 1);
}

/**
 * Original engine function controls_binding_rows_toggle_device_mode; the author notes are in
 * docs/original/interface/controls_binding_rows_toggle_device_mode.txt.
 * blam-cc: widget -> ESI
 *
 * @address 0x4b53a0
 */
void ControlsBindings::binding_rows_toggle_device_mode(widget_instance *widget, uint8_t mode)
{
    widget_instance *row_a = widget->first_child->next_sibling;
    widget_instance *row_b = row_a->next_sibling;

    if (mode == 0 || controls_selected_device < 2) {
        row_b->state = 0;
        row_b->hidden = 1;
        widget->focused_child = row_a;
        row_a->hidden = 0;
        row_a->state = 1;
    } else if ((selected_saved_item & 0xf) == 0) {
        int32_t device = controls_selected_device;
        widget_instance *group;

        row_b->state = 1;
        row_b->hidden = 0;
        widget->focused_child = row_b;
        row_a->state = 0;
        row_a->hidden = 1;

        group = row_b->first_child->first_child;
        controls_spinner_set_from_byte(controls_find_spinner(group->first_child), controls_device_sensitivity_a[device]);
        group = group->next_sibling;
        controls_spinner_set_from_byte(controls_find_spinner(group->first_child), controls_device_sensitivity_b[device]);
    }
    controls_menu_list_mode = mode;
}

/**
 * VERIFIED against disassembly 0x4b4890..0x4b4972 (2026-09-30): the profile select, the 0x840-dword clear, the
 * tag string terminator patch (((size & ~1) - 2) bytes in), the default label add, and the four 0x220-byte
 * profile label copies match. The function returns nothing: the EAX difference a difftest shows is leftover
 * register content.
 *
 * @address 0x4b4890
 */
void ControlsBindings::build_device_label_table(void)
{
    uint8_t *profile = ((selected_saved_item & 0xf) != 0) ? (uint8_t *)0 : saved_item_working_copy;
    datum_index tag_id = tag_lookup(0x75737472,
        (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\controls_setup\\controls_device_labels");
    int i;
    const uint16_t *tag_supplied_label = (const uint16_t *)L"<missing string>";

    {
        int32_t *clear = (int32_t *)controls_device_labels;
        for (i = 0; i < 0x840; i++) clear[i] = 0;
    }
    controls_device_label_count = 0;

    if (tag_id != (datum_index)-1) {
        int32_t *reflexive = *(int32_t **)&tag_instances[tag_id & 0xffff].data;
        if (*(int32_t *)((uint8_t *)tag_instances[tag_id & 0xffff].data) > 0) {
            uint32_t *item = (uint32_t *)((int32_t *)tag_instances[tag_id & 0xffff].data)[1];
            uint32_t count = item[0];
            (void)reflexive;
            if ((int32_t)count > 0) {
                uint16_t *string_data = (uint16_t *)item[3];
                tag_supplied_label = string_data;
                *(uint16_t *)((uint8_t *)string_data + ((count & 0xfffffffe) - 2)) = 0;
            }
        }
    }
    controls_device_label_add(tag_supplied_label, 0);

    for (i = 0; i < 4; i++) {
        uint16_t *entry = (uint16_t *)(profile + 0x1108 + i * 0x220);
        if (profile != 0 && *entry != 0) {
            uint32_t local[0x88];
            int j;
            uint16_t *src = entry;
            for (j = 0; j < 0x88; j++) {
                local[j] = *(uint32_t *)src;
                src += 2;
            }
            controls_device_label_add((const uint16_t *)local, i + 2);
        }
    }
}

/**
 * Original engine function controls_device_label_add; the author notes are in
 * docs/original/interface/controls_device_label_add.txt.
 *
 * @address 0x4b4830
 */
void ControlsBindings::device_label_add(const uint16_t *name, int32_t device_type)
{
    int i;
    for (i = 0; i < 0x10; i++) {
        if (controls_device_labels[i].name[0] == 0) {
            wcsncpy((wchar_t *)controls_device_labels[i].name, (const wchar_t *)name, 0x104);
            controls_device_labels[i].name[0x104] = 0;
            controls_device_labels[i].device_type = device_type;
            controls_device_label_count++;
            return;
        }
    }
}

/**
 * Original engine function controls_enumerate_next_assignable_action; the author notes are in
 * docs/original/interface/controls_enumerate_next_assignable_action.txt.
 * blam-cc: device -> EAX, record -> ECX, action_name -> EDI
 *
 * @address 0x4b43e0
 */
uint8_t ControlsBindings::enumerate_next_assignable_action(int32_t device, int16_t *record, const char *action_name, uint8_t accept_reserved_on_retry)
{
    uint8_t found = 0;

    ((int32_t *)record)[0] = 0;
    ((int32_t *)record)[1] = 0;
    ((int32_t *)record)[2] = 0;

    if (device == 0) {
        uint8_t accept_reserved = 0;

        record[0] = 1;
        record[1] = 0;
        for (;;) {
            record[3] = 0;
            while (control_profile_find_binding_for_action(action_name, record) != 0) {
                int32_t i;
                uint8_t reserved = 0;

                if (accept_reserved) {
                    found = 1;
                    break;
                }
                for (i = 0; i < 9; i++) {
                    if (controls_reserved_action_table[i] == (int32_t)record[3]) {
                        reserved = 1;
                        break;
                    }
                }
                if (!reserved) {
                    found = 1;
                    break;
                }
                record[3]++;
            }
            if (accept_reserved_on_retry == 0 || found) {
                return found;
            }
            accept_reserved = 1;
            accept_reserved_on_retry = 0;
        }
    }

    record[3] = 0;
    if (device == 1) {
        record[0] = 2;
        record[1] = 0;
    } else {
        record[0] = 3;
        record[1] = (int16_t)(device - 2);
    }
    if (control_profile_find_binding_for_action(action_name, record) != 0) {
        return 1;
    }
    return found;
}

/**
 * Returns 1 (not reserved) unless `action` matches one of the fixed reserved values.
 * blam-cc: action -> EDX
 *
 * @address 0x4b43c0
 */
uint8_t ControlsBindings::key_is_bindable(int32_t action)
{
    int i;
    for (i = 0; i < 9; i++) {
        if (controls_reserved_action_table[i] == action) {
            return 0;
        }
    }
    return 1;
}

}
