/**
 * Widget event handlers reachable only through ui_event_function_table. Each handler takes the widget instance,
 * the event record and an out-flag and returns whether the event was consumed.
 */

#include "crt.h"
#include "halo/interface/constants.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/interface/engine_state.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include <string.h>
#include <wchar.h>
#include "game.h"
#include "networking.h"

#include "halo/interface/uis_event_handlers.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "saved_games.h"
#include "halo/interface/records.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

static auto &selected_saved_item = halo::link::ref<int32_t>(halo::ui::vars().selected_saved_item);
static auto &saved_item_working_copy = halo::link::ref<uint8_t [k_saved_player_profile_size]>(halo::ui::vars().saved_item_working_copy);
static auto &ui_list_current = halo::link::ref<int32_t>(halo::ui::vars().ui_list_current);
static auto &ui_lists = halo::link::ref<growable_array [3]>(halo::ui::vars().ui_lists);
static auto &profile_slot_lookup_cache_00692ac8 = halo::link::ref<int32_t>(halo::ui::vars().profile_slot_lookup_cache_00692ac8);
static auto &quit_confirm_error_string_index = halo::link::ref<int16_t>(halo::ui::vars().quit_confirm_error_string_index);
static auto &quit_confirm_error_unknown_ae = halo::link::ref<int16_t>(halo::ui::vars().quit_confirm_error_unknown_ae);
static auto &quit_confirm_error_modal = halo::link::ref<uint8_t>(halo::ui::vars().quit_confirm_error_modal);
static auto &quit_confirm_error_is_error = halo::link::ref<uint8_t>(halo::ui::vars().quit_confirm_error_is_error);
static auto &pending_delete_saved_game_name_00718fd0 = halo::link::ref<char []>(halo::ui::vars().pending_delete_saved_game_name_00718fd0);
static auto &network_host_edit_field_00719410 = halo::link::ref<int32_t>(halo::ui::vars().network_host_edit_field_00719410);
static auto &profile_slot_id = halo::link::ref<int16_t []>(halo::ui::vars().profile_slot_id);
static auto &local_team_00714dd8 = halo::link::ref<uint8_t []>(halo::ui::vars().local_team_00714dd8);
static auto &coop_profile_globals_block_00714ddc = halo::link::ref<uint8_t [k_saved_player_profile_size]>(halo::ui::vars().coop_profile_globals_block_00714ddc);
static auto &ui_widget_history = halo::link::ref<widget_history_node * [3]>(halo::ui::vars().ui_widget_history);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &level_select_flags_0071916b = halo::link::ref<uint8_t>(halo::ui::vars().level_select_flags_0071916b);
static auto &level_select_current_path_00719068 = halo::link::ref<char [halo::interface::k_level_select_path_chars]>(halo::ui::vars().level_select_current_path_00719068);
static auto &level_select_frame_00719168 = halo::link::ref<int16_t>(halo::ui::vars().level_select_frame_00719168);
static auto &split_screen_quit_prompt_armed = halo::link::ref<uint8_t>(halo::ui::vars().split_screen_quit_prompt_armed);
static auto &ui_event_byte_0071975b = halo::link::ref<uint8_t>(halo::ui::vars().ui_event_byte_0071975b);
static auto &input_event_queue_active = halo::link::ref<uint8_t>(halo::ui::vars().input_event_queue_active);
#include "halo/interface/widget_pool.hpp"
#include "halo/interface/wide_text.hpp"

namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != uiwidgettype_spinner_list) {
        child = child->next_sibling;
    }
    return child;
}

/** Local helper shared by the handlers of this file. */
static int32_t list_item_id(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].id;
    }
    return -1;
}

}

/**
 * Slot 71 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a0fb0
 */
uint8_t UiEventHandlers::event_4a0fb0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    widget_instance *group;
    int16_t selection;

    if (profile == 0) {
        return 0;
    }
    group = widget->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct saved_player_profile *)profile)->look_inverted = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 9) {
        ((struct saved_player_profile *)profile)->look_sensitivity = (uint8_t)(selection + 1);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct saved_player_profile *)profile)->unknown_130 = (uint8_t)selection;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct saved_player_profile *)profile)->look_inverted_driving = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct saved_player_profile *)profile)->auto_center_look = (uint8_t)(selection == 0);
    }
    return 1;
}

/**
 * Slot 72 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a10f0
 */
uint8_t UiEventHandlers::event_4a10f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    halo::networking::network_client_rejoin_check((int8_t)event[1]);
    return 1;
}

