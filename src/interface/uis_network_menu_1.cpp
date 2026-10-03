/**
 * Network game menu behaviour: host setup, adapter details, client connection and wait timeouts.
 */

#include "crt.h"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/text/api.hpp"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

#include "halo/interface/uis_network_menu.hpp"
#include "halo/memory/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "saved_games.h"
#include "halo/interface/widget_pool.hpp"
#include "halo/interface/wide_text.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"


static auto &ui_list_current = halo::link::ref<int32_t>(halo::ui::vars().ui_list_current);
static auto &ui_lists = halo::link::ref<growable_array[3]>(halo::ui::vars().ui_lists);
static auto &profile_globals_block = halo::link::ref<saved_player_profile_slot[k_maximum_local_player_profiles]>(halo::ui::vars().profile_globals_block);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &network_host_name_field_00719238 = halo::link::ref<uint16_t[32]>(halo::ui::vars().network_host_name_field_00719238);
static auto &network_host_subname_007191f0 = halo::link::ref<uint16_t[9]>(halo::ui::vars().network_host_subname_007191f0);
static auto &selected_saved_item = halo::link::ref<int32_t>(halo::ui::vars().selected_saved_item);
static auto &saved_item_working_copy = halo::link::ref<saved_player_profile>(halo::ui::vars().saved_item_working_copy);
static auto &network_game_option_a_00719210 = halo::link::ref<uint32_t>(halo::ui::vars().network_game_option_a_00719210);
static auto &network_game_option_b_00719214 = halo::link::ref<uint32_t>(halo::ui::vars().network_game_option_b_00719214);
static auto &network_host_name_00719170 = halo::link::ref<uint16_t[144]>(halo::ui::vars().network_host_name_00719170);
static auto &quality_selection_00692b04 = halo::link::ref<int32_t>(halo::ui::vars().quality_selection_00692b04);
static auto &resolution_row_count_table_0065bfb4 = halo::link::ref<int32_t[5][1]>(halo::ui::vars().resolution_row_count_table_0065bfb4);
static auto &resolution_selection_00719204 = halo::link::ref<int32_t>(halo::ui::vars().resolution_selection_00719204);
static auto &resolution_index_table_0065bf74 = halo::link::ref<int32_t[]>(halo::ui::vars().resolution_index_table_0065bf74);
static auto &sv_maxplayers_value = halo::link::ref<uint32_t>(halo::ui::vars().sv_maxplayers_value);
static auto &network_resolved_local_address = halo::link::ref<uint32_t>(halo::ui::vars().network_resolved_local_address);
static auto &ip_port_format_string_0066a564 = halo::link::ref<uint16_t[]>(halo::ui::vars().ip_port_format_string_0066a564);
static auto &network_host_name_flag_00719276 = halo::link::ref<uint8_t>(halo::ui::vars().network_host_name_flag_00719276);
static auto &ui_network_wait_active = halo::link::ref<uint8_t>(halo::ui::vars().ui_network_wait_active);
static auto &ui_network_wait_start_time = halo::link::ref<int32_t>(halo::ui::vars().ui_network_wait_start_time);
static auto &ui_network_wait_timed_out = halo::link::ref<uint8_t>(halo::ui::vars().ui_network_wait_timed_out);
static auto &autopatch_status_state_00719234 = halo::link::ref<uint8_t>(halo::ui::vars().autopatch_status_state_00719234);
static auto &autopatch_status_active_00719235 = halo::link::ref<uint8_t>(halo::ui::vars().autopatch_status_active_00719235);
static auto &ui_server_option_flag_00692b10 = halo::link::ref<uint8_t>(halo::ui::vars().ui_server_option_flag_00692b10);
static auto &network_game_info_packet_flag = halo::link::ref<uint8_t>(halo::ui::vars().network_game_info_packet_flag);
static auto &server_browser_require_valid_entry = halo::link::ref<uint8_t>(halo::networking::vars().server_browser_require_valid_entry);


