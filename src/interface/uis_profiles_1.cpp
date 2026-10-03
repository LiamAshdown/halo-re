/**
 * Profile list and profile selection behaviour of the UI.
 */

#include "crt.h"
#include "halo/core/datum.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include "saved_games.h"
#include <wchar.h>

#include "halo/interface/uis_profiles.hpp"
#include "halo/memory/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern int32_t profile_slot_lookup_cache_00692ac8;
extern profile_carousel_slot profile_carousel_slots[3];
extern growable_array ui_lists[3];
extern char last_profile_name[];
extern int32_t cached_profile_slot;
extern int32_t ui_list_current;
extern uint8_t ui_list_has_default;
extern uint8_t default_profile_data[k_saved_player_profile_size];
extern heap *widget_memory_pool;
extern int16_t new_profile_name_entry_player_00692b00;
extern virtual_keyboard_globals virtual_keyboard;
extern uint16_t new_profile_name_buffer_006b37f4[0xb];
extern int16_t profile_slot_id[];
extern uint8_t new_profile_name_terminator_006b380a;
extern uint8_t new_profile_name_flag_0071916e;
extern uint8_t network_wait_flag_00719739;
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern uint8_t quit_confirm_error_modal;
extern uint8_t quit_confirm_error_is_error;
extern uint16_t split_screen_quit_prompt_string;
extern uint8_t split_screen_quit_prompt_armed;
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id);
extern void saved_item_select(int32_t selection_id);
extern void widget_play_sound_effect(int16_t effect_id);
extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles];
}

namespace halo::ui {

/**
 * Resets the shared UI list arrays, allocates a 100-slot profile-id buffer, enumerates saved profiles into it,
 * and adds one list entry per occupied slot (skipping empty ones), while tracking which list index corresponds
 * to the currently loaded profile.
 *
 * @address 0x49dd70
 */
uint32_t UiProfiles::build_profile_list(widget_instance *widget)
{
    int32_t *slot_ids;
    uint32_t high_bits = 0;

    profile_slot_lookup_cache_00692ac8 = -1;
    memset(profile_carousel_slots, 0xff, sizeof(profile_carousel_slots));

    slot_ids = (int32_t *)halo::memory::heap_reallocate(widget->list_items, 400, widget_memory_pool);
    widget->list_items = slot_ids;
    if (slot_ids != nullptr) {
        uint8_t profile_buffer[k_saved_player_profile_size];
        int32_t matched_profile;
        int32_t i;

        {
            uint32_t count = 0x64;
            halo::saved_games::saved_game_enumerate_by_type(0, (int32_t *)slot_ids, 0, (uint16_t *)&count);
        }
        widget->item_count = 100;

        ui_lists[0].element_size = 0x10;
        ui_lists[1].element_size = 0x10;
        ui_lists[2].element_size = 0x10;
        ui_lists[0].count = 0;
        ui_lists[1].count = 0;
        ui_lists[2].count = 0;
        ui_lists[0].data = nullptr;
        ui_lists[1].data = nullptr;
        ui_lists[2].data = nullptr;
        ui_list_current = -1;
        ui_list_has_default = 0;

        if (last_profile_name[0] == '\0' && halo::saved_games::saved_game_last_profile_read((uint8_t *)last_profile_name) != 0) {
            cached_profile_slot = halo::saved_games::saved_game_find_by_name(last_profile_name, 0);
        }

        matched_profile = cached_profile_slot;
        widget->selection_index = -1;

        for (i = 0; i < (int16_t)widget->item_count; i++) {
            int32_t slot_id = slot_ids[i];

            if (matched_profile != -1 && slot_id == matched_profile) {
                widget->selection_index = (int16_t)i;
            }
            if (slot_id == -1) {
                memcpy(profile_buffer, default_profile_data, sizeof(profile_buffer));
            } else if (halo::saved_games::player_profile_get(slot_id, (saved_player_profile *)profile_buffer) != 0) {
                halo::interface::ui_list_add_entry(1, (const uint16_t *)(profile_buffer + 2), i, profile_buffer, k_saved_player_profile_size, 0);
            }
        }

        high_bits = 0xffffff;
        if (widget->selection_index == -1) {
            widget->selection_index = 0;
        }
        *(int16_t *)&((struct widget_instance *)widget)->text = widget->selection_index;
        *(int16_t *)((uint8_t *)widget + 0x3e) = -1;
    }
    return (high_bits << 8) | 1;
}

/**
 * Frees the widget's allocated profile-slot-id array (if any), clears its item count, and frees the three shared
 * UI list groups.
 *
 * @address 0x49df70
 */
uint32_t UiProfiles::free_profile_list(widget_instance *widget)
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
    widget->item_count = 0;
    halo::interface::ui_list_free_all();
    return 1;
}

/**
 * Original UI routine; see docs/original/interface/ui_new_profile_name_entry_commit.c.txt for the recovery
 * notes.
 *
 * @address 0x4a19a0
 */
