/**
 * Per-frame game-data input functions run for each game_data_inputs entry of a widget
 * (game_data_input_function_table).
 */

#include "tags.h"
#include "halo/text/api.hpp"
#include "halo/interface/engine_state.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <stdlib.h>
#include <wchar.h>
#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"
#include <string.h>

#include "halo/interface/uis_game_data_inputs.hpp"
#include "halo/memory/api.hpp"
#include "halo/input/api.hpp"
#include "halo/networking/api.hpp"

extern "C" {
extern uint8_t ui_split_screen;
extern int32_t network_host_number_field_00719218;
extern uint16_t network_host_number_text_0071921c[0x10];
extern uint32_t network_game_option_a_00719210;
extern uint32_t network_game_option_b_00719214;
extern heap *widget_memory_pool;
extern void widget_extended_description_sync_selection(widget_instance *widget);
extern uint8_t *network_client;
extern void widget_instance_set_state_recursive(widget_instance *widget, uint8_t state);
extern int32_t selected_saved_item;
extern uint8_t saved_item_working_copy[0x1ffc];
extern uint8_t variant_teams_enabled_0071920c;
extern int32_t variant_team_selection_00692b08;
extern void ui_controls_populate_bind_rows(widget_instance *widget, uint32_t packed);
extern void controls_gamepad_lists_refresh(widget_instance *screen);
extern void controls_gamepad_widget_nodes_collect(widget_instance **out, widget_instance *screen);
}

namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static void set_option_text(widget_instance *row, uint32_t value, uint8_t hidden)
{
    widget_instance *label = row->first_child->next_sibling;
    uint16_t *text;

    text = (uint16_t *)halo::memory::heap_reallocate(label->text, 0x10, widget_memory_pool);
    label->text = text;
    if (text != 0) {
        halo::text::string_format_wide_va(text, (const uint16_t *)L"%d", value);
        text[7] = 0;
    }
    if (hidden) {
        row->hidden = 1;
        *(uint32_t *)&row->scale = 0x3eaa7efa;
    } else {
        row->hidden = 0;
        row->scale = 1.0f;
    }
}

/** Local helper shared by the handlers of this file. */
static void set_text(widget_instance *label, const uint16_t *source)
{
    int32_t length = (int32_t)wcslen((const wchar_t *)source);
    uint16_t *text;

    text = (uint16_t *)halo::memory::heap_reallocate(label->text, (uint16_t)(length * 2 + 2), widget_memory_pool);
    label->text = text;
    if (text != 0) {
        wcsncpy((wchar_t *)text, (const wchar_t *)source, length);
        text[length] = 0;
    }
}

/** Local helper shared by the handlers of this file. */
static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

}

/**
 * Slot 58 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a3b70
 */
void UiGameDataInputs::input_4a3b70(widget_instance *widget)
{
    uint8_t hidden = (uint8_t)(ui_split_screen == 0);
    widget_instance *row;

    if (network_host_number_field_00719218 == 1) {
        if (network_host_number_text_0071921c[0] == 0) {
            network_game_option_a_00719210 = halo::networking::globals().game_socket_port;
        } else {
            network_game_option_a_00719210 = (uint32_t)_wtoi((const wchar_t *)network_host_number_text_0071921c);
            if (network_game_option_a_00719210 > 0xffff) {
                network_game_option_a_00719210 = 0xffff;
            }
        }
    } else if (network_host_number_field_00719218 == 2) {
        if (network_host_number_text_0071921c[0] == 0) {
            network_game_option_b_00719214 = 0;
        } else {
            network_game_option_b_00719214 = (uint32_t)_wtoi((const wchar_t *)network_host_number_text_0071921c);
            if (network_game_option_b_00719214 > 0xffff) {
                network_game_option_b_00719214 = 0xffff;
            }
        }
    }
    row = widget->first_child->next_sibling;
    set_option_text(row, network_game_option_a_00719210, hidden);
    row = row->next_sibling;
    set_option_text(row, network_game_option_b_00719214, hidden);
    widget_extended_description_sync_selection(widget);
    network_host_number_field_00719218 = 0;
}

/**
 * Slot 1 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a4c70
 */
