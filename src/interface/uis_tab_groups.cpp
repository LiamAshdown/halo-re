/**
 * Tab-group synchronisation for the 5, 7, 9-wide and grouped tab layouts of the UI.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>

#include "halo/interface/uis_tab_groups.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

static auto &ui_split_screen = halo::link::ref<uint8_t>(halo::ui::vars().ui_split_screen);
static auto &input_device_count = halo::link::ref<int32_t>(halo::ui::vars().input_device_count);
static auto &profile_globals_block = halo::link::ref<saved_player_profile_slot [k_maximum_local_player_profiles]>(halo::ui::vars().profile_globals_block);

namespace halo::ui {

/**
 * Original UI routine; see docs/original/interface/ui_tab_group_sync_5wide.c.txt for the recovery notes.
 *
 * @address 0x4a4cf0
 */
void UiTabGroups::tab_group_sync_5wide(widget_instance *widget)
{
    widget_instance *child = widget->first_child;
    int16_t index = 0;
    widget_instance *target;

    if (child != (widget_instance *)0) {
        while (child != widget->focused_child) {
            child = child->next_sibling;
            index = index + 1;
            if (child == (widget_instance *)0) {
                break;
            }
        }
        if (index > 4 || index == -1) {
            return;
        }
    }
    target = widget->extended_description->first_child;
    target->selection_index = index;
    target->next_sibling->background_bitmap_frame = index;
}

/**
 * Original UI routine; see docs/original/interface/ui_tab_group_sync_7wide.c.txt for the recovery notes.
 *
 * @address 0x4a4cb0
 */
void UiTabGroups::tab_group_sync_7wide(widget_instance *widget)
{
    widget_instance *child = widget->first_child;
    int16_t index = 0;
    widget_instance *target;

    if (child != (widget_instance *)0) {
        while (child != widget->focused_child) {
            child = child->next_sibling;
            index = index + 1;
            if (child == (widget_instance *)0) {
                break;
            }
        }
        if (index > 6 || index == -1) {
            return;
        }
    }
    target = widget->extended_description->first_child;
    target->selection_index = index;
    target->next_sibling->background_bitmap_frame = index;
}

/**
 * Original UI routine; see docs/original/interface/ui_tab_group_sync_9wide.c.txt for the recovery notes.
 *
 * @address 0x4a62d0
 */
void UiTabGroups::tab_group_sync_9wide(widget_instance *widget)
{
    widget_instance *child = widget->first_child;
    int16_t index = 0;
    widget_instance *cursor;
    int16_t position;

    if (child != (widget_instance *)0) {
        while (child != widget->focused_child) {
            child = child->next_sibling;
            index = index + 1;
            if (child == (widget_instance *)0) {
                break;
            }
        }
        if (index > 8 || index == -1) {
            goto sync_visibility;
        }
    }
    {
        widget_instance *display = widget->extended_description->first_child;

        display->selection_index = index;
        display->next_sibling->background_bitmap_frame = (index != 8) ? index : 6;
    }

sync_visibility:
    position = 0;
    for (cursor = widget->first_child; cursor != (widget_instance *)0; cursor = cursor->next_sibling) {
        if (position == 0) {
            if (ui_split_screen == 0) {
                cursor->hidden = 1;
                cursor->scale = 0.333f;
                if (widget->focused_child == cursor) {
                    widget->focused_child = cursor->next_sibling;
                }
            } else {
                cursor->hidden = 0;
                cursor->scale = 1.0f;
            }
        } else if (position == 2) {
            if ((int16_t)input_device_count != 0) {
                cursor->hidden = 0;
                cursor->scale = 1.0f;
            } else {
                cursor->hidden = 1;
                cursor->scale = 0.333f;
            }
        }
        position = position + 1;
    }
}

/**
 * Original UI routine; see docs/original/interface/ui_tab_group_sync_grouped.c.txt for the recovery notes.
 *
 * @address 0x4a4d30
 */
void UiTabGroups::tab_group_sync_grouped(widget_instance *widget)
{
    widget_instance *child = widget->first_child;
    int16_t index = 0;
    widget_instance *display;
    int16_t group_offset;
    widget_instance *cursor;
    int16_t position;

    if (child != (widget_instance *)0) {
        while (child != widget->focused_child) {
            child = child->next_sibling;
            index = index + 1;
            if (child == (widget_instance *)0) {
                break;
            }
        }
    }

    display = widget->extended_description->first_child;
    switch (index) {
    case 1:
    case 2:
    case 3:
        display->background_bitmap_frame = 0;
        group_offset = -1;
        goto apply;
    case 5:
    case 6:
        display->background_bitmap_frame = 0;
        group_offset = -2;
        break;
    case 7:
        display->background_bitmap_frame = 2;
        group_offset = -2;
        break;
    default:
        goto sync_visibility;
    }
apply:
    if ((int16_t)(index + group_offset) != -1) {
        display->next_sibling->selection_index = index + group_offset;
    }

sync_visibility:
    position = 0;
    for (cursor = widget->first_child; cursor != (widget_instance *)0; cursor = cursor->next_sibling) {
        if (position == 0 || position == 4) {
            cursor->hidden = 1;
        }
        position = position + 1;
    }

    {
        saved_player_profile profile_copy;

        profile_copy = profile_globals_block[0].profile;
        halo::interface::set_profile_name(widget, profile_copy.name);
    }
}

}