uint32_t UiProfiles::new_profile_name_entry_commit(void)
{
    int32_t profile_id;

    if (new_profile_name_entry_player_00692b00 == -1) {
        return 0;
    }
    if (virtual_keyboard.committed == 0) {
        new_profile_name_entry_player_00692b00 = -1;
        return 0;
    }

    if (new_profile_name_buffer_006b37f4[0] != 0) {
        uint16_t default_name[4220];

        profile_slot_id[0] = new_profile_name_entry_player_00692b00;
        profile_id = halo::saved_games::saved_game_create_default_profile((uint16_t *)new_profile_name_buffer_006b37f4);
        if (profile_id == -1) {
            halo::saved_games::saved_game_allocate_new_slot(default_name);
            wcsncpy((wchar_t *)new_profile_name_buffer_006b37f4, (const wchar_t *)default_name, 0xb);
            new_profile_name_terminator_006b380a = 0;
            profile_id = halo::saved_games::saved_game_create_default_profile((uint16_t *)new_profile_name_buffer_006b37f4);
            if (profile_id == -1) {
                goto fail;
            }
        }
        if (halo::saved_games::player_profile_get_or_cached_default((saved_player_profile *)default_profile_data, (int32_t)profile_id) != 0) {
            halo::interface::player_profile_load((int16_t)profile_id, nullptr, profile_id);
            if (new_profile_name_flag_0071916e != 0) {
                halo::interface::saved_item_select(-1);
            }
            halo::main::main_queue_map_change((char *)"");
            new_profile_name_entry_player_00692b00 = -1;
            network_wait_flag_00719739 = 0;
            return 1;
        }
    }

fail:
    split_screen_quit_prompt_string = halo::k_word_none;
    halo::networking::globals().join_error_reason = 0;
    split_screen_quit_prompt_armed = 1;
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x25;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
    }
    halo::interface::widget_play_sound_effect(0);
    new_profile_name_entry_player_00692b00 = -1;
    return 0;
}

/**
 * Copies up to 11 wide characters of the carousel-slot profile's display name (starting 2 bytes into the record,
 * as in the sibling profile helpers) into a freshly allocated 24 byte widget text buffer.
 *
 * @address 0x4a69f0
 */
void UiProfiles::profile_carousel_fetch_name(widget_instance *widget)
{
    uint8_t profile_record[k_saved_player_profile_size];
    uint16_t *dest;

    memcpy(profile_record, &profile_globals_block[widget->controller_index].profile, sizeof(profile_record));

    dest = (uint16_t *)halo::memory::heap_reallocate(widget->text, 0x18, widget_memory_pool);
    widget->text = dest;
    if (dest != 0) {
        wcsncpy((wchar_t *)dest, (const wchar_t *)(profile_record + 2), 0x0b);
        dest[0x0b] = 0;
    }
}

/**
 * Reads a raw int16 value out of the carousel-slot profile record and clamps it into 0..0x11 before storing it
 * in the widget's live-value field (background_bitmap_frame, offset 0x58).
 *
 * @address 0x4a6b00
 */
void UiProfiles::profile_carousel_fetch_sensitivity(widget_instance *widget)
{
    uint8_t profile_record[k_saved_player_profile_size];
    int16_t raw_value;

    memcpy(profile_record, &profile_globals_block[widget->controller_index].profile, sizeof(profile_record));
    raw_value = *(int16_t *)(profile_record + 0x11a);

    if (raw_value < 0) {
        widget->background_bitmap_frame = 0;
        return;
    }
    if (raw_value > 0x11) {
        widget->background_bitmap_frame = 0x11;
        return;
    }
    widget->background_bitmap_frame = raw_value;
}

/**
 * Original UI routine; see docs/original/interface/ui_profile_carousel_slot_cache_populate.c.txt for the
 * recovery notes.
 *
 * Register convention: candidate_ids -> EBX
 *
 * @address 0x4a74b0
 */
void UiProfiles::profile_carousel_slot_cache_populate(int32_t count, const int32_t *candidate_ids)
{
    uint8_t slot_kept[3] = { 0, 0, 0 };
    int32_t slot;
    int32_t i;

    for (slot = 0; slot < 3; slot++) {
        if (profile_carousel_slots[slot].profile_id != -1) {
            for (i = 0; i < count; i++) {
                if (profile_carousel_slots[slot].profile_id == candidate_ids[i]) {
                    slot_kept[slot] = 1;
                    break;
                }
            }
        }
    }

    for (i = 0; i < count; i++) {
        int32_t id = candidate_ids[i];
        int32_t found;
        int32_t free_slot;

        if (id == -1) {
            continue;
        }
        for (found = 0; found < 3 && profile_carousel_slots[found].profile_id != id; found++) {
        }
        if (found != 3) {
            continue;
        }
        for (free_slot = 0; free_slot < 3 && slot_kept[free_slot] == 1; free_slot++) {
        }
        if (halo::saved_games::player_profile_get(id, (saved_player_profile *)profile_carousel_slots[free_slot].profile)) {
            profile_carousel_slots[free_slot].profile_id = candidate_ids[i];
            slot_kept[free_slot] = 1;
        }
    }
}