void UiGameDataInputs::input_4a4c70(widget_instance *widget)
{
    widget_instance *label = widget->extended_description->first_child;
    widget_instance *list = label->next_sibling;
    widget_instance *child = widget->first_child;
    int16_t index = 0;

    if (child != 0) {
        for (; child != 0 && child != widget->focused_child; child = child->next_sibling) {
            index++;
        }
        if ((uint16_t)index == 0xffff) {
            return;
        }
    }
    label->background_bitmap_frame = index;
    list->selection_index = index;
}

/**
 * Slot 11 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a5740
 */
void UiGameDataInputs::input_4a5740(widget_instance *widget)
{
    uint8_t *game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;
    widget_instance *first;
    widget_instance *second;
    widget_instance *countdown;
    widget_instance *status;
    widget_instance *frame;
    widget_instance *name;
    widget_instance *team;
    int16_t key;
    int32_t found = -1;
    int32_t i;
    uint16_t *text;

    if (game == 0) {
        return;
    }
    key = *(int16_t *)network_client;
    first = widget->first_child;
    second = first->next_sibling;
    countdown = second->next_sibling;
    text = (uint16_t *)halo::memory::heap_reallocate(countdown->text, 0x20, widget_memory_pool);
    countdown->text = text;
    if (text != 0) {
        int16_t seconds = *(int16_t *)(network_client + 0xed8);

        wcsncpy((wchar_t *)text, L"-:--", 0xf);
        second->state = 1;
        second->selection_index = 0;
        countdown->state = 1;
        if (seconds == 0) {
            second->selection_index = 1;
            countdown->state = 0;
        } else if (seconds > 0) {
            if (seconds < 60) {
                halo::text::string_format_wide_va_bounded(0xf, text, (const uint16_t *)L"0:%02d", (int32_t)seconds);
            } else if (seconds < 3600) {
                halo::text::string_format_wide_va_bounded(0xf, text, (const uint16_t *)L"%02d:%02d", seconds / 60, seconds % 60);
            } else {
                int32_t hours = seconds / 3600;
                int32_t minutes = (seconds - hours * 3600) / 60;

                halo::text::string_format_wide_va_bounded(0xf, text, (const uint16_t *)L"%d:%02d:%02d", hours, minutes,
                    seconds - (hours * 60 + minutes) * 60);
            }
        } else if (*(int16_t *)(game + 0x1a0) < 2 || game[0x138] == 1) {
            second->state = 0;
            countdown->state = 0;
        }
        text[0xf] = 0;
    }
    status = first->first_child;
    frame = status->next_sibling;
    name = frame->next_sibling;
    for (i = 0; i < 0x10; i++) {
        uint8_t *entry = game + 0x1a2 + i * 0x20;

        if (halo::networking::network_player_entry_validate((network_player_entry *)entry) != 0 && (int16_t)(int8_t)entry[0x1c] == key) {
            if ((int8_t)entry[0x1d] == 0) {
                found = i;
            }
            break;
        }
    }
    set_text(status, (const uint16_t *)L"");
    frame->background_bitmap_frame = 0;
    frame = name->first_child;
    team = frame->next_sibling->next_sibling;
    if (game[0x138] == 0) {
        widget_instance *child;

        team->state = 0;
        for (child = team->first_child; child != 0; child = child->next_sibling) {
            widget_instance_set_state_recursive(child, 0);
        }
    }
    if (found == -1) {
        frame->background_bitmap_frame = 0;
        text = (uint16_t *)halo::memory::heap_reallocate(frame->next_sibling->text, 2, widget_memory_pool);
        frame->next_sibling->text = text;
        if (text != 0) {
            text[0] = 0;
        }
        team->selection_index = 0;
        return;
    }
    set_text(frame->next_sibling, (const uint16_t *)(game + 0x1a2 + found * 0x20));
    if (game[0x138] == 0) {
        frame->background_bitmap_frame = 1;
        return;
    }
    switch ((int8_t)game[0x1a2 + found * 0x20 + 0x1e]) {
    case 0:
        frame->background_bitmap_frame = 5;
        team->selection_index = 0;
        break;
    case 1:
        frame->background_bitmap_frame = 4;
        team->selection_index = 1;
        break;
    default:
        frame->background_bitmap_frame = 3;
        team->selection_index = 0;
        break;
    }
}

