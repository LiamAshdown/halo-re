/**
 * Controls options menu: binding rows, sensitivity rows and the profile reload.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#include "objects.h"
#include "units.h"

#include "halo/interface/uis_controls_menu.hpp"
#include "halo/memory/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "saved_games.h"

extern "C" {
extern uint8_t profile_globals_block[0x60a4];
extern heap *widget_memory_pool;
extern int32_t selected_saved_item;
extern uint8_t saved_item_working_copy[k_saved_player_profile_size];
extern int32_t profile_slot_lookup_cache_00692ac8;
extern uint8_t directsound_initialized;
extern uint8_t directsound_eax_available;
}

namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static widget_instance *find_row_control(widget_instance *row)
{
    widget_instance *control;

    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != uiwidgettype_spinner_list;
         control = control->next_sibling) {
    }
    return control;
}

/** Local helper shared by the handlers of this file. */
static widget_instance *find_row_control_until(widget_instance *row, widget_instance *stop)
{
    widget_instance *control;

    for (control = row->first_child; control != stop && control->widget_type != uiwidgettype_spinner_list;
         control = control->next_sibling) {
    }
    return control;
}

}

/**
 * Original UI routine; see docs/original/interface/ui_controls_4wide_selector_refresh.c.txt for the recovery
 * notes.
 *
 * @address 0x4a4f60
 */
void UiControlsMenu::controls_4wide_selector_refresh(widget_instance *widget)
{
    int16_t selection = *(int16_t *)&((struct widget_instance *)widget)->text;
    widget_instance *display = widget->extended_description->first_child;
    widget_instance *cursor;
    int16_t i;

    if (selection != -1) {
        display->background_bitmap_frame = selection;
        display->next_sibling->selection_index = selection;
    }

    {
        uint8_t profile_copy[0x2000];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
        halo::interface::set_profile_name(widget, (const uint16_t *)(profile_copy + 2));
    }

    cursor = widget->first_child;
    for (i = 0; i < 4; i++) {
        cursor->background_bitmap_frame = (i == selection);
        cursor = cursor->next_sibling;
    }
}

/**
 * Original UI routine; see docs/original/interface/ui_controls_options_free_list.c.txt for the recovery notes.
 *
 * @address 0x4a0a30
 */
uint32_t UiControlsMenu::controls_options_free_list(widget_instance *widget)
{
    if (widget->list_items != nullptr) {
        heap_block *block = (heap_block *)((uint8_t *)widget->list_items - 0x10);
        uint32_t size = block->size;

        halo::memory::heap_unlink_block(block, widget_memory_pool);
        widget_memory_pool->bytes_allocated =
            widget_memory_pool->bytes_allocated - (int32_t)(size & 0x7fffffff);
        widget_memory_pool->allocation_count = widget_memory_pool->allocation_count - 1;
        widget->list_items = nullptr;
    }
    halo::interface::ui_list_free_all();
    return 1;
}

/**
 * Original UI routine; see docs/original/interface/ui_controls_options_populate_from_profile.c.txt for the
 * recovery notes.
 *
 * @address 0x4a0050
 */
uint32_t UiControlsMenu::controls_options_populate_from_profile(widget_instance *widget)
{
    uint8_t *record;
    widget_instance *row;
    widget_instance *control;
    int32_t field;

    if ((selected_saved_item & 0xf) != 1) {
        return 0;
    }
    record = saved_item_working_copy;

    row = widget->first_child;
    control = find_row_control(row);
    field = *(int32_t *)(record + 0x80);
    control->selection_index = (field == 1 || field == 2) ? (int16_t)field : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    field = *(int32_t *)(record + 0x7c);
    control->selection_index = (field == 1 || field == 2) ? (int16_t)field : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    switch (*(int32_t *)(record + 0x58)) {
    case 3: control->selection_index = 1; break;
    case 5: control->selection_index = 2; break;
    case 10: control->selection_index = 3; break;
    case 15: control->selection_index = 4; break;
    case 25: control->selection_index = 5; break;
    default: control->selection_index = 0; break;
    }

    row = row->next_sibling;
    control = find_row_control(row);
    control->selection_index = (record[0x34] == 0) ? 1 : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    field = *(int32_t *)(record + 0x78);
    switch (field) {
    case 18000: control->selection_index = 1; return 1;
    case 27000: control->selection_index = 2; return 1;
    case 36000: control->selection_index = 3; return 1;
    case 45000: control->selection_index = 4; return 1;
    case 54000: control->selection_index = 5; return 1;
    case 81000: control->selection_index = 6; return 1;
    default: control->selection_index = 0; return 1;
    }
}

/**
 * Original UI routine; see docs/original/interface/ui_controls_options_reload_profile.c.txt for the recovery
 * notes.
 *
 * @address 0x4a0b50
 */