/**
 * Slot 74 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1180
 */
uint8_t UiEventHandlers::event_4a1180(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *list = widget->first_child->next_sibling;
    int32_t id = list_item_id(halo::interface::widget_list_committed(list));

    if (id != -1) {
        profile_slot_lookup_cache_00692ac8 = ((int32_t *)list->list_items)[id];
        if (profile_slot_lookup_cache_00692ac8 != -1) {
            return 1;
        }
    }
    halo::interface::widget_play_sound_effect(4);
    return 0;
}

/**
 * Slot 75 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a11e0
 */
uint8_t UiEventHandlers::event_4a11e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t id = list_item_id(halo::interface::widget_list_committed(widget));
    int32_t item = ((int32_t *)widget->list_items)[id];

    profile_slot_lookup_cache_00692ac8 = item;
    if (item == -1) {
        halo::interface::widget_play_sound_effect(4);
        return 0;
    }
    if ((item & halo::interface::k_saved_item_builtin_marker) == 0) {
        return 1;
    }
    halo::interface::widget_play_sound_effect(4);
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x1a;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
    }
    return 0;
}

/**
 * Slot 76 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1280
 */
uint8_t UiEventHandlers::event_4a1280(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t handle = profile_slot_lookup_cache_00692ac8;
    int32_t current;

    if ((handle & halo::interface::k_saved_item_builtin_marker) != 0 || (handle & 0xf) != 0) {
        return 0;
    }
    current = halo::saved_games::globals().player_profile_slots_handle;
    if (handle != -1) {
        halo::saved_games::saved_game_delete_by_handle(handle);
        handle = profile_slot_lookup_cache_00692ac8;
    }
    if (handle == current) {
        halo::interface::player_profile_auto_select();
    }
    return 1;
}

/**
 * Slot 77 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a12c0
 */
uint8_t UiEventHandlers::event_4a12c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t handle = profile_slot_lookup_cache_00692ac8;

    if ((handle & 0xf) != 1) {
        return 0;
    }
    if (handle != -1) {
        halo::saved_games::saved_game_delete_by_handle(handle);
    }
    return 1;
}

/**
 * Slot 78 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a12f0
 */
uint8_t UiEventHandlers::event_4a12f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    profile_slot_lookup_cache_00692ac8 = -1;
    pending_delete_saved_game_name_00718fd0[0] = 0;
    return 1;
}

/**
 * Slot 79 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1310
 */
uint8_t UiEventHandlers::event_4a1310(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *list = widget->parent->parent;
    uint16_t name[0x80];
    uint8_t scratch[halo::interface::k_text_buffer_chars];
    uint32_t handle;

    halo::saved_games::saved_game_allocate_new_slot(name);
    if (name[0] != 0) {
        handle = halo::saved_games::saved_game_create_custom_variant((uint32_t)(uint16_t)widget->controller_index, name);
        if (handle != halo::k_dword_none) {
            halo::interface::saved_item_select((int32_t)handle);
            if ((selected_saved_item & 0xf) == 1) {
                int32_t id = halo::interface::ui_list_get_id(halo::interface::widget_list_committed(list));
                const void *source;
                uint8_t opened;

                source = id != -1 ? halo::interface::ui_list_get_data(id) : halo::game::game_engine_variant_defaults_classic_slayer((game_variant *)scratch);
                memcpy(saved_item_working_copy, source, 0x98);
                *(uint16_t *)(saved_item_working_copy + 0x94) = 0;
                wcsncpy((wchar_t *)saved_item_working_copy, (const wchar_t *)name, 0x17);
                *(uint16_t *)(saved_item_working_copy + 0x2e) = 0;
                ((game_variant *)saved_item_working_copy)->flags &= ~0x180u;
                opened = halo::interface::virtual_keyboard_open((uint16_t *)saved_item_working_copy, 0x30, 9);
                if (opened == 1) {
                    network_host_edit_field_00719410 = 2;
                    if (halo::saved_games::saved_game_get_directory_by_handle((int32_t)handle, (char *)scratch) != 0) {
                        halo::saved_games::saved_game_last_mp_variant_clear(scratch);
                    }
                    return 1;
                }
                if (opened != 0) {
                    return opened;
                }
            } else {
                selected_saved_item = -1;
            }
        }
    }
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x26;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
    }
    halo::interface::widget_play_sound_effect(4);
    return 0;
}

/**
 * Slot 80 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1480
 */
