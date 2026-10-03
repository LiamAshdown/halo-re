/**
 * Network game menu behaviour: host setup, adapter details, client connection and wait timeouts.
 */

#include "crt.h"
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

extern "C" {
extern int32_t ui_list_current;
extern growable_array ui_lists[3];
extern uint8_t profile_globals_block[0x60a4];
extern uint8_t ui_list_default_item_format(void *item_buffer, int32_t item_index, void *list_items);
extern void ui_list_widget_rebuild_rows(widget_instance *widget, ui_list_item_format_function format_item);
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source);
extern heap *widget_memory_pool;
extern uint16_t network_host_name_field_00719238[32];
extern uint16_t network_host_subname_007191f0[9];
extern int32_t selected_saved_item;
extern uint8_t saved_item_working_copy[0x1ffc];
extern uint8_t network_game_client_connect_to_address_async(char *name, char *address);
extern void saved_item_select(int32_t profile_index);
extern uint8_t saved_item_has_unsaved_changes(void);
extern uint8_t player_profile_save(void);
extern uint32_t network_game_option_a_00719210;
extern uint32_t network_game_option_b_00719214;
extern void widget_play_sound_effect(int16_t effect_id);
extern uint8_t ui_network_game_options_populate(widget_instance *widget, const uint8_t *options_record);
extern uint16_t network_host_name_00719170[144];
extern int32_t quality_selection_00692b04;
extern int32_t resolution_row_count_table_0065bfb4[5][1];
extern int32_t resolution_selection_00719204;
extern int32_t resolution_index_table_0065bf74[];
extern uint32_t sv_maxplayers_value;
extern uint32_t network_resolved_local_address;
extern uint16_t local_port_006869b6;
extern uint16_t ip_port_format_string_0066a564[];
extern uint8_t network_host_name_flag_00719276;
extern uint8_t ui_network_wait_active;
extern int32_t ui_network_wait_start_time;
extern uint8_t ui_network_wait_timed_out;
extern void *widget_instance_find_root(widget_instance *widget);
extern int32_t widget_get_sibling_index(widget_instance *widget);
extern void chat_close(void);
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection);
extern uint8_t autopatch_status_state_00719234;
extern uint8_t autopatch_status_active_00719235;
extern uint8_t ui_server_option_flag_00692b10;
extern uint8_t network_game_info_packet_flag;
extern uint8_t server_browser_require_valid_entry;
}