/**
 * Slot 20 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a6880
 */
void UiGameDataInputs::input_4a6880(widget_instance *widget)
{
    widget_instance *focused = widget->focused_child;
    widget_instance *label = widget->extended_description;
    widget_instance *child;
    int16_t sum = 0;

    if (focused == 0) {
        label->background_bitmap_frame = (int16_t)(uint32_t)widget;
        return;
    }
    for (child = widget->first_child; child != 0; child = child->next_sibling) {
        widget_instance *list = first_list_child(child);

        if (child == focused) {
            sum = (int16_t)(sum + list->selection_index);
            break;
        }
        sum = (int16_t)(sum + list->item_count);
    }
    label->background_bitmap_frame = sum;
}

/**
 * Slot 24 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a6a60
 */
void UiGameDataInputs::input_4a6a60(widget_instance *widget)
{
    uint16_t *text;

    if ((selected_saved_item & 0xf) != 0) {
        return;
    }
    text = (uint16_t *)halo::memory::heap_reallocate(widget->text, 0x18, widget_memory_pool);
    widget->text = text;
    if (text != 0) {
        wcsncpy((wchar_t *)text, (const wchar_t *)(saved_item_working_copy + 2), 0xb);
        text[0xb] = 0;
    }
}

/**
 * Slot 25 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a6ab0
 */
void UiGameDataInputs::input_4a6ab0(widget_instance *widget)
{
    uint16_t *text;

    if ((selected_saved_item & 0xf) != 1) {
        return;
    }
    text = (uint16_t *)halo::memory::heap_reallocate(widget->text, 0x30, widget_memory_pool);
    widget->text = text;
    if (text != 0) {
        wcsncpy((wchar_t *)text, (const wchar_t *)saved_item_working_copy, 0x17);
        text[0x17] = 0;
    }
}

/**
 * Slot 27 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a6b70
 */
void UiGameDataInputs::input_4a6b70(widget_instance *widget)
{
    uint8_t *game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;
    static const char *const maps[] = {
        "beavercreek", "sidewinder", "damnation", "ratrace", "prisoner",
        "hangemhigh", "chillout", "carousel", "boardingaction", "bloodgulch",
        "wizard", "putput", "longest"
    };
    int16_t i;

    if (game == 0) {
        return;
    }
    for (i = 0; i < (int16_t)(sizeof(maps) / sizeof(maps[0])); i++) {
        if (strstr((char *)(game + 0x84), maps[i]) != 0) {
            widget->selection_index = i;
            return;
        }
    }
    widget->selection_index = (int16_t)(strstr((char *)(game + 0x84), "icefields") != 0 ? 0xd : 0x13);
}

/**
 * Slot 28 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a6d50
 */
void UiGameDataInputs::input_4a6d50(widget_instance *widget)
{
    uint8_t *game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;

    if (game == 0) {
        return;
    }
    switch (*(int32_t *)(game + 0x134)) {
    case 1:
        if (game[0x180] == 1) {
            widget->selection_index = (int16_t)(*(int32_t *)(game + 0x184) != 0 ? 0x1c : 0x1d);
        } else {
            widget->selection_index = (int16_t)(*(int32_t *)(game + 0x184) != 0 ? 0x1e : 3);
        }
        break;
    case 2:
        widget->selection_index = 4;
        break;
    case 3:
        switch (*(int32_t *)(game + 0x190)) {
        case 1:
            widget->selection_index = 0x1f;
            break;
        case 2:
            widget->selection_index = 0x20;
            break;
        default:
            widget->selection_index = 5;
            break;
        }
        break;
    case 4:
        widget->selection_index = 6;
        break;
    case 5:
        widget->selection_index = (int16_t)(*(int32_t *)(game + 0x180) == 2 ? 0x21 : 7);
        break;
    default:
        widget->selection_index = 8;
        break;
    }
}

/**
 * Slot 29 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a6e50
 */
void UiGameDataInputs::input_4a6e50(widget_instance *widget)
{
    uint8_t *game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;

    if (game != 0) {
        widget->selection_index = (int16_t)((game[0x138] != 1) + 0xc);
    }
}

