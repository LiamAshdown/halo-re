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
#include "halo/input/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/interface/wide_text.hpp"

static auto &profile_globals_block = halo::link::ref<saved_player_profile_slot [k_maximum_local_player_profiles]>(halo::ui::vars().profile_globals_block);
static auto &ui_list_current = halo::link::ref<int32_t>(halo::ui::vars().ui_list_current);
static auto &ui_lists = halo::link::ref<growable_array [3]>(halo::ui::vars().ui_lists);
static auto &known_campaign_levels_00692acc = halo::link::ref<char []>(halo::ui::vars().known_campaign_levels_00692acc);
static auto &pending_difficulty = halo::link::ref<uint8_t>(halo::ui::vars().pending_difficulty);
static auto &split_screen_quit_prompt_armed = halo::link::ref<uint8_t>(halo::ui::vars().split_screen_quit_prompt_armed);
static auto &selected_level_active_00719878 = halo::link::ref<uint8_t>(halo::ui::vars().selected_level_active_00719878);
static auto &selected_level_pending_00719778 = halo::link::ref<uint8_t>(halo::ui::vars().selected_level_pending_00719778);
static auto &network_wait_flag_00719739 = halo::link::ref<uint8_t>(halo::ui::vars().network_wait_flag_00719739);
static auto &profile_slot_id = halo::link::ref<int16_t []>(halo::ui::vars().profile_slot_id);
static auto &game_variant_saved_default = halo::link::ref<int16_t>(halo::ui::vars().game_variant_saved_default);
static auto &cached_profile_slot = halo::link::ref<int32_t>(halo::ui::vars().cached_profile_slot);
static auto &last_profile_name = halo::link::ref<char []>(halo::ui::vars().last_profile_name);

namespace halo::ui {

/**
 * Rebuilds this widget's rows, then refreshes the linked game-variant description widget from the currently
 * selected ui_list entry's data pointer.
 *
 * @address 0x4a84d0
 */
void UiGameSetup::game_variant_list_widget_build(widget_instance *widget)
{
    saved_player_profile profile_record;
    int16_t combo_index;
    const uint16_t *variant_description = 0;

    halo::interface::ui_list_widget_rebuild_rows(widget, (ui_list_item_format_function)((void *)halo::interface::ui_list_default_item_format));

    profile_record = profile_globals_block[0].profile;
    halo::interface::set_profile_name(widget->extended_description->first_child, profile_record.name);

    combo_index = halo::interface::widget_list_committed(widget);
    if (combo_index > -1 && combo_index < ui_lists[ui_list_current].count) {
        ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + combo_index;
        variant_description = (const uint16_t *)entry->data;
    }

    halo::interface::multiplayer_settings_select_list_update_item(
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
    int16_t requested_index = event[1];
    int16_t i;

    (void)widget;

    if (halo::game::globals().local_player_count >= 2) {
        i = 0;
        if (halo::input::globals().joystick_slot_devices[0] == -1 || requested_index == 0) {
            halo::game::globals().local_player_count = 1;
            halo::interface::display_error(0x13, -1, 1, 0);
            return 0;
        }
    } else {
        i = -1;
    }

    pending_difficulty = 1;
    split_screen_quit_prompt_armed = 0;
    strncpy(halo::interface::state::current_campaign_level_path, known_campaign_levels_00692acc, 0xff);
    selected_level_active_00719878 = 0;
    selected_level_pending_00719778 = 1;
    halo::networking::globals().game_mode = 0;
    network_wait_flag_00719739 = 1;
    profile_slot_id[0] = requested_index;
    if (i != -1) {
        game_variant_saved_default = i;
    }
    if (cached_profile_slot != halo::saved_games::globals().player_profile_slots_handle) {
        if (halo::saved_games::globals().player_profile_slots_handle != -1) {
            halo::saved_games::saved_game_get_directory_by_handle(halo::saved_games::globals().player_profile_slots_handle, last_profile_name);
        }
        cached_profile_slot = halo::saved_games::globals().player_profile_slots_handle;
    }
    if (last_profile_name[0] != '\0') {
        halo::saved_games::saved_game_last_profile_clear(last_profile_name);
    }
    return 1;
}

}
