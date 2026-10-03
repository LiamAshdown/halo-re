/**
 * Level, map and variant selection lists and the campaign start and restart paths.
 */

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>
#include "objects.h"
#include "units.h"

#include "halo/interface/uis_game_setup.hpp"

extern "C" {
extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles];
extern int32_t ui_list_current;
extern growable_array ui_lists[3];
extern void ui_list_widget_rebuild_rows(widget_instance *widget, ui_list_item_format_function format_item);
extern uint8_t ui_list_default_item_format(void *item_buffer, int32_t item_index, void *list_items);
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source);
extern void multiplayer_settings_select_list_update_item(widget_instance *description_widget, const uint16_t *variant_description);
extern int16_t local_player_count;
extern int32_t joystick_slot_devices[4];
extern char known_campaign_levels_00692acc[];
extern char unknown_00719779[0x100];
extern uint8_t pending_difficulty;
extern uint8_t split_screen_quit_prompt_armed;
extern uint8_t selected_level_active_00719878;
extern uint8_t selected_level_pending_00719778;
extern int16_t network_game_mode;
extern uint8_t network_wait_flag_00719739;
extern int16_t profile_slot_id[];
extern int16_t game_variant_saved_default;
extern int32_t saved_player_profile_slots_handle;
extern int32_t cached_profile_slot;
extern char last_profile_name[];
extern void display_error(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error);
extern uint8_t saved_game_get_directory_by_handle(int32_t slot, char *out_name);
extern void saved_game_last_profile_clear(char *name);
}

namespace halo::ui {

/**
 * Rebuilds this widget's rows, then refreshes the linked game-variant description widget from the currently
 * selected ui_list entry's data pointer.
 *
 * @address 0x4a84d0
 */
void UiGameSetup::game_variant_list_widget_build(widget_instance *widget)
{
    uint8_t profile_record[0x1ffc];
    int16_t combo_index;
    const uint16_t *variant_description = 0;

    ui_list_widget_rebuild_rows(widget, (ui_list_item_format_function)((void *)ui_list_default_item_format));

    memcpy(profile_record, &profile_globals_block[0].profile, sizeof(profile_record));
    set_profile_name(widget->extended_description->first_child, (const uint16_t *)(profile_record + 2));

    combo_index = *(int16_t *)&((struct widget_instance *)widget)->text;
    if (combo_index > -1 && combo_index < ui_lists[ui_list_current].count) {
        ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + combo_index;
        variant_description = (const uint16_t *)entry->data;
    }

    multiplayer_settings_select_list_update_item(
        widget->extended_description->first_child->next_sibling, variant_description);
}

/**
 * Validates the requested difficulty/option index (when more than one is configured), then starts a new campaign
 * game at the first level; reports error 0x13 and forces single-option mode if the requested index is not valid.
 *
 * @address 0x49cfd0
 */
uint32_t UiGameSetup::start_campaign_from_level_one(void *widget, int16_t *event)
{
    int16_t requested_index = *(int16_t *)((uint8_t *)event + 2);
    int16_t i;

    (void)widget;

    if (local_player_count >= 2) {
        i = 0;
        while (joystick_slot_devices[i] == -1 || i == requested_index) {
            i = i + 1;
            if (i > 0) {
                goto report_error;
            }
        }
        if (i == -1) {
            goto report_error;
        }
    } else {
        i = -1;
    }

    pending_difficulty = 1;
    split_screen_quit_prompt_armed = 0;
    strncpy(unknown_00719779, known_campaign_levels_00692acc, 0xff);
    selected_level_active_00719878 = 0;
    selected_level_pending_00719778 = 1;
    network_game_mode = 0;
    network_wait_flag_00719739 = 1;
    profile_slot_id[0] = requested_index;
    if (i != -1) {
        game_variant_saved_default = i;
    }
    if (cached_profile_slot != saved_player_profile_slots_handle) {
        if (saved_player_profile_slots_handle != -1) {
            saved_game_get_directory_by_handle(saved_player_profile_slots_handle, last_profile_name);
        }
        cached_profile_slot = saved_player_profile_slots_handle;
    }
    if (last_profile_name[0] != '\0') {
        saved_game_last_profile_clear(last_profile_name);
    }
    return 1;

report_error:
    local_player_count = 1;
    display_error(0x13, -1, 1, 0);
    return 0;
}

}