/**
 * Slot 30 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a6e90
 */
void UiGameDataInputs::input_4a6e90(widget_instance *widget)
{
    uint8_t *game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;
    uint16_t *text;

    if (game == 0) {
        return;
    }
    text = (uint16_t *)halo::memory::heap_reallocate(widget->text, 0x10, widget_memory_pool);
    widget->text = text;
    if (text != 0) {
        halo::text::string_format_wide_va_bounded(7, text, (const uint16_t *)L"%d", *(int32_t *)(game + 0x15c));
        text[7] = 0;
    }
}

/**
 * Slot 31 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a6f00
 */
void UiGameDataInputs::input_4a6f00(widget_instance *widget)
{
    uint8_t *game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;

    if (game == 0) {
        return;
    }
    switch (*(int32_t *)(game + 0x134)) {
    case 1:
        widget->selection_index = 0x16;
        break;
    case 3:
        widget->selection_index = (int16_t)(0x17 + (*(int32_t *)(game + 0x190) == 2));
        break;
    case 4:
        widget->selection_index = 0x17;
        break;
    case 5:
        widget->selection_index = 0x19;
        break;
    default:
        widget->selection_index = 0x18;
        break;
    }
}

/**
 * Slot 32 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a6fa0
 */
void UiGameDataInputs::input_4a6fa0(widget_instance *widget)
{
    uint8_t *game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;
    static const char *const maps[] = {
        "beavercreek", "sidewinder", "damnation", "ratrace", "prisoner",
        "hangemhigh", "chillout", "carousel", "boardingaction", "bloodgulch",
        "wizard", "putput", "longest"
    };
    int16_t i;

    if (game == 0) {
        return;
    }
    for (i = 0; i < (int16_t)(sizeof(maps) / sizeof(maps[0])); i++) {
        if (strstr((char *)(game + 0x84), maps[i]) != 0) {
            widget->background_bitmap_frame = i;
            return;
        }
    }
    widget->background_bitmap_frame = (int16_t)(strstr((char *)(game + 0x84), "icefields") != 0 ? 0xd : 0x13);
}

/**
 * Slot 33 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a7180
 */
void UiGameDataInputs::input_4a7180(widget_instance *widget)
{
    uint8_t *game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;

    if (game == 0) {
        return;
    }
    switch (*(int32_t *)(game + 0x134)) {
    case 1:
        widget->background_bitmap_frame = 0;
        break;
    case 2:
        widget->background_bitmap_frame = 2;
        break;
    case 3:
        widget->background_bitmap_frame = 3;
        break;
    case 4:
        widget->background_bitmap_frame = 1;
        break;
    case 5:
        widget->background_bitmap_frame = 4;
        break;
    default:
        widget->background_bitmap_frame = 5;
        break;
    }
}

/**
 * Slot 34 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a7210
 */
void UiGameDataInputs::input_4a7210(widget_instance *widget)
{
    uint8_t *game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;
    uint16_t *text;

    if (game == 0) {
        return;
    }
    text = (uint16_t *)halo::memory::heap_reallocate(widget->text, 8, widget_memory_pool);
    widget->text = text;
    if (text != 0) {
        halo::text::string_format_wide_va_bounded(3, text, (const uint16_t *)L"%d", (int32_t)*(int16_t *)(game + 0x1a0));
        text[3] = 0;
    }
}

/**
 * Slot 35 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a7280
 */
void UiGameDataInputs::input_4a7280(widget_instance *widget)
{
    int32_t type;

    if ((selected_saved_item & 0xf) != 1) {
        return;
    }
    type = *(int32_t *)(saved_item_working_copy + 0x30);
    widget->selection_index = (int16_t)(type >= 1 && type <= 5 ? type + 2 : 8);
}

/**
 * Slot 38 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a7300
 */
void UiGameDataInputs::input_4a7300(widget_instance *widget)
{
    uint8_t *game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;

    if (game != 0) {
        widget->background_bitmap_frame = (int16_t)(game[0x138] != 1);
    }
}

/**
 * Slot 39 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a7340
 */
void UiGameDataInputs::input_4a7340(widget_instance *widget)
{
    widget->first_child->next_sibling->state = 0;
}

/**
 * Slot 40 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a7350
 */
