#include "halo/interface/ifr1_gamepad_bindings.hpp"
#include <string.h>
#include <wchar.h>
#include "halo/memory/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern int32_t selected_saved_item;
extern uint8_t saved_item_working_copy[0x1ffc];
extern int32_t controls_assigned_gamepad_count;
extern controls_gamepad_record controls_assigned_gamepads[4];
extern controls_gamepad_record controls_available_gamepads[8];
extern int32_t controls_available_gamepad_count;
extern int32_t input_device_count;
extern uint8_t input_devices[];
extern heap *widget_memory_pool;
}

static void controls_gamepad_row_set_disabled(widget_instance *row)
{
    row->hidden = 1;
    row->scale = 0.333f;
    row->background_bitmap_frame = 0;
}

static void controls_gamepad_focus_visible_row(widget_instance *list)
{
    widget_instance *row = list->focused_child;

    while (row != 0 && row->hidden != 0) {
        row = row->previous_sibling;
    }
    list->focused_child = row != 0 ? row : list->first_child;
}

namespace halo::interface {

/**
 * Original engine function controls_gamepad_bindings_restore; the author notes are in
 * docs/original/interface/controls_gamepad_bindings_restore.txt.
 *
 * @address 0x4b5a70
 */
uint8_t GamepadBindings::bindings_restore(void)
{
    uint8_t saved_profile[0x1ffc];
    int32_t i;

    if ((selected_saved_item & 0xf) != 0) {
        return 0;
    }
    memcpy(saved_profile, saved_item_working_copy, sizeof(saved_profile));
    halo::saved_games::control_profile_clear_device_slot_mappings((saved_player_profile *)saved_item_working_copy);
    for (i = 0; i < 4; i++) {
        halo::saved_games::control_profile_reset_slot((saved_player_profile *)saved_item_working_copy, i);
    }
    for (i = 0; i < controls_assigned_gamepad_count; i++) {
        if (halo::saved_games::control_profile_find_or_create_gamepad_slot(&controls_assigned_gamepads[i], (saved_player_profile *)saved_item_working_copy) != 0) {
            halo::saved_games::control_profile_copy_gamepad_bindings_by_key(&controls_assigned_gamepads[i], (saved_player_profile *)saved_item_working_copy, (saved_player_profile *)saved_profile);
        }
    }
    halo::saved_games::control_profile_reestablish_device_slot_mappings((saved_player_profile *)saved_item_working_copy);
    return 1;
}

/**
 * Original engine function controls_gamepad_list_add; the author notes are in
 * docs/original/interface/controls_gamepad_list_add.txt.
 * blam-cc: EAX -> list
 *
 * @address 0x4b5800
 */
uint8_t GamepadBindings::list_add(const controls_gamepad_record *entry, controls_gamepad_record *list)
{
    int32_t capacity;
    int32_t *count;

    if (list == controls_assigned_gamepads) {
        capacity = 4;
        count = &controls_assigned_gamepad_count;
    } else if (list == controls_available_gamepads) {
        capacity = 8;
        count = &controls_available_gamepad_count;
    } else {
        return 0;
    }

    if (*count < capacity) {
        list[*count] = *entry;
        (*count)++;
        return 1;
    }
    return 0;
}

/**
 * Original engine function controls_gamepad_list_find; the author notes are in
 * docs/original/interface/controls_gamepad_list_find.txt.
 * blam-cc: EDX -> list, stack -> entry
 *
 * @address 0x4b5760
 */
int32_t GamepadBindings::list_find(const controls_gamepad_record *entry, controls_gamepad_record *list)
{
    int32_t count;
    int i;

    if (list == controls_assigned_gamepads) {
        count = controls_assigned_gamepad_count;
    } else if (list == controls_available_gamepads) {
        count = controls_available_gamepad_count;
    } else {
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (memcmp(&list[i].product_guid, &entry->product_guid, sizeof(input_guid) + sizeof(int32_t)) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * Original engine function controls_gamepad_list_remove; the author notes are in
 * docs/original/interface/controls_gamepad_list_remove.txt.
 * blam-cc: EDI -> list
 *
 * @address 0x4b5850
 */
uint8_t GamepadBindings::list_remove(const controls_gamepad_record *entry, controls_gamepad_record *list)
{
    int32_t index = halo::interface::controls_gamepad_list_find(entry, list);
    int32_t *count;
    int32_t tail_count;

    if (index == -1) {
        return 0;
    }
    if (list == controls_assigned_gamepads) {
        count = &controls_assigned_gamepad_count;
    } else if (list == controls_available_gamepads) {
        count = &controls_available_gamepad_count;
    } else {
        return 0;
    }

    index = halo::interface::controls_gamepad_list_find(entry, list);
    if (index == -1) {
        return 0;
    }
    tail_count = (*count - 1) - index;
    (*count)--;
    if (tail_count > 0) {
        memmove(&list[index], &list[index + 1], tail_count * sizeof(controls_gamepad_record));
    }
    return 1;
}

/**
 * Original engine function controls_gamepad_lists_load; the author notes are in
 * docs/original/interface/controls_gamepad_lists_load.txt.
 *
 * @address 0x4b58d0
 */
uint8_t GamepadBindings::lists_load(widget_instance *screen)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : (uint8_t *)0;
    widget_instance *nodes[17];
    controls_gamepad_record entry;
    uint8_t have_entry = 0;
    int32_t count;
    int32_t i;

    memset(controls_assigned_gamepads, 0, sizeof(controls_assigned_gamepads));
    memset(controls_available_gamepads, 0, sizeof(controls_available_gamepads));
    controls_assigned_gamepad_count = 0;
    controls_available_gamepad_count = 0;
    if (profile == 0) {
        return 0;
    }

    halo::interface::controls_gamepad_widget_nodes_collect(nodes, screen);
    count = (int16_t)input_device_count;
    for (i = 0; i < count; i++) {
        if ((int16_t)i < input_device_count) {
            memcpy(&entry, input_devices + (int16_t)i * 0x240, sizeof(entry));
            have_entry = 1;
        } else if (!have_entry) {
            continue;
        }
        halo::interface::controls_gamepad_list_add(&entry, controls_available_gamepads);
    }

    for (i = 0; i < 4; i++) {
        const controls_gamepad_record *saved = (const controls_gamepad_record *)(profile + 0x1108) + i;

        if (*(const uint16_t *)saved == 0) {
            continue;
        }
        entry = *saved;
        if (controls_assigned_gamepad_count < 4) {
            controls_assigned_gamepads[controls_assigned_gamepad_count] = *saved;
            controls_assigned_gamepad_count++;
        }
        halo::interface::controls_gamepad_list_remove(&entry, controls_available_gamepads);
    }

    halo::interface::controls_gamepad_lists_refresh(screen);
    if (controls_assigned_gamepad_count > 0) {
        screen->focused_child = nodes[0];
    } else if (controls_available_gamepad_count > 0) {
        screen->focused_child = nodes[5];
    } else {
        screen->focused_child = nodes[15];
    }
    return 1;
}

/**
 * Original engine function controls_gamepad_lists_refresh; the author notes are in
 * docs/original/interface/controls_gamepad_lists_refresh.txt.
 * blam-cc: screen -> ECX
 *
 * @address 0x4b55d0
 */
void GamepadBindings::lists_refresh(widget_instance *screen)
{
    static const uint16_t dashes_text[4] = {'-', '-', '-', 0};
    static const uint16_t empty_text[1] = {0};
    widget_instance *nodes[17];
    int32_t i;

    halo::interface::controls_gamepad_widget_nodes_collect(nodes, screen);

    nodes[0]->hidden = controls_assigned_gamepad_count == 0;
    for (i = 0; i < 4; i++) {
        widget_instance *text = nodes[1 + i]->first_child;
        uint16_t *buffer = (uint16_t *)halo::memory::heap_reallocate(text->text, 0x80, widget_memory_pool);

        text->text = buffer;
        if (buffer == 0) {
            continue;
        }
        if (i < controls_assigned_gamepad_count) {
            wcsncpy((wchar_t *)buffer, (const wchar_t *)&controls_assigned_gamepads[i], 0x3f);
            ((uint16_t *)text->text)[0x3f] = 0;
            text->parent->hidden = 0;
            text->parent->scale = 1.0f;
        } else {
            wcsncpy((wchar_t *)buffer, (const wchar_t *)dashes_text, 0x3f);
            ((uint16_t *)text->text)[0x3f] = 0;
            controls_gamepad_row_set_disabled(text->parent);
        }
    }

    nodes[5]->hidden = controls_available_gamepad_count == 0;
    for (i = 0; i < 8; i++) {
        widget_instance *text = nodes[6 + i]->first_child;
        uint16_t *buffer = (uint16_t *)halo::memory::heap_reallocate(text->text, 0x80, widget_memory_pool);

        text->text = buffer;
        if (buffer == 0) {
            continue;
        }
        if (i < controls_available_gamepad_count) {
            wcsncpy((wchar_t *)buffer, (const wchar_t *)&controls_available_gamepads[i], 0x3f);
            ((uint16_t *)text->text)[0x3f] = 0;
            if (controls_assigned_gamepad_count != 4) {
                text->parent->hidden = 0;
                text->parent->scale = 1.0f;
                continue;
            }
        } else {
            wcsncpy((wchar_t *)buffer, (const wchar_t *)empty_text, 0x3f);
            ((uint16_t *)text->text)[0x3f] = 0;
        }
        controls_gamepad_row_set_disabled(text->parent);
    }
}

/**
 * Original engine function controls_gamepad_toggle_assignment; the author notes are in
 * docs/original/interface/controls_gamepad_toggle_assignment.txt.
 *
 * @address 0x4b5b20
 */
uint8_t GamepadBindings::toggle_assignment(widget_instance *row)
{
    widget_instance *list = row->parent;
    widget_instance *screen = list->parent;
    widget_instance *nodes[17];
    controls_gamepad_record *source = 0;
    controls_gamepad_record *target = 0;
    int32_t index = -1;
    int32_t i;
    uint8_t moved = 0;

    halo::interface::controls_gamepad_widget_nodes_collect(nodes, screen);
    if (list == nodes[0]) {
        source = controls_assigned_gamepads;
        target = controls_available_gamepads;
        for (i = 0; i < 4; i++) {
            if (nodes[1 + i] == row) {
                index = i;
                break;
            }
        }
    } else if (list == nodes[5]) {
        source = controls_available_gamepads;
        target = controls_assigned_gamepads;
        for (i = 0; i < 8; i++) {
            if (nodes[6 + i] == row) {
                index = i;
                break;
            }
        }
    }

    if (source == controls_assigned_gamepads || source == controls_available_gamepads) {
        int32_t source_count = source == controls_assigned_gamepads ? controls_assigned_gamepad_count : controls_available_gamepad_count;

        if (index < source_count) {
            controls_gamepad_record entry = source[index];
            int32_t *target_count = target == controls_assigned_gamepads ? &controls_assigned_gamepad_count : &controls_available_gamepad_count;
            int32_t capacity = target == controls_assigned_gamepads ? 4 : 8;

            if (*target_count < capacity) {
                target[*target_count] = entry;
                (*target_count)++;
                moved = halo::interface::controls_gamepad_list_remove(&entry, source);
            }
        }
    }

    halo::interface::controls_gamepad_lists_refresh(screen);
    if (row->parent == nodes[0]) {
        if (controls_assigned_gamepad_count == 0) {
            screen->focused_child = nodes[5];
            nodes[5]->focused_child = nodes[5]->first_child;
        } else {
            controls_gamepad_focus_visible_row(nodes[0]);
        }
    } else if (controls_available_gamepad_count != 0 && controls_assigned_gamepad_count != 4) {
        controls_gamepad_focus_visible_row(nodes[5]);
    } else {
        screen->focused_child = nodes[0];
        nodes[0]->focused_child = nodes[0]->first_child;
    }
    return moved;
}

/**
 * Original engine function controls_gamepad_widget_nodes_collect; the author notes are in
 * docs/original/interface/controls_gamepad_widget_nodes_collect.txt.
 * blam-cc: out -> EAX, screen -> ECX
 *
 * @address 0x4b5560
 */
void GamepadBindings::widget_nodes_collect(widget_instance **out, widget_instance *screen)
{
    widget_instance *w;

    w = screen->first_child;
    out[0] = w;
    w = w->first_child;
    out[1] = w;
    w = w->next_sibling;
    out[2] = w;
    w = w->next_sibling;
    out[3] = w;
    out[4] = w->next_sibling;

    w = out[0]->next_sibling;
    out[5] = w;
    w = w->first_child;
    out[6] = w;
    w = w->next_sibling;
    out[7] = w;
    w = w->next_sibling;
    out[8] = w;
    w = w->next_sibling;
    out[9] = w;
    w = w->next_sibling;
    out[10] = w;
    w = w->next_sibling;
    out[11] = w;
    w = w->next_sibling;
    out[12] = w;
    out[13] = w->next_sibling;

    w = out[5]->next_sibling;
    out[14] = w;
    w = w->next_sibling;
    out[15] = w;
    out[16] = w->next_sibling;
}

}
