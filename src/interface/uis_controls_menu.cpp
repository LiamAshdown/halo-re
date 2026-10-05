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
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

static auto &profile_globals_block = halo::link::ref<saved_player_profile_slot [k_maximum_local_player_profiles]>(halo::ui::vars().profile_globals_block);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &selected_saved_item = halo::link::ref<int32_t>(halo::ui::vars().selected_saved_item);
static auto &saved_item_working_copy = halo::link::ref<game_variant>(halo::ui::vars().saved_item_working_copy);
static auto &profile_slot_lookup_cache_00692ac8 = halo::link::ref<int32_t>(halo::ui::vars().profile_slot_lookup_cache_00692ac8);
static auto &directsound_initialized = halo::link::ref<uint8_t>(halo::ui::vars().directsound_initialized);
static auto &directsound_eax_available = halo::link::ref<uint8_t>(halo::ui::vars().directsound_eax_available);
#include "halo/interface/constants.hpp"
#include "halo/interface/widget_pool.hpp"
#include "halo/interface/wide_text.hpp"

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
 *
 * @address 0x4a4f60
 */
void UiControlsMenu::controls_4wide_selector_refresh(widget_instance *widget)
{
    int16_t selection = halo::interface::widget_list_committed(widget);
    widget_instance *display = widget->extended_description->first_child;
    widget_instance *cursor;
    int16_t i;

    if (selection != -1) {
        display->background_bitmap_frame = selection;
        display->next_sibling->selection_index = selection;
    }

    {
        saved_player_profile profile_copy;

        profile_copy = profile_globals_block[0].profile;
        halo::interface::set_profile_name(display->next_sibling->next_sibling, profile_copy.name);  // EBX in 0x4a4fa9
    }

    cursor = widget->first_child;
    for (i = 0; i < 4; i++) {
        cursor->background_bitmap_frame = (i == selection);
        cursor = cursor->next_sibling;
    }
}

/**
 *
 * @address 0x4a0a30
 */
uint32_t UiControlsMenu::controls_options_free_list(widget_instance *widget)
{
    if (widget->list_items != nullptr) {
        halo::interface::widget_pool_free(widget->list_items);
        widget->list_items = nullptr;
    }
    halo::interface::ui_list_free_all();
    return 1;
}

/**
 *
 * @address 0x4a0050
 */
uint32_t UiControlsMenu::controls_options_populate_from_profile(widget_instance *widget)
{
    const game_variant *record;
    widget_instance *row;
    widget_instance *control;
    int32_t field;

    if ((selected_saved_item & 0xf) != 1) {
        return 0;
    }
    record = &saved_item_working_copy;

    row = widget->first_child;
    control = find_row_control(row);
    field = record->engine.race.team_scoring;
    control->selection_index = (field == 1 || field == 2) ? (int16_t)field : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    field = record->engine.race.race_type;
    control->selection_index = (field == 1 || field == 2) ? (int16_t)field : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    switch (record->score_limit) {
    case 3: control->selection_index = 1; break;
    case 5: control->selection_index = 2; break;
    case 10: control->selection_index = 3; break;
    case 15: control->selection_index = 4; break;
    case 25: control->selection_index = 5; break;
    default: control->selection_index = 0; break;
    }

    row = row->next_sibling;
    control = find_row_control(row);
    control->selection_index = (record->teams == 0) ? 1 : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    field = record->time_limit;
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
 *
 * @address 0x4a0b50
 */
uint8_t UiControlsMenu::controls_options_reload_profile(void)
{
    profile_slot_lookup_cache_00692ac8 = -1;
    if ((selected_saved_item & 0xf) == 0) {
        if (halo::saved_games::globals().player_profile_slots_handle != -1) {
            saved_player_profile profile_copy;

            profile_copy = profile_globals_block[0].profile;
            halo::interface::player_profile_load(0, &profile_copy, halo::saved_games::globals().player_profile_slots_handle);
        }
        selected_saved_item = -1;
    }
    return 1;
}

/**
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
 *
 * Register convention: EAX -> widget, EDI -> profile_record
 *
 * @address 0x4a22e0
 */
void UiControlsMenu::controls_populate_input_row(widget_instance *widget, const saved_player_profile *profile_record)
{
    widget_instance *row = widget->first_child;
    widget_instance *control;
    uint8_t value;

    control = find_row_control(row);
    value = profile_record->master_volume;
    control->selection_index = (value < 0xb) ? value : 10;

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record->effects_volume;
    control->selection_index = (value < 0xb) ? value : 10;

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record->music_volume;
    control->selection_index = (value < 0xb) ? value : 10;

    row = row->next_sibling;
    control = find_row_control(row);
    if (directsound_initialized == 0 || directsound_eax_available == 0) {
        row->hidden = 1;
        row->scale = 0.333f;
        control->selection_index = 0;
    } else {
        control->selection_index = (profile_record->eax_enabled != 0) ? 1 : 0;
        row->hidden = 0;
        row->scale = 1.0f;
    }

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record->sound_quality;
    control->selection_index = (value < 3) ? value : 2;

    row = row->next_sibling;
    control = find_row_control(row);
    control->selection_index =
        (profile_record->hardware_acceleration != 0 && directsound_initialized != 0 && directsound_eax_available != 0)
            ? 1 : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record->sound_variety;
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
void UiControlsMenu::controls_populate_sensitivity_row(widget_instance *widget, const saved_player_profile *profile_record)
{
    widget_instance *row = widget->first_child;
    widget_instance *control;
    uint8_t value;

    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != uiwidgettype_spinner_list;
         control = control->next_sibling) {
    }
    value = profile_record->mouse_look_x_sensitivity;
    control->selection_index = (value == 0 || value > 10) ? 0 : (int16_t)(value - 1);

    row = row->next_sibling;
    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != uiwidgettype_spinner_list;
         control = control->next_sibling) {
    }
    value = profile_record->mouse_look_y_sensitivity;
    control->selection_index = (value == 0 || value > 10) ? 0 : (int16_t)(value - 1);

    row = row->next_sibling;
    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != uiwidgettype_spinner_list;
         control = control->next_sibling) {
    }
    control->selection_index = (profile_record->look_inverted != 0) ? 1 : 0;
}

/**
 *
 * @address 0x4a2270
 */
uint32_t UiControlsMenu::controls_sensitivity_row_refresh(widget_instance *widget, const saved_player_profile *profile_record)
{
    halo::interface::ui_controls_populate_sensitivity_row(widget, profile_record);
    halo::interface::widget_play_sound_effect(2);
    return 1;
}

}