uint8_t UiEventHandlers::event_4a1480(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint16_t name[0x82];
    uint32_t handle;

    halo::saved_games::saved_game_allocate_new_slot(name);
    if (name[0] != 0) {
        handle = halo::saved_games::saved_game_create_default_profile(name);
        if (handle != halo::k_dword_none) {
            uint8_t *profile;

            halo::interface::saved_item_select((int32_t)handle);
            profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
            halo::interface::player_profile_load(0, profile, (int32_t)handle);
            if (profile != 0) {
                uint8_t opened;

                wcsncpy((wchar_t *)(profile + 2), (const wchar_t *)name, 0xb);
                ((struct saved_player_profile *)profile)->name[11] = 0;
                opened = halo::interface::virtual_keyboard_open((uint16_t *)(profile + 2), 0x18, 8);
                if (opened != 0) {
                    return opened;
                }
            } else {
                selected_saved_item = -1;
            }
        }
    }
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x25;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
    }
    halo::interface::widget_play_sound_effect(4);
    return 0;
}

/**
 * AX value (its C still takes only the client and ignores AX: OPEN, networking phase)
 *
 * @address 0x4a1570
 */
uint8_t UiEventHandlers::event_4a1570(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_client_globals *client = halo::networking::globals().client;
    int32_t i;

    if (client == 0) {
        return 1;
    }
    for (i = 0; i < 0x10; i++) {
        network_player_entry *entry = &client->session.players[i];

        if (halo::networking::network_player_entry_validate(entry) != 0 && (int16_t)entry->machine_index == (int16_t)client->machine_index &&
            (int16_t)entry->machine_player_index == event[1]) {
            halo::networking::network_staged_message_commit(client, 1);
            return 1;
        }
    }
    return 1;
}

/**
 * AX value (its C still takes only the client and ignores AX: OPEN, networking phase)
 *
 * @address 0x4a15e0
 */
uint8_t UiEventHandlers::event_4a15e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_client_globals *client = halo::networking::globals().client;
    int32_t i;

    if (client == 0) {
        return 1;
    }
    for (i = 0; i < 0x10; i++) {
        network_player_entry *entry = &client->session.players[i];

        if (halo::networking::network_player_entry_validate(entry) != 0 && (int16_t)entry->machine_index == (int16_t)client->machine_index &&
            (int16_t)entry->machine_player_index == event[1]) {
            halo::networking::network_staged_message_commit(client, 0);
            return 1;
        }
    }
    return 1;
}

/**
 * Slot 83 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1650
 */
uint8_t UiEventHandlers::event_4a1650(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_server_globals *server = halo::networking::globals().server;

    if (server != 0) {
        server->flags |= 1;
        server->listen_channel->listening = 1;
    }
    return 1;
}

/**
 * Slot 85 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a16a0
 */
uint8_t UiEventHandlers::event_4a16a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (halo::networking::globals().server != 0) {
        halo::networking::globals().server->handshake_blocked = 0;
    }
    return 1;
}

/**
 * Slot 86 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a16c0
 */
uint8_t UiEventHandlers::event_4a16c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget->hidden = 1;
    widget->state = 0;
    return 1;
}

/**
 * Slot 88 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a16d0
 */
uint8_t UiEventHandlers::event_4a16d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    profile_slot_id[0] = -1;
    return 1;
}

/**
 * Slot 89 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a16e0
 */
uint8_t UiEventHandlers::event_4a16e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    profile_slot_id[0] = event[1];
    return 1;
}

/**
 * Slot 90 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1700
 */
uint8_t UiEventHandlers::event_4a1700(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (event[1] == profile_slot_id[0]) {
        halo::interface::display_error(0x12, -1, 1, 0);
        *out_handled = 1;
        return 0;
    }
    profile_slot_id[1] = event[1];
    return 1;
}

/**
 * Slot 93 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1790
 */