namespace halo::ui {

/**
 * Original UI routine; see docs/original/interface/ui_network_adapter_details_refresh.c.txt for the recovery
 * notes.
 *
 * @address 0x4a4650
 */
void UiNetworkMenu::network_adapter_details_refresh(widget_instance *widget)
{
    widget_instance *row;
    widget_instance *c1, *c2, *c3, *c4, *c5, *c6;
    ui_list_item *entry = (ui_list_item *)0;

    ui_list_widget_rebuild_rows(widget, (ui_list_item_format_function)((void *)ui_list_default_item_format));

    {
        uint8_t profile_copy[0x2000];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
        set_profile_name(widget, (const uint16_t *)(profile_copy + 2));
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

    if (entry == (ui_list_item *)0 || entry->data == (void *)0) {
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
        if (c3->text != (void *)0) {
            wcsncpy((wchar_t *)((uint16_t *)c3->text), (const wchar_t *)(blob + 4), 0x1f);
            ((uint16_t *)c3->text)[0x1f] = 0;
        }
    }
}

/**
 * Original UI routine; see docs/original/interface/ui_network_client_connect_and_save.c.txt for the recovery
 * notes.
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

    saved_item_select(halo::saved_games::globals().player_profile_slots_handle);
    {
        uint8_t *record = ((selected_saved_item & 0xf) == 0) ? saved_item_working_copy : (uint8_t *)0;
        wcslen((const wchar_t *)network_host_name_field_00719238);
        wcscpy((wchar_t *)((uint16_t *)(record + 0xfc2)), (const wchar_t *)network_host_name_field_00719238);
    }
    if (saved_item_has_unsaved_changes() != 0) {
        player_profile_save();
        return result;
    }
    selected_saved_item = -1;
    return result;
}

/**
 * Original UI routine; see docs/original/interface/ui_network_game_options_populate.c.txt for the recovery
 * notes.
 *
 * Register convention: ECX -> widget, ESI -> options_record
 *
 * @address 0x4a3960
 */
uint8_t UiNetworkMenu::network_game_options_populate(widget_instance *widget, const uint8_t *options_record)
{
    widget_instance *control;
    uint8_t value;

    if (options_record == (const uint8_t *)0) {
        return 0;
    }

    for (control = widget->first_child->first_child;
         control != (widget_instance *)0 && control->widget_type != 2;
         control = control->next_sibling) {
    }
    value = options_record[0xfc0];
    control->selection_index = (value < 5) ? value : 4;
    network_game_option_a_00719210 = *(const uint16_t *)(options_record + 0x1002);
    network_game_option_b_00719214 = *(const uint16_t *)(options_record + 0x1004);
    return 1;
}

/**
 * Original UI routine; see docs/original/interface/ui_network_game_options_refresh.c.txt for the recovery notes.
 *
 * @address 0x4a3b30
 */
void UiNetworkMenu::network_game_options_refresh(widget_instance *widget, const uint8_t *options_record)
{
    halo::saved_games::player_profile_set_default_server_options((saved_player_profile *)profile_globals_block);
    widget_play_sound_effect(0);
    ui_network_game_options_populate(widget, options_record);
}

/**
 * Original UI routine; see docs/original/interface/ui_network_host_setup_refresh.c.txt for the recovery notes.
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

    buffer = (uint16_t *)halo::memory::heap_reallocate(control->text, 0x80, widget_memory_pool);
    control->text = buffer;
    if (buffer != (uint16_t *)0) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)network_host_name_00719170, 0x3f);
        ((uint16_t *)control->text)[0x3f] = 0;
    }
    if (row->parent->focused_child == row) {
        tab_index = 0;
    }

    row = row->next_sibling;
    control = row->first_child->next_sibling;
    buffer = (uint16_t *)halo::memory::heap_reallocate(control->text, 0x12, widget_memory_pool);
    control->text = buffer;
    if (buffer != (uint16_t *)0) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)network_host_subname_007191f0, 8);
        ((uint16_t *)control->text)[8] = 0;
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
    if (ip_control->text != (void *)0) {
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
        halo::text::string_convert_ascii_to_unicode(reinterpret_cast<uint16_t *>(ip_control->text), 0x40, text);
        halo::text::string_format_wide_va_bounded(
            0x1f - address_length, reinterpret_cast<uint16_t *>(ip_control->text) + address_length,
            reinterpret_cast<const uint16_t *>(ip_port_format_string_0066a564), halo::networking::globals().game_socket_port);
        ((uint16_t *)ip_control->text)[0x1f] = 0;
    }

    if (tab_index == -1) {
        tab_group->first_child->state = 0;
    } else {
        tab_group->first_child->selection_index = (int16_t)tab_index;
        tab_group->first_child->state = 1;
    }

    {
        uint8_t profile_copy[0x2000];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
        set_profile_name(widget, (const uint16_t *)(profile_copy + 2));
    }
}

/**
 * Original UI routine; see docs/original/interface/ui_network_name_fields_refresh.c.txt for the recovery notes.
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

    buffer = (uint16_t *)halo::memory::heap_reallocate(control->text, 0x40, widget_memory_pool);
    control->text = buffer;
    if (buffer != (uint16_t *)0) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)network_host_name_field_00719238, 0x1f);
        ((uint16_t *)control->text)[0x1f] = 0;
    }
    if (row->parent->focused_child == row) {
        tab_index = 0;
    }

    row = row->next_sibling;
    control = row->first_child->next_sibling;
    buffer = (uint16_t *)halo::memory::heap_reallocate(control->text, 0x12, widget_memory_pool);
    control->text = buffer;
    if (buffer != (uint16_t *)0) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)network_host_subname_007191f0, 8);
        ((uint16_t *)control->text)[8] = 0;
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
        uint8_t profile_copy[0x1ffc];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
        set_profile_name(tab_group->first_child->next_sibling, (const uint16_t *)(profile_copy + 2));
    }
}

/**
 * Original UI routine; see docs/original/interface/ui_network_name_fields_reset.c.txt for the recovery notes.
 *
 * @address 0x4a49c0
 */
uint32_t UiNetworkMenu::network_name_fields_reset(void)
{
    uint16_t unused_name_source[2077];

    if (halo::saved_games::globals().player_profile_slots_handle == -1) {
        halo::saved_games::player_profile_set_default_server_options((saved_player_profile *)profile_globals_block);
    } else {
        uint8_t profile_copy[0x2000];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
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
        widget->selection_index < (int16_t)widget->item_count && widget->list_items != (void *)0 &&
        widget->item_count != 0) {
        uint8_t **entries = (uint8_t **)widget->list_items;
        uint8_t *entry = entries[widget->selection_index];

        if (entry[0x12c] == 1) {
            if (*(int16_t *)(entry + 0x12a) == 1 && *(uint32_t *)entry != 0 &&
                *(int16_t *)(entry + 0x12) != 0) {
                uint32_t session_info[9] = {0};
                s_network_address connect_address = {};
                int32_t connected;

                connect_address.ipv4 = *(uint32_t *)entry;
                connect_address.size = *(int16_t *)(entry + 0x10);
                connect_address.port = *(uint16_t *)(entry + 0x12);
                halo::networking::network_debug_fill_canary_buffer(&session_info[5]);
                connected = halo::networking::network_connection_initiate(halo::networking::globals().client, (const uint32_t *)entry,
                                                         session_info, (const uint32_t *)&connect_address);
                if ((uint8_t)connected == 0) {
                    halo::networking::globals().host_handoff_requested = 1;
                    chat_close();
                    return 0;
                }

                {
                    void *page = widget_instance_find_root(widget);
                    datum_index parent_definition = (widget->parent != (widget_instance *)0)
                                                         ? widget->parent->definition
                                                         : (datum_index)-1;
                    int32_t sibling = widget_get_sibling_index(widget);
                    widget_instance *opened;

                    opened = chimera__load_ui_widget(
                        (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen",
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
            widget_play_sound_effect(0);
        }
    }
    return 0;
}

/**
 * Original UI routine; see docs/original/interface/ui_server_type_option_selected.c.txt for the recovery notes.
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