namespace halo::ui {

/**
 *
 * @address 0x4a4650
 */
void UiNetworkMenu::network_adapter_details_refresh(widget_instance *widget)
{
    widget_instance *row;
    widget_instance *c1, *c2, *c3, *c4, *c5, *c6;
    ui_list_item *entry = (ui_list_item *)0;

    halo::interface::ui_list_widget_rebuild_rows(widget, (ui_list_item_format_function)((void *)halo::interface::ui_list_default_item_format));

    {
        saved_player_profile profile_copy;

        profile_copy = profile_globals_block[0].profile;
        halo::interface::set_profile_name(widget, profile_copy.name);
    }

    row = widget->extended_description->first_child->next_sibling;
    c1 = row->first_child;
    c2 = c1->next_sibling;
    c3 = c2->next_sibling;
    c4 = c3->next_sibling;
    c5 = c4->next_sibling;
    c6 = c5->next_sibling;

    if (widget->selection_index >= 0 && widget->selection_index < ui_lists[ui_list_current].count) {
        entry = (ui_list_item *)ui_lists[ui_list_current].data + widget->selection_index;
    }
    row->background_bitmap_frame = 0;

    if (entry == (ui_list_item *)0 || entry->data == nullptr) {
        c4->state = 0;
        c5->state = 0;
        c6->state = 0;
        c1->selection_index = 0;
        c2->background_bitmap_frame = 0;
        return;
    }

    {
        const uint16_t *blob = (const uint16_t *)entry->data;
        int32_t type = *(const int32_t *)(blob + 2);

        c4->background_bitmap_frame = 1;
        c4->state = (type == 1);
        c5->background_bitmap_frame = 2;
        c5->state = (type == 2);
        c6->background_bitmap_frame = 3;
        c6->state = (type == 3);
        c1->selection_index = (int16_t)blob[0];
        c2->background_bitmap_frame = (int16_t)blob[0];

        c3->text = halo::memory::heap_reallocate(c3->text, 0x40, widget_memory_pool);
        if (c3->text != nullptr) {
            wcsncpy((wchar_t *)(halo::interface::widget_text(c3)), (const wchar_t *)(blob + 4), 0x1f);
            (halo::interface::widget_text(c3))[0x1f] = 0;
        }
    }
}

/**
 *
 * @address 0x4a4a30
 */
uint8_t UiNetworkMenu::network_client_connect_and_save(void)
{
    char name[0x20];
    char port[0xc];
    uint8_t result;

    halo::text::string_convert_unicode_to_ascii((uint8_t *)name, network_host_name_field_00719238, 0x20);
    halo::text::string_convert_unicode_to_ascii((uint8_t *)port, network_host_subname_007191f0, 9);
    result = halo::main::network_game_client_connect_to_address_async(name, port);
    if (result == 0 || halo::saved_games::globals().player_profile_slots_handle == -1) {
        return result;
    }

    halo::interface::saved_item_select(halo::saved_games::globals().player_profile_slots_handle);
    {
        saved_player_profile *record = ((selected_saved_item & 0xf) == 0) ? &saved_item_working_copy : nullptr;
        wcslen((const wchar_t *)network_host_name_field_00719238);
        wcscpy((wchar_t *)record->join_server_address, (const wchar_t *)network_host_name_field_00719238);
    }
    if (halo::interface::saved_item_has_unsaved_changes() != 0) {
        halo::interface::player_profile_save();
        return result;
    }
    selected_saved_item = -1;
    return result;
}

/**
 *
 * Register convention: ECX -> widget, ESI -> options_record
 *
 * @address 0x4a3960
 */
uint8_t UiNetworkMenu::network_game_options_populate(widget_instance *widget, const saved_player_profile *options_record)
{
    widget_instance *control;
    uint8_t value;

    if (options_record == nullptr) {
        return 0;
    }

    for (control = widget->first_child->first_child;
         control != (widget_instance *)0 && control->widget_type != uiwidgettype_spinner_list;
         control = control->next_sibling) {
    }
    value = options_record->connection_type;
    control->selection_index = (value < 5) ? value : 4;
    network_game_option_a_00719210 = options_record->server_port;
    network_game_option_b_00719214 = options_record->client_port;
    return 1;
}

/**
 *
 * @address 0x4a3b30
 */
void UiNetworkMenu::network_game_options_refresh(widget_instance *widget, const saved_player_profile *options_record)
{
    halo::saved_games::player_profile_set_default_server_options(&profile_globals_block[0].profile);
    halo::interface::widget_play_sound_effect(2);
    halo::interface::ui_network_game_options_populate(widget, options_record);
}

/**
 *
 * @address 0x4a2cb0
 */
void UiNetworkMenu::network_host_setup_refresh(widget_instance *widget)
{
    widget_instance *tab_group = widget->extended_description;
    widget_instance *row = widget->first_child;
    widget_instance *control = row->first_child->next_sibling;
    int32_t tab_index = -1;
    uint16_t *buffer;
    int32_t quality;
    int32_t resolution_row_count;
    int32_t resolution_index;
    widget_instance *ip_control;

    buffer = halo::interface::widget_pool_resize_text(control->text, 0x80);
    control->text = buffer;
    if (buffer != nullptr) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)network_host_name_00719170, 0x3f);
        (halo::interface::widget_text(control))[0x3f] = 0;
    }
    if (row->parent->focused_child == row) {
        tab_index = 0;
    }

    row = row->next_sibling;
    control = row->first_child->next_sibling;
    buffer = halo::interface::widget_pool_resize_text(control->text, 0x12);
    control->text = buffer;
    if (buffer != nullptr) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)network_host_subname_007191f0, 8);
        (halo::interface::widget_text(control))[8] = 0;
    }
    if (row->parent->focused_child == row) {
        tab_index = 1;
    }

    row = row->next_sibling;
    quality = (row->first_child->next_sibling)->selection_index;
    if (row->parent->focused_child == row) {
        tab_index = quality + 2;
    }

    row = row->next_sibling;
    control = row->first_child->next_sibling;
    if (control->selection_index < 0) {
        resolution_index = 0;
    } else {
        resolution_row_count = resolution_row_count_table_0065bfb4[quality][0] - 1;
        resolution_index = (control->selection_index <= resolution_row_count) ? control->selection_index
                                                                                : resolution_row_count;
    }
    quality_selection_00692b04 = quality;
    control->selection_index = (int16_t)resolution_index;
    resolution_selection_00719204 = resolution_index;
    control->item_count = *(uint16_t *)((uint8_t *)resolution_row_count_table_0065bfb4 + quality * 4);
    control->selection_index = (int16_t)resolution_index;

    if (resolution_index < 0) {
        resolution_index = 0;
    } else {
        resolution_row_count = resolution_row_count_table_0065bfb4[quality][0] - 1;
        if (resolution_index > resolution_row_count) {
            resolution_index = resolution_row_count;
            resolution_selection_00719204 = resolution_row_count;
        }
    }
    sv_maxplayers_value = resolution_index_table_0065bf74[resolution_index];
    if (row->parent->focused_child == row) {
        tab_index = 7;
    }

    row = row->next_sibling;
    row->hidden = 1;
    ip_control = row->first_child->next_sibling;
    ip_control->text = halo::memory::heap_reallocate(ip_control->text, 0x40, widget_memory_pool);
    if (ip_control->text != nullptr) {
        uint32_t swapped = ((network_resolved_local_address << 0x10 | network_resolved_local_address & 0xff00 |
                             network_resolved_local_address >> 0x10 & 0xff) << 8) |
                            (network_resolved_local_address >> 0x18);
        struct in_addr swapped_address;
        char *text;

        swapped_address.s_addr = swapped;
        text = inet_ntoa(swapped_address);
        char *scan = text;
        while (*scan != '\0') {
            scan++;
        }
        int32_t address_length = (int32_t)(scan - text);
        halo::text::string_convert_ascii_to_unicode(reinterpret_cast<uint16_t *>(ip_control->text), 0x3e, text);
        halo::text::string_format_wide_va_bounded(
            0x20 - address_length, reinterpret_cast<uint16_t *>(ip_control->text) + address_length,
            reinterpret_cast<const uint16_t *>(ip_port_format_string_0066a564), halo::networking::globals().game_socket_port);
        (halo::interface::widget_text(ip_control))[0x1f] = 0;
    }

    if (tab_index == -1) {
        tab_group->first_child->state = 0;
    } else {
        tab_group->first_child->selection_index = (int16_t)tab_index;
        tab_group->first_child->state = 1;
    }

    {
        saved_player_profile profile_copy;

        profile_copy = profile_globals_block[0].profile;
        halo::interface::set_profile_name(widget, profile_copy.name);
    }
}

