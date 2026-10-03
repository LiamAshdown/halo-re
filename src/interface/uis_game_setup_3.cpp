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
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

static auto &ui_list_current = halo::link::ref<int32_t>(halo::ui::vars().ui_list_current);
static auto &ui_lists = halo::link::ref<growable_array [3]>(halo::ui::vars().ui_lists);
static auto &profile_globals_block = halo::link::ref<uint8_t [0x60a4]>(halo::ui::vars().profile_globals_block);
static auto &cached_profile_slot = halo::link::ref<int32_t>(halo::ui::vars().cached_profile_slot);
static auto &last_profile_name = halo::link::ref<char []>(halo::ui::vars().last_profile_name);
static auto &known_campaign_levels_00692acc = halo::link::ref<campaign_level_entry [10]>(halo::ui::vars().known_campaign_levels_00692acc);
static auto &split_screen_quit_prompt_armed = halo::link::ref<uint8_t>(halo::ui::vars().split_screen_quit_prompt_armed);
static auto &selected_level_active_00719878 = halo::link::ref<uint8_t>(halo::ui::vars().selected_level_active_00719878);
static auto &selected_level_pending_00719778 = halo::link::ref<uint8_t>(halo::ui::vars().selected_level_pending_00719778);
static auto &network_wait_flag_00719739 = halo::link::ref<uint8_t>(halo::ui::vars().network_wait_flag_00719739);

namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static uint8_t level_unlocked_for(int16_t player, int32_t level_id)
{
    uint8_t profile_copy[k_saved_player_profile_size];
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
    if (halo::game::globals().local_player_count == 1) {
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
    } else if (halo::game::globals().local_player_count == 2) {
        for (player = 0; player <= 1 && !unlocked; player++) {
            unlocked = level_unlocked_for(player, level_id);
        }
    }
    if (unlocked != 1) {
        halo::interface::widget_play_sound_effect(4);
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
