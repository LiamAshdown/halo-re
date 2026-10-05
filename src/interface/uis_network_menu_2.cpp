/**
 * Network game menu behaviour: host setup, adapter details, client connection and wait timeouts.
 */

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "halo/saved_games/layout.hpp"
#include <string.h>
#include "objects.h"
#include "units.h"

#include "halo/interface/uis_network_menu.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

static auto &profile_globals_block = halo::link::ref<saved_player_profile_slot [k_maximum_local_player_profiles]>(halo::ui::vars().profile_globals_block);
static auto &map_list = halo::link::ref<map_list_entry *>(halo::ui::vars().map_list);
static auto &save_in_progress_00719010 = halo::link::ref<uint8_t>(halo::ui::vars().save_in_progress_00719010);
static auto &quality_selection_00692b04 = halo::link::ref<int32_t>(halo::ui::vars().quality_selection_00692b04);
static auto &resolution_selection_00719204 = halo::link::ref<int32_t>(halo::ui::vars().resolution_selection_00719204);
static auto &network_host_name_00719170 = halo::link::ref<uint16_t [0x40]>(halo::ui::vars().network_host_name_00719170);
static auto &network_host_subname_007191f0 = halo::link::ref<uint16_t [9]>(halo::ui::vars().network_host_subname_007191f0);
static auto &resolution_row_count_table_0065bfb4 = halo::link::ref<int32_t [5]>(halo::ui::vars().resolution_row_count_table_0065bfb4);
static auto &resolution_index_table_0065bf74 = halo::link::ref<int32_t []>(halo::ui::vars().resolution_index_table_0065bf74);
static auto &sv_maxplayers_value = halo::link::ref<int32_t>(halo::ui::vars().sv_maxplayers_value);
static auto &network_game_info_packet_flag = halo::link::ref<uint8_t>(halo::ui::vars().network_game_info_packet_flag);
#include "halo/interface/wide_text.hpp"

namespace halo::ui {

/**
 * Rebuilds this widget's rows with the map-name formatter, then mirrors the widget's cached "selected combo
 * index" (offset 0x3c) as a map_id into the three sibling widgets under extended_description.
 *
 * @address 0x4a8440
 */
void UiNetworkMenu::network_adapter_list_widget_build(widget_instance *widget)
{
    saved_player_profile profile_record;
    widget_instance *target1;
    widget_instance *target2;
    widget_instance *target3;
    int16_t map_id;

    halo::interface::ui_list_widget_rebuild_rows(widget, (ui_list_item_format_function)((void *)halo::interface::ui_list_item_format_name_and_cache_flag));

    profile_record = profile_globals_block[0].profile;
    halo::interface::set_profile_name(widget->extended_description->first_child, profile_record.name);

    target1 = widget->extended_description->first_child->next_sibling->first_child;
    target2 = target1->next_sibling;
    target3 = target2->next_sibling;

    map_id = (int16_t)map_list[halo::interface::widget_list_committed(widget)].map_id;
    target1->selection_index = map_id;
    target2->background_bitmap_frame = map_id;
    target3->selection_index = map_id;
}

/**
 *
 * @address 0x4a2ad0
 */
uint8_t UiNetworkMenu::network_host_setup_defaults_init(widget_instance *widget)
{
    saved_player_profile profile;
    int32_t choice;
    int32_t last_row;
    int32_t index;
    widget_instance *row1;
    widget_instance *row2;
    widget_instance *control;

    quality_selection_00692b04 = 4;
    if (save_in_progress_00719010 != 0) {
        resolution_selection_00719204 = 0;
    }
    if (halo::saved_games::globals().player_profile_slots_handle != -1) {
        profile = profile_globals_block[0].profile;
    } else {
        halo::saved_games::player_profile_set_default_server_options(&profile);
    }

    wcslen((const wchar_t *)profile.server_name);
    wcscpy((wchar_t *)network_host_name_00719170, (const wchar_t *)profile.server_name);
    wcslen((const wchar_t *)profile.server_password);
    wcscpy((wchar_t *)network_host_subname_007191f0, (const wchar_t *)profile.server_password);

    choice = (profile.connection_type > 4) ? 4 : profile.connection_type;
    if (halo::saved_games::k_connection_type_locked) {
        choice = halo::saved_games::k_connection_type_t1_lan;
    }
    last_row = resolution_row_count_table_0065bfb4[choice] - 1;
    if ((int32_t)profile.server_maximum_players_index > last_row) {
        profile.server_maximum_players_index = (uint8_t)last_row;
    }
    quality_selection_00692b04 = choice;
    index = profile.server_maximum_players_index;
    if (index < 0) {
        index = 0;
    } else if (index > last_row) {
        index = last_row;
    }
    resolution_selection_00719204 = index;
    sv_maxplayers_value = resolution_index_table_0065bf74[index];

    row1 = widget->first_child->next_sibling->next_sibling;
    control = row1->first_child->next_sibling;
    if (network_game_info_packet_flag != 0 && !halo::saved_games::k_connection_type_locked) {
        row1->scale = 1.0f;
        row1->hidden = 0;
        control->selection_index = (int16_t)choice;
    } else {
        row1->scale = 0.333f;
        row1->hidden = 1;
        control->selection_index = 4;
    }

    row2 = row1->next_sibling;
    control = row2->first_child->next_sibling;
    control->selection_index = (int16_t)index;
    control->item_count = (uint16_t)resolution_row_count_table_0065bfb4[choice];
    if (save_in_progress_00719010 != 0) {
        row2->scale = 0.333f;
        row2->hidden = 1;
    } else {
        row2->scale = 1.0f;
        row2->hidden = 0;
    }
    return 1;
}

}