/**
 *
 * @address 0x4a4b60
 */
void UiNetworkMenu::network_name_fields_refresh(widget_instance *widget)
{
    widget_instance *tab_group = widget->extended_description;
    widget_instance *row = widget->first_child;
    widget_instance *control = row->first_child->next_sibling;
    int32_t tab_index = -1;
    uint16_t *buffer;

    buffer = halo::interface::widget_pool_resize_text(control->text, 0x40);
    control->text = buffer;
    if (buffer != nullptr) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)network_host_name_field_00719238, 0x1f);
        (halo::interface::widget_text(control))[0x1f] = 0;
    }
    if (row->parent->focused_child == row) {
        tab_index = 0;
    }

    row = row->next_sibling;
    control = row->first_child->next_sibling;
    buffer = halo::interface::widget_pool_resize_text(control->text, 0x12);
    control->text = buffer;
    if (buffer != nullptr) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)network_host_subname_007191f0, 8);
        (halo::interface::widget_text(control))[8] = 0;
    }

    if (row->parent->focused_child == row) {
        tab_group->first_child->selection_index = 1;
        tab_group->first_child->state = 1;
    } else if (tab_index == -1) {
        tab_group->first_child->state = 0;
    } else {
        tab_group->first_child->selection_index = (int16_t)tab_index;
        tab_group->first_child->state = 1;
    }

    {
        saved_player_profile profile_copy;

        profile_copy = profile_globals_block[0].profile;
        halo::interface::set_profile_name(tab_group->first_child->next_sibling, profile_copy.name);
    }
}