uint8_t UiControlsMenu::controls_options_reload_profile(void)
{
    profile_slot_lookup_cache_00692ac8 = -1;
    if ((selected_saved_item & 0xf) == 0) {
        if (halo::saved_games::globals().player_profile_slots_handle != -1) {
            uint8_t profile_copy[k_saved_player_profile_size];

            memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
            halo::interface::player_profile_load(0, profile_copy, halo::saved_games::globals().player_profile_slots_handle);
        }
        selected_saved_item = -1;
    }
    return 1;
}

/**
 * Original UI routine; see docs/original/interface/ui_controls_populate_bind_rows.c.txt for the recovery notes.
 *
 * @address 0x4a3180
 */
void UiControlsMenu::controls_populate_bind_rows(widget_instance *widget, uint32_t packed)
{
    widget_instance *stop = (widget_instance *)0;
    uint8_t disabled = (packed & 0xf) > 7;
    uint16_t fallback = 0;
    uint8_t enable_state = 0;
    widget_instance *row = widget->first_child->next_sibling->next_sibling->next_sibling;
    int shift;

    if (!disabled) {
        packed = halo::game::game_variant_option_default_by_index(packed & 0xf);
    }

    for (shift = 4; shift <= 19; shift += 3) {
        widget_instance *control = find_row_control_until(row, stop);
        uint8_t field = (uint8_t)(packed >> shift) & 7;

        control->selection_index = (field < 5) ? field : fallback;
        if (disabled == enable_state) {
            row->hidden = 1;
            row->scale = 0.333f;
        } else {
            row->hidden = enable_state;
            row->scale = 1.0f;
        }
        row = row->next_sibling;
    }
}

/**
 * Original UI routine; see docs/original/interface/ui_controls_populate_input_row.c.txt for the recovery notes.
 *
 * Register convention: EAX -> widget, EDI -> profile_record
 *
 * @address 0x4a22e0
 */
void UiControlsMenu::controls_populate_input_row(widget_instance *widget, const uint8_t *profile_record)
{
    widget_instance *row = widget->first_child;
    widget_instance *control;
    uint8_t value;

    control = find_row_control(row);
    value = profile_record[0xb78];
    control->selection_index = (value < 0xb) ? value : 10;

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record[0xb79];
    control->selection_index = (value < 0xb) ? value : 10;

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record[0xb7a];
    control->selection_index = (value < 0xb) ? value : 10;

    row = row->next_sibling;
    control = find_row_control(row);
    if (directsound_initialized == 0 || directsound_eax_available == 0) {
        row->hidden = 1;
        row->scale = 0.333f;
        control->selection_index = 0;
    } else {
        control->selection_index = (profile_record[0xb7c] != 0) ? 1 : 0;
        row->hidden = 0;
        row->scale = 1.0f;
    }

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record[0xb7d];
    control->selection_index = (value < 3) ? value : 2;

    row = row->next_sibling;
    control = find_row_control(row);
    control->selection_index =
        (profile_record[0xb7b] != 0 && directsound_initialized != 0 && directsound_eax_available != 0)
            ? 1 : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record[0xb7f];
    control->selection_index = (value > 2) ? 2 : value;
}

/**
 * Finds the first spinner_list among each of widget's first three child "rows" and sets each one's
 * selection_index from a byte in the raw saved-profile record: rows 0/1 from a 1..10 sensitivity index (clamped
 * to 0 when out of range, else index-1), row 2 from a plain boolean.
 *
 * Register convention: EAX -> widget, ESI -> profile_record
 *
 * @address 0x4a20f0
 */
void UiControlsMenu::controls_populate_sensitivity_row(widget_instance *widget, const uint8_t *profile_record)
{
    widget_instance *row = widget->first_child;
    widget_instance *control;
    uint8_t value;

    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != uiwidgettype_spinner_list;
         control = control->next_sibling) {
    }
    value = profile_record[0x954];
    control->selection_index = (value == 0 || value > 10) ? 0 : (int16_t)(value - 1);

    row = row->next_sibling;
    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != uiwidgettype_spinner_list;
         control = control->next_sibling) {
    }
    value = profile_record[0x955];
    control->selection_index = (value == 0 || value > 10) ? 0 : (int16_t)(value - 1);

    row = row->next_sibling;
    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != uiwidgettype_spinner_list;
         control = control->next_sibling) {
    }
    control->selection_index = (profile_record[0x12f] != 0) ? 1 : 0;
}

/**
 * Original UI routine; see docs/original/interface/ui_controls_sensitivity_row_refresh.c.txt for the recovery
 * notes.
 *
 * @address 0x4a2270
 */
uint32_t UiControlsMenu::controls_sensitivity_row_refresh(widget_instance *widget, const uint8_t *profile_record)
{
    halo::interface::ui_controls_populate_sensitivity_row(widget, profile_record);
    halo::interface::widget_play_sound_effect(0);
    return 1;
}

}