uint8_t UiEventHandlers::event_4a1790(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_client_globals *client = halo::networking::globals().client;
    network_player_entry *found = 0;
    int32_t count = 0;
    int16_t key;
    int32_t i;
    uint16_t *state;

    if (client == 0) {
        return 1;
    }
    state = &client->state;
    if (*state == 1) {
        halo::cseries::time_query_performance_counter_ms();
    }
    if (*state != 2) {
        return 1;
    }
    key = halo::networking::globals().client != 0 ? (int16_t)client->machine_index : -1;
    if (key == -1) {
        return 1;
    }
    for (i = 0; i < 0x10; i++) {
        network_player_entry *entry = &client->session.players[i];

        if (halo::networking::network_player_entry_validate(entry) != 0 && (int16_t)entry->machine_index == key) {
            count++;
            if ((int16_t)entry->machine_player_index == event[1]) {
                found = entry;
            }
        }
    }
    if (count <= 0) {
        return 1;
    }
    if (found != 0) {
        halo::networking::network_session_info_packet_send((const uint32_t *)found, client);
        local_team_00714dd8[(int8_t)found->machine_player_index * sizeof(saved_player_profile_slot)] = 0;
    }
    if (count != 1) {
        return 0;
    }
    if (halo::networking::globals().server != 0 && halo::networking::globals().disconnect_timeout_flag != 1) {
        uint8_t result = (uint8_t)halo::interface::network_server_reset_game_stats();

        local_team_00714dd8[0] = coop_profile_globals_block_00714ddc[0];
        return result;
    }
    halo::networking::network_client_globals_dispose();
    if (halo::networking::globals().server != 0) {
        halo::networking::network_game_server_host_dispose(halo::networking::globals().server);
        halo::networking::globals().server = 0;
        halo::networking::globals().server_host_valid = 0;
    }
    local_team_00714dd8[0] = coop_profile_globals_block_00714ddc[0];
    return 1;
}

/**
 * Slot 94 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1900
 */
uint8_t UiEventHandlers::event_4a1900(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *root = widget;
    int32_t kind = selected_saved_item & 0xf;

    if (kind == 0 || kind == 1) {
        return 1;
    }
    while (root->parent != 0) {
        root = root->parent;
    }
    root->milliseconds_to_auto_close = 1;
    root->state = 0;
    return 0;
}

/**
 * Slot 98 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1b00
 */
uint8_t UiEventHandlers::event_4a1b00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int16_t controller = widget->controller_index;
    widget_history_node *node;

    if (controller == -1) {
        controller = 0;
    }
    node = ui_widget_history[controller];
    if (node != 0) {
        ui_widget_history[controller] = node->next;
        halo::interface::widget_pool_free(node);
    }
    return 1;
}

/**
 * Slot 99 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1b60
 */
uint8_t UiEventHandlers::event_4a1b60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *child = widget->first_child;
    int16_t selection = 1;
    int32_t i;

    if (level_select_flags_0071916b == 1 && _stricmp(level_select_current_path_00719068, halo::interface::state::current_campaign_level_path) == 0) {
        selection = level_select_frame_00719168;
    }
    for (i = 0; i < selection && child != 0; i++) {
        child = child->next_sibling;
    }
    widget->selection_index = selection;
    widget->focused_child = child;
    halo::interface::widget_list_committed(widget) = widget->selection_index;
    return 1;
}

/**
 * Slot 100 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1bf0
 */
uint8_t UiEventHandlers::event_4a1bf0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (halo::main::globals().menu_music_pending == 1) {
        datum_index music = halo::interface::lookup_tag(halo::fourcc('l', 's', 'n', 'd'), "sound\\music\\title1\\title1");

        if (music != halo::k_dword_none) {
            halo::sound::sound_looping_stop(music);
        }
        halo::main::globals().menu_music_pending = 0;
    }
    return 1;
}

/**
 *
 * @address 0x4a1c80
 */
uint8_t UiEventHandlers::event_4a1c80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    split_screen_quit_prompt_armed = 0;
    ui_event_byte_0071975b = 1;
    halo::main::globals().movie_playback_abort = 1;
    return 1;
}

/**
 *
 * @address 0x4a1ca0
 */
uint8_t UiEventHandlers::event_4a1ca0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (input_event_queue_active) {
        ui_input_event queued;

        memset(&queued, 0, sizeof(queued));
        queued.kind = 3;
        queued.code = 0;
        queued.pressed = 1;
        halo::input::UiEvents::queue_push_event(0, &queued);
    }
    return 1;
}

/**
 * Slot 109 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1cd0
 */
uint8_t UiEventHandlers::event_4a1cd0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (input_event_queue_active) {
        ui_input_event queued;

        memset(&queued, 0, sizeof(queued));
        queued.kind = 3;
        queued.code = 0xd;
        queued.pressed = 1;
        halo::input::UiEvents::queue_push_event(0, &queued);
    }
    return 1;
}

/**
 * Slot 110 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1d00
 */
uint8_t UiEventHandlers::event_4a1d00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (input_event_queue_active) {
        ui_input_event queued;

        memset(&queued, 0, sizeof(queued));
        queued.kind = 3;
        queued.code = 0xa;
        queued.pressed = 1;
        halo::input::UiEvents::queue_push_event(0, &queued);
    }
    return 1;
}

}