/**
 *
 * @address 0x4a49c0
 */
uint32_t UiNetworkMenu::network_name_fields_reset(void)
{
    uint16_t unused_name_source[2077];

    if (halo::saved_games::globals().player_profile_slots_handle == -1) {
        halo::saved_games::player_profile_set_default_server_options(&profile_globals_block[0].profile);
    } else {
        saved_player_profile profile_copy;

        profile_copy = profile_globals_block[0].profile;
    }

    wcsncpy((wchar_t *)network_host_name_field_00719238, (const wchar_t *)unused_name_source, 0x1f);
    network_host_name_flag_00719276 = 0;
    network_host_subname_007191f0[0] = 0;
    return 1;
}

/**
 * If a network wait is not currently active, clears the start time and timeout flag. Otherwise, once 10 seconds
 * have elapsed since the wait started, raises the timeout flag; either way the active flag is cleared before
 * returning.
 *
 * @address 0x49c7b0
 */
void UiNetworkMenu::network_wait_timeout_check(void)
{
    if (ui_network_wait_active == 0) {
        ui_network_wait_start_time = -1;
        ui_network_wait_timed_out = 0;
    } else if (ui_network_wait_start_time != -1 && ui_network_wait_timed_out == 0) {
        int32_t now = halo::cseries::time_query_performance_counter_ms();

        ui_network_wait_active = 0;
        if ((uint32_t)(now - ui_network_wait_start_time) > 9999) {
            ui_network_wait_timed_out = 1;
            return;
        }
    }
    ui_network_wait_active = 0;
}

/**
 * If no wait is currently timed, samples the performance counter and converts it to milliseconds as the new wait
 * start time. Always marks the wait as active.
 *
 * @address 0x49c810
 */
