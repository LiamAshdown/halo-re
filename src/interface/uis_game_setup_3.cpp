/**
 * Level, map and variant selection lists and the campaign start and restart paths.
 */

#include "crt.h"
#include "halo/interface/engine_state.hpp"
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
#include "halo/saved_games/api.hpp"

extern "C" {
extern int32_t ui_list_current;
extern growable_array ui_lists[3];
extern int16_t local_player_count;
extern uint8_t profile_globals_block[0x60a4];
extern int32_t cached_profile_slot;
extern char last_profile_name[];
extern campaign_level_entry known_campaign_levels_00692acc[10];
extern uint8_t split_screen_quit_prompt_armed;
extern uint8_t selected_level_active_00719878;
extern uint8_t selected_level_pending_00719778;
extern uint8_t network_wait_flag_00719739;
extern void widget_play_sound_effect(int16_t effect_id);
}

namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static uint8_t level_unlocked_for(int16_t player, int32_t level_id)
{
    uint8_t profile_copy[0x1ffc];
    int16_t type;
    int16_t last_level;

    memcpy(profile_copy, profile_globals_block + player * 0x2004, sizeof(profile_copy));
    halo::saved_games::player_profile_scan_campaign_progress(&type, (saved_player_profile *)profile_copy, &last_level);
    return profile_copy[0x11e + level_id] != 0 || level_id == last_level + 1 || level_id == 0;
}

}

/**
 * Original UI routine; see docs/original/interface/ui_level_select_confirm_choice.c.txt for the recovery notes.
 *
 * @address 0x49ce00
 */
uint8_t UiGameSetup::level_select_confirm_choice(widget_instance *widget)
{
    int16_t list_index = *(int16_t *)&((struct widget_instance *)widget)->text;
    int32_t level_id = -1;
    uint8_t unlocked = 0;
    growable_array *list = &ui_lists[ui_list_current];
    int16_t player;

    if (list_index >= 0 && list_index < list->count) {
        level_id = ((ui_list_item *)list->data)[list_index].id;
    }
    if (local_player_count == 1) {
        unlocked = level_unlocked_for(0, level_id);
        if (cached_profile_slot != halo::saved_games::globals().player_profile_slots_handle) {
            if (halo::saved_games::globals().player_profile_slots_handle != -1) {
                halo::saved_games::saved_game_get_directory_by_handle(halo::saved_games::globals().player_profile_slots_handle, last_profile_name);
            }
            cached_profile_slot = halo::saved_games::globals().player_profile_slots_handle;
        }
        if (last_profile_name[0] != 0) {
            halo::saved_games::saved_game_last_profile_clear(last_profile_name);
        }
    } else if (local_player_count == 2) {
        for (player = 0; player <= 1 && !unlocked; player++) {
            unlocked = level_unlocked_for(player, level_id);
        }
    }
    if (unlocked != 1) {
        widget_play_sound_effect(4);
        return unlocked;
    }
    split_screen_quit_prompt_armed = 0;
    strncpy(halo::interface::state::current_campaign_level_path, known_campaign_levels_00692acc[level_id].path, 0xff);
    selected_level_active_00719878 = 0;
    selected_level_pending_00719778 = 1;
    network_wait_flag_00719739 = 0;
    return 1;
}

}
