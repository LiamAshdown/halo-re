/**
 * Level, map, profile and variant carousel refresh and slot cache population.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>

#include "halo/interface/uis_carousels.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles];
extern level_select_entry level_select_entries[10];
extern int8_t level_select_flags_0071916a;
extern uint8_t level_select_flags_0071916b;
extern map_list_entry *map_list;
extern variant_carousel_slot variant_carousel_slots[3];
}

namespace halo::ui {

/**
 * qsort-style comparator over two int32_t* carousel-slot ids: entries that are not -1 sort before entries that
 * are -1; two entries of the same "validity" compare equal.
 *
 * @address 0x4a7630
 */
int32_t UiCarousels::carousel_slot_compare_valid_first(const int32_t *a, const int32_t *b)
{
    if (*a == -1) {
        if (*b != -1) {
            return 1;
        }
    } else if (*b == -1) {
        return -1;
    }
    return 0;
}

/**
 * Original UI routine; see docs/original/interface/ui_level_carousel_refresh.c.txt for the recovery notes.
 *
 * @address 0x4a4ee0
 */
void UiCarousels::level_carousel_refresh(widget_instance *widget)
{
    int32_t visible[3];
    uint8_t profile_record[0x1ffc];
    int32_t i;

    memcpy(profile_record, &profile_globals_block[0].profile, sizeof(profile_record));
    halo::interface::set_profile_name(widget->extended_description, (const uint16_t *)(profile_record + 2));
    halo::interface::widget_list_scroll_window(visible, widget);

    for (i = 0; i < 3; i++) {
        widget_instance *row;
        int32_t depth;

        if (visible[i] == -1) {
            return;
        }
        row = widget->first_child;
        for (depth = 0; depth < i && row != (widget_instance *)0; depth++) {
            row = row->next_sibling;
        }
        halo::interface::ui_level_carousel_row_refresh(row, visible[i]);
    }
}

/**
 * Original UI routine; see docs/original/interface/ui_level_carousel_row_refresh.c.txt for the recovery notes.
 *
 * Register convention: ECX -> widget, EAX -> level_index
 *
 * @address 0x4a4e20
 */
void UiCarousels::level_carousel_row_refresh(widget_instance *widget, int32_t level_index)
{
    widget_instance *row1 = widget->first_child;
    widget_instance *row2 = row1->next_sibling;
    widget_instance *row3 = row2->next_sibling;
    widget_instance *row4 = row3->next_sibling;
    widget_instance *row5 = row4->next_sibling;
    widget_instance *row6 = row5->next_sibling;
    level_select_entry *entry = &level_select_entries[level_index];

    row4->background_bitmap_frame = 1;
    row5->background_bitmap_frame = 2;
    row6->background_bitmap_frame = 3;

    if (entry->valid == 0 && entry->flag_bit1 == 0 && entry->flag_bit2 == 0 && entry->flag_bit3 == 0) {
        row1->selection_index = 10;
        row2->background_bitmap_frame = 10;
        row3->selection_index = 10;
        row4->state = 0;
        row5->state = 0;
        row6->state = 0;
        return;
    }

    row1->selection_index = (int16_t)level_index;
    row2->background_bitmap_frame = (int16_t)level_index;
    row3->selection_index = (int16_t)level_index;
    if (level_select_flags_0071916b == 1 && level_index == level_select_flags_0071916a) {
        row3->selection_index = 0xb;
    }
    row4->state = entry->flag_bit1;
    row5->state = entry->flag_bit2;
    row6->state = entry->flag_bit3;
}

/**
 * Recomputes the 3-wide previous/current/next window of the shared map_list into this widget's three visible row
 * children (their label's selection_index and the row's own background_bitmap_frame all get the same 16 bit
 * map_id).
 *
 * @address 0x4a6940
 */
void UiCarousels::map_list_carousel_refresh_window(widget_instance *widget)
{
    uint8_t profile_record[0x1ffc];
    int32_t window[3];
    int32_t slot;

    memcpy(profile_record, &profile_globals_block[0].profile, sizeof(profile_record));
    halo::interface::set_profile_name(widget->extended_description, (const uint16_t *)(profile_record + 2));

    halo::interface::widget_list_scroll_window(window, widget);

    for (slot = 0; slot < 3; slot = slot + 1) {
        widget_instance *row;
        widget_instance *label;
        widget_instance *value;
        int16_t map_id;

        if (window[slot] == -1) {
            return;
        }

        row = widget->first_child;
        if (slot > 0) {
            widget_instance *cur = row;
            int32_t depth = 0;
            row = 0;
            do {
                row = 0;
                if (cur == 0) break;
                cur = cur->next_sibling;
                depth = depth + 1;
                row = cur;
            } while (depth < slot);
        }

        label = row->first_child->next_sibling;
        value = label->next_sibling;
        map_id = (int16_t)map_list[window[slot]].map_id;
        row->first_child->selection_index = map_id;
        label->background_bitmap_frame = map_id;
        value->selection_index = map_id;
    }
}

/**
 * Marks which of the 3 game-variant carousel slots already hold one of the candidate ids, then assigns each
 * still-unmatched candidate into the first free slot once saved_game_get_variant fills in and confirms the
 * variant data.
 *
 * Register convention: EBX -> candidate_ids, stack -> count
 *
 * @address 0x4a7570
 */
void UiCarousels::variant_carousel_slot_cache_populate(int32_t *candidate_ids, int32_t count)
{
    uint8_t slot_filled[3] = {0, 0, 0};
    int32_t slot;
    int32_t i;

    for (slot = 0; slot < 3; slot = slot + 1) {
        if (variant_carousel_slots[slot].id != -1) {
            for (i = 0; i < count; i = i + 1) {
                if (variant_carousel_slots[slot].id == candidate_ids[i]) {
                    slot_filled[slot] = 1;
                    break;
                }
            }
        }
    }

    for (i = 0; i < count; i = i + 1) {
        int32_t id = candidate_ids[i];
        if (id != -1) {
            int32_t found_slot = 0;
            while (found_slot < 3 && id != variant_carousel_slots[found_slot].id) {
                found_slot = found_slot + 1;
            }
            if (found_slot == 3) {
                int32_t free_slot = 0;
                while (free_slot < 3 && slot_filled[free_slot] == 1) {
                    free_slot = free_slot + 1;
                }
                if (halo::saved_games::saved_game_get_variant(id, (game_variant *)variant_carousel_slots[free_slot].unknown)) {
                    variant_carousel_slots[free_slot].id = id;
                    slot_filled[free_slot] = 1;
                }
            }
        }
    }
}

}