void UiNetworkMenu::network_wait_timeout_start(void)
{
    if (ui_network_wait_start_time == -1) {
        large_integer counter;

        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        ui_network_wait_start_time = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    }
    ui_network_wait_active = 1;
}

/**
 * If a connectable server entry is selected, initiates a connection to it and, once connected, opens the
 * connected pre-game lobby widget.
 *
 * Register convention: matches ui_event_function (widget, event, out_handled)
 *
 * @address 0x49d2e0
 */
uint8_t UiNetworkMenu::server_list_connect_selected(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    (void)event;

    if (widget->focused_child != (widget_instance *)0 && widget->selection_index >= 0 &&
        widget->selection_index < (int16_t)widget->item_count && widget->list_items != nullptr &&
        widget->item_count != 0) {
        network_game_search_entry **entries = (network_game_search_entry **)widget->list_items;
        network_game_search_entry *entry = entries[widget->selection_index];
        const s_network_address *entry_address = (const s_network_address *)entry->identity;

        if (entry->joinable == 1) {
            if (entry->unknown_12a == 1 && entry_address->ipv4 != 0 &&
                entry_address->port != 0) {
                uint32_t session_info[9] = {0};
                s_network_address connect_address = {};
                int32_t connected;

                connect_address.ipv4 = entry_address->ipv4;
                connect_address.size = entry_address->size;
                connect_address.port = entry_address->port;
                halo::networking::network_debug_fill_canary_buffer(&session_info[5]);
                connected = halo::networking::network_connection_initiate(halo::networking::globals().client, entry->identity,
                                                         session_info, (const uint32_t *)&connect_address);
                if ((uint8_t)connected == 0) {
                    halo::networking::globals().host_handoff_requested = 1;
                    halo::interface::chat_close();
                    return 0;
                }

                {
                    void *page = halo::interface::widget_instance_find_root(widget);
                    datum_index parent_definition = (widget->parent != (widget_instance *)0)
                                                         ? widget->parent->definition
                                                         : (datum_index)-1;
                    int32_t sibling = halo::interface::widget_get_sibling_index(widget);
                    widget_instance *opened;

                    opened = halo::interface::chimera__load_ui_widget(
                        halo::tag_paths::connected_pregame_screen,
                        (datum_index)-1, (widget_instance *)0, (uint16_t)-1,
                        *(datum_index *)page, parent_definition, (int16_t)sibling);
                    if (opened != (widget_instance *)0) {
                        halo::networking::globals().game_mode = 1;
                    }
                    *out_handled = 1;
                    return opened != (widget_instance *)0;
                }
            }
        } else {
            halo::interface::widget_play_sound_effect(4);
        }
    }
    return 0;
}

/**
 *
 * @address 0x4a47c0
 */
uint8_t UiNetworkMenu::server_type_option_selected(widget_instance *widget)
{
    widget_instance *sibling = widget->parent->first_child;
    int32_t index = 0;

    while (sibling != (widget_instance *)0 && sibling != widget) {
        sibling = sibling->next_sibling;
        index++;
    }
    if (sibling == (widget_instance *)0) {
        return 0;
    }

    switch (index - 1) {
    case 0:
    case 2:
        autopatch_status_state_00719234 = 0;
        autopatch_status_active_00719235 = 0;
        ui_server_option_flag_00692b10 = 1;
        network_game_info_packet_flag = 1;
        server_browser_require_valid_entry = 1;
        break;
    case 1:
        network_game_info_packet_flag = 0;
        server_browser_require_valid_entry = 0;
        ui_server_option_flag_00692b10 = 1;
        break;
    case 4:
        ui_server_option_flag_00692b10 = 0;
        autopatch_status_state_00719234 = 0;
        autopatch_status_active_00719235 = 0;
        network_game_info_packet_flag = 1;
        server_browser_require_valid_entry = 1;
        break;
    case 5:
        ui_server_option_flag_00692b10 = 0;
        network_game_info_packet_flag = 0;
        server_browser_require_valid_entry = 0;
        break;
    default:
        break;
    }
    return 1;
}

}
