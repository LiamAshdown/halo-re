#include "halo/interface/ifr2_players.hpp"
#include <string.h>
#include "halo/interface/api.hpp"
#include "saved_games.h"

#ifdef interface
#undef interface
#endif

extern "C" {
extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles];
extern int32_t ui_list_current;
extern growable_array ui_lists[3];
}

namespace halo::interface {

/**
 * Rebuilds this widget's rows and refreshes the profile-name label, then either blanks the profile-details
 * sub-tree (when the selected combo index is out of range) or refreshes it from the selected ui_list entry's
 * data pointer, finally showing/hiding a fixed row 11 slots down depending on whether the list is empty.
 *
 * @address 0x4a85f0
 */
void PlayerProfiles::select_list_widget_build(widget_instance *widget)
{
    uint8_t profile_record[k_saved_player_profile_size];
    int32_t combo_index;
    widget_instance *row;
    widget_instance *target;
    int32_t depth;

    halo::interface::ui_list_widget_rebuild_rows(widget, (ui_list_item_format_function)((void *)halo::interface::ui_list_default_item_format));

    memcpy(profile_record, &profile_globals_block[0].profile, sizeof(profile_record));
    halo::interface::set_profile_name(widget->extended_description->first_child, (const uint16_t *)(profile_record + 2));

    combo_index = *(int16_t *)&((struct widget_instance *)widget)->text;
    if (combo_index < 0 || (uint16_t)widget->item_count <= combo_index) {
        widget_instance *a = widget->extended_description->first_child->next_sibling->first_child;
        widget_instance *b = a->next_sibling;
        widget_instance *c = b->next_sibling->first_child;
        widget_instance *d1 = c->next_sibling;
        widget_instance *d2 = d1->next_sibling;
        widget_instance *d3 = d2->next_sibling;
        widget_instance *d4 = d3->next_sibling;
        widget_instance *d5 = d4->next_sibling;
        widget_instance *d6 = d5->next_sibling;

        a->state = 0;
        b->background_bitmap_frame = 0x12;
        c->state = 1;
        d1->state = 0;
        d2->state = 0;
        d3->state = 0;
        d4->state = 0;
        d5->state = 0;
        d6->state = 0;
    } else {
        void *item_data = 0;
        if (combo_index < ui_lists[ui_list_current].count) {
            ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + combo_index;
            item_data = entry->data;
        }
        halo::interface::player_profile_details_widget_refresh(widget->extended_description->first_child->next_sibling,
                                              (const uint8_t *)item_data);
    }

    row = widget->first_child;
    depth = 0;
    {
        widget_instance *cur = row;
        row = 0;
        do {
            row = 0;
            if (cur == 0) break;
            cur = cur->next_sibling;
            depth = depth + 1;
            row = cur;
        } while (depth < 0x0b);
    }

    target = row->first_child->next_sibling;
    if (widget->item_count == 0) {
        target->hidden = 1;
        target->scale = 0.333f;
    } else {
        target->hidden = 0;
        target->scale = 1.0f;
    }
}

} // namespace halo::interface

namespace halo::interface {

void player_profile_select_list_widget_build(widget_instance *widget)
{
    halo::interface::PlayerProfiles::select_list_widget_build(widget);
}

}