void UiGameDataInputs::input_4a7350(widget_instance *widget)
{
    widget->scale = 1.0f;
}

/**
 * Slot 42 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a73d0
 */
void UiGameDataInputs::input_4a73d0(widget_instance *widget)
{
    int32_t count = halo::input::globals().joystick_slot_devices[0] != -1 ? 1 : 0;

    if (count >= 2) {
        widget->scale = 1.0f;
        return;
    }
    *(uint32_t *)&widget->scale = 0x3eaa7efa;
}

/**
 * Original UI routine; see docs/original/interface/ui_game_data_input_4a7660.c.txt for the recovery notes.
 *
 * @address 0x4a7660
 */
void UiGameDataInputs::input_4a7660(widget_instance *widget)
{
    widget_instance *child;

    if (widget->parent->focused_child != widget) {
        child = widget->first_child;
        widget->focused_child = 0;
        for (; child != 0; child = child->next_sibling) {
            child->background_bitmap_frame = 0;
        }
        return;
    }
    if (widget->focused_child != 0) {
        return;
    }
    child = widget->first_child;
    widget->focused_child = child;
    for (; child != 0; child = child->next_sibling) {
        if (child == widget->focused_child) {
            if (child->background_bitmap_frames == 2) {
                child->background_bitmap_frame = 1;
            }
        } else if (child->background_bitmap_frames == 2) {
            child->background_bitmap_frame = 0;
        }
    }
}

/**
 * Slot 50 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4a7880
 */
void UiGameDataInputs::input_4a7880(widget_instance *widget)
{
    static const uint32_t delays[] = {0, 0x384, 0x708, 0xa8c, 0xe10, 0x1518, 0x2328};
    widget_instance *group = widget->first_child;
    int16_t selection = first_list_child(group)->selection_index;
    uint8_t changed = 0;
    uint32_t *packed;
    int32_t which;

    halo::interface::state::vehicle_options_respawn_time = selection >= 0 && selection <= 6 ? delays[selection] : 0;
    group = group->next_sibling;
    which = halo::interface::state::vehicle_options_team_page;
    if (first_list_child(group)->selection_index != which) {
        which = first_list_child(group)->selection_index;
        halo::interface::state::vehicle_options_team_page = which;
        changed = 1;
    }
    packed = which == 1 ? &halo::interface::state::vehicle_options_blue_set : &halo::interface::state::vehicle_options_red_set;
    if (changed) {
        uint32_t value = *packed;

        first_list_child(group->next_sibling)->selection_index = (int16_t)((value & 0xf) < 9 ? (value & 0xf) : 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection != variant_team_selection_00692b08 || changed) {
        *packed = (*packed & ~0xfu) | ((uint32_t)selection & 0xf);
        variant_team_selection_00692b08 = selection;
        ui_controls_populate_bind_rows(widget, *packed);
    }
    if ((*packed & 0xf) == 8) {
        int32_t shift;

        for (shift = 4; shift <= 19; shift += 3) {
            group = group->next_sibling;
            selection = first_list_child(group)->selection_index;
            *packed = (*packed & ~(7u << shift)) | (((uint32_t)(int32_t)selection << shift) & (7u << shift));
        }
    }
    widget_extended_description_sync_selection(widget);
}

/**
 * Slot 47 of game_data_input_function_table (game-data input function); only reachable through that table.
 *
 * @address 0x4b5ce0
 */
void UiGameDataInputs::input_4b5ce0(widget_instance *widget)
{
    widget_instance *nodes[17];
    widget_instance *focused;

    controls_gamepad_lists_refresh(widget);
    controls_gamepad_widget_nodes_collect(nodes, widget);
    nodes[15]->background_bitmap_frame = (int16_t)(widget->focused_child == nodes[15]);
    nodes[14]->background_bitmap_frame = (int16_t)(widget->focused_child == nodes[14]);
    focused = widget->focused_child;
    if (focused == nodes[0]) {
        nodes[16]->selection_index = 0;
        nodes[16]->state = 1;
    } else if (focused == nodes[5]) {
        nodes[16]->selection_index = 1;
        nodes[16]->state = 1;
    } else {
        nodes[16]->state = 0;
    }
}

}