/**
 * Rebuilds this widget's list rows, refreshes the profile-name label, clears the description value's highlight,
 * then hands off to ui_level_carousel_row_refresh to lay out the three profile-detail rows.
 *
 * @address 0x4a8360
 */
void UiProfiles::profile_details_list_widget_build(widget_instance *widget)
{
    uint8_t profile_record[k_saved_player_profile_size];

    halo::interface::ui_list_widget_rebuild_rows(widget, (ui_list_item_format_function)((void *)halo::interface::ui_list_default_item_format));

    memcpy(profile_record, &profile_globals_block[0].profile, sizeof(profile_record));
    halo::interface::set_profile_name(widget->extended_description->first_child, (const uint16_t *)(profile_record + 2));

    widget->extended_description->first_child->next_sibling->background_bitmap_frame = 0;
    halo::interface::ui_level_carousel_row_refresh(widget->extended_description->first_child->next_sibling,
                 *(int16_t *)&((struct widget_instance *)widget)->text);
}

/**
 * Original UI routine; see docs/original/interface/ui_profile_list_apply_selection.c.txt for the recovery notes.
 *
 * Register convention: matches ui_event_function (widget, event, out_handled)
 *
 * @address 0x49dfc0
 */
uint8_t UiProfiles::profile_list_apply_selection(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *list_widget = widget->parent;
    int32_t *slot_ids = (int32_t *)list_widget->list_items;
    int32_t entry_id = slot_ids[list_widget->selection_index];
    uint8_t profile_data[k_saved_player_profile_size];

    (void)event;

    if (entry_id == -1) {
        halo::interface::widget_play_sound_effect(0);
        return 0;
    }

    if (entry_id > -1) {
        if (quit_confirm_error_string_index == -1) {
            quit_confirm_error_string_index = 0x1f;
            quit_confirm_error_unknown_ae = halo::k_word_none;
            quit_confirm_error_modal = 1;
            quit_confirm_error_is_error = 0;
        }
        halo::interface::widget_play_sound_effect(0);
        *out_handled = 1;
        return 0;
    }

    if (halo::saved_games::player_profile_get(entry_id, (saved_player_profile *)profile_data) != 0) {
        int32_t player_index = halo::interface::player_profile_find_index_by_id((int16_t)entry_id);

        halo::interface::player_profile_load((int16_t)player_index, profile_data, entry_id);
        return 1;
    }
    return 0;
}

/**
 * Finds the first spinner_list child of `widget`, validates the profile entry at its current selection, and
 * either loads it or (if unpopulated) arms the per-player-slot help prompt.
 *
 * @address 0x49e090
 */
uint32_t UiProfiles::profile_list_apply_selection_for_player(widget_instance *widget, int16_t *context)
{
    int16_t requested_player = context[1];
    widget_instance *list_widget;
    int32_t *slot_ids;
    int32_t entry_id;
    int32_t player_slot;
    uint8_t profile_data[k_saved_player_profile_size];

    for (list_widget = widget->first_child;
         list_widget != (widget_instance *)0 && list_widget->widget_type != uiwidgettype_spinner_list;
         list_widget = list_widget->next_sibling) {
    }

    slot_ids = (int32_t *)list_widget->list_items;
    entry_id = slot_ids[list_widget->selection_index];

    if (entry_id < 0) {
        if (entry_id != -1 && halo::saved_games::player_profile_get(entry_id, (saved_player_profile *)profile_data) != 0) {
            halo::interface::player_profile_load((int16_t)entry_id, profile_data, entry_id);
            return 1;
        }
        return 0;
    }

    player_slot = (requested_player == -1) ? 0 : (int32_t)requested_player;
    if ((&quit_confirm_error_string_index)[player_slot * 3] == -1) {
        (&quit_confirm_error_string_index)[player_slot * 3] = 0x1f;
        (&quit_confirm_error_unknown_ae)[player_slot * 3] = requested_player;
        (&quit_confirm_error_modal)[player_slot * 6] = 1;
        (&quit_confirm_error_is_error)[player_slot * 6] = 0;
    }
    halo::interface::widget_play_sound_effect(0);
    return 0;
}

/**
 * Original UI routine; see docs/original/interface/ui_profile_require_existing.c.txt for the recovery notes.
 *
 * @address 0x4a1c30
 */
uint8_t UiProfiles::profile_require_existing(void *widget, int16_t *event, uint8_t *out_handled)
{
    int16_t count = 1;
    int32_t slot;

    halo::saved_games::saved_game_enumerate_by_type(0, &slot, 0, (uint16_t *)&count);
    if (count > 0) {
        return 1;
    }
    halo::interface::ui_new_profile_name_entry_open(widget, event, out_handled);
    return 0;
}

}
