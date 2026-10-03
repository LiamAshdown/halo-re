/**
 * Widget event handlers reachable only through ui_event_function_table. Each handler takes the widget instance,
 * the event record and an out-flag and returns whether the event was consumed.
 */

#include "crt.h"
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

extern "C" {
extern int32_t selected_saved_item;
extern uint8_t saved_item_working_copy[0x1ffc];
extern void network_client_rejoin_check(int8_t machine_player_index);
extern int32_t ui_list_current;
extern growable_array ui_lists[3];
extern int32_t profile_slot_lookup_cache_00692ac8;
extern void widget_play_sound_effect(int16_t effect_id);
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern uint8_t quit_confirm_error_modal;
extern uint8_t quit_confirm_error_is_error;
extern int32_t saved_player_profile_slots_handle;
extern uint8_t saved_game_delete_by_handle(int32_t handle);
extern void player_profile_auto_select(void);
extern char pending_delete_saved_game_name_00718fd0[];
extern void saved_game_allocate_new_slot(uint16_t *out_name);
extern uint32_t saved_game_create_custom_variant(uint32_t unused, uint16_t *name);
extern void saved_item_select(int32_t item);
extern int32_t ui_list_get_id(int32_t index);
extern void *ui_list_get_data(int32_t index);
extern void *game_engine_variant_defaults_classic_slayer(void *out);
extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind);
extern int32_t network_host_edit_field_00719410;
extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory);
extern void saved_game_last_mp_variant_clear(const void *data);
extern uint32_t saved_game_create_default_profile(uint16_t *name);
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id);
extern network_client_globals *network_client;
extern char network_player_entry_validate(void *entry);
extern int32_t network_staged_message_commit(void *client, int16_t value);
extern network_server_globals *network_server;
extern int16_t profile_slot_id[];
extern void display_error(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error);
extern uint32_t time_query_performance_counter_ms(void);
extern char network_session_info_packet_send(const uint32_t *source, void *client);
extern uint8_t local_team_00714dd8[];
extern uint8_t network_disconnect_timeout_flag;
extern uint32_t network_server_reset_game_stats(void);
extern uint8_t coop_profile_globals_block_00714ddc[0x1ffc];
extern void network_client_globals_dispose(void);
extern void network_game_server_host_dispose(void *host);
extern uint8_t network_server_host_valid;
extern widget_history_node *ui_widget_history[3];
extern heap *widget_memory_pool;
extern uint8_t level_select_flags_0071916b;
extern char level_select_current_path_00719068[0x106];
extern int16_t level_select_frame_00719168;
extern uint8_t main_menu_music_pending;
extern void sound_looping_stop(datum_index looping_definition);
extern uint8_t split_screen_quit_prompt_armed;
extern uint8_t ui_event_byte_0071975b;
extern int32_t movie_playback_abort;
extern uint8_t input_event_queue_active;
}

namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
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
        profile[0x12f] = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 9) {
        profile[0x12e] = (uint8_t)(selection + 1);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        profile[0x130] = (uint8_t)selection;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        profile[0x131] = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        profile[0x132] = (uint8_t)(selection == 0);
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
    network_client_rejoin_check((int8_t)event[1]);
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
    int32_t id = list_item_id(*(int16_t *)&((struct widget_instance *)list)->text);

    if (id != -1) {
        profile_slot_lookup_cache_00692ac8 = ((int32_t *)list->list_items)[id];
        if (profile_slot_lookup_cache_00692ac8 != -1) {
            return 1;
        }
    }
    widget_play_sound_effect(4);
    return 0;
}

/**
 * Slot 75 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a11e0
 */
uint8_t UiEventHandlers::event_4a11e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t id = list_item_id(*(int16_t *)&((struct widget_instance *)widget)->text);
    int32_t item = ((int32_t *)widget->list_items)[id];

    profile_slot_lookup_cache_00692ac8 = item;
    if (item == -1) {
        widget_play_sound_effect(4);
        return 0;
    }
    if ((item & 0x40000000) == 0) {
        return 1;
    }
    widget_play_sound_effect(4);
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

    if ((handle & 0x40000000) != 0 || (handle & 0xf) != 0) {
        return 0;
    }
    current = saved_player_profile_slots_handle;
    if (handle != -1) {
        saved_game_delete_by_handle(handle);
        handle = profile_slot_lookup_cache_00692ac8;
    }
    if (handle == current) {
        player_profile_auto_select();
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
        saved_game_delete_by_handle(handle);
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
    uint8_t scratch[0x100];
    uint32_t handle;

    saved_game_allocate_new_slot(name);
    if (name[0] != 0) {
        handle = saved_game_create_custom_variant((uint32_t)(uint16_t)widget->controller_index, name);
        if (handle != 0xffffffff) {
            saved_item_select((int32_t)handle);
            if ((selected_saved_item & 0xf) == 1) {
                int32_t id = ui_list_get_id(*(int16_t *)&((struct widget_instance *)list)->text);
                const void *source;
                uint8_t opened;

                source = id != -1 ? ui_list_get_data(id) : game_engine_variant_defaults_classic_slayer(scratch);
                memcpy(saved_item_working_copy, source, 0x98);
                *(uint16_t *)(saved_item_working_copy + 0x94) = 0;
                wcsncpy((wchar_t *)saved_item_working_copy, (const wchar_t *)name, 0x17);
                *(uint16_t *)(saved_item_working_copy + 0x2e) = 0;
                *(uint32_t *)(saved_item_working_copy + 0x38) &= 0xfffffe7f;
                opened = virtual_keyboard_open((uint16_t *)saved_item_working_copy, 0x30, 9);
                if (opened == 1) {
                    network_host_edit_field_00719410 = 2;
                    if (saved_game_get_directory_by_handle((int32_t)handle, (char *)scratch) != 0) {
                        saved_game_last_mp_variant_clear(scratch);
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
    widget_play_sound_effect(4);
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

    saved_game_allocate_new_slot(name);
    if (name[0] != 0) {
        handle = saved_game_create_default_profile(name);
        if (handle != 0xffffffff) {
            uint8_t *profile;

            saved_item_select((int32_t)handle);
            profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
            player_profile_load(0, profile, (int32_t)handle);
            if (profile != 0) {
                uint8_t opened;

                wcsncpy((wchar_t *)(profile + 2), (const wchar_t *)name, 0xb);
                *(uint16_t *)(profile + 0x18) = 0;
                opened = virtual_keyboard_open((uint16_t *)(profile + 2), 0x18, 8);
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
    widget_play_sound_effect(4);
    return 0;
}

/**
 * AX value (its C still takes only the client and ignores AX: OPEN, networking phase)
 *
 * @address 0x4a1570
 */
uint8_t UiEventHandlers::event_4a1570(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *client = (uint8_t *)network_client;
    int32_t i;

    if (client == 0) {
        return 1;
    }
    for (i = 0; i < 0x10; i++) {
        uint8_t *entry = client + 0xcb6 + i * 0x20;

        if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == *(int16_t *)client &&
            (int16_t)(int8_t)entry[0x1d] == event[1]) {
            network_staged_message_commit(client, 1);
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
    uint8_t *client = (uint8_t *)network_client;
    int32_t i;

    if (client == 0) {
        return 1;
    }
    for (i = 0; i < 0x10; i++) {
        uint8_t *entry = client + 0xcb6 + i * 0x20;

        if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == *(int16_t *)client &&
            (int16_t)(int8_t)entry[0x1d] == event[1]) {
            network_staged_message_commit(client, 0);
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
    uint8_t *server = (uint8_t *)network_server;

    if (server != 0) {
        *(uint16_t *)(server + 6) |= 1;
        (*(uint8_t **)server)[0xae0] = 1;
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
    if (network_server != 0) {
        ((uint8_t *)network_server)[0x9d5] = 0;
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
        display_error(0x12, -1, 1, 0);
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
    uint8_t *client = (uint8_t *)network_client;
    uint8_t *found = 0;
    int32_t count = 0;
    int16_t key;
    int32_t i;
    int16_t *state;

    if (client == 0) {
        return 1;
    }
    state = (int16_t *)(client + 0xeda);
    if (*state == 1) {
        time_query_performance_counter_ms();
    }
    if (*state != 2) {
        return 1;
    }
    key = network_client != 0 ? *(int16_t *)network_client : -1;
    if (key == -1) {
        return 1;
    }
    for (i = 0; i < 0x10; i++) {
        uint8_t *entry = client + 0xcb6 + i * 0x20;

        if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == key) {
            count++;
            if ((int16_t)(int8_t)entry[0x1d] == event[1]) {
                found = entry;
            }
        }
    }
    if (count <= 0) {
        return 1;
    }
    if (found != 0) {
        network_session_info_packet_send((const uint32_t *)found, client);
        local_team_00714dd8[(int8_t)found[0x1d] * 0x2004] = 0;
    }
    if (count != 1) {
        return 0;
    }
    if (network_server != 0 && network_disconnect_timeout_flag != 1) {
        uint8_t result = (uint8_t)network_server_reset_game_stats();

        local_team_00714dd8[0] = coop_profile_globals_block_00714ddc[0];
        return result;
    }
    network_client_globals_dispose();
    if (network_server != 0) {
        network_game_server_host_dispose(network_server);
        network_server = 0;
        network_server_host_valid = 0;
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
        heap_block *block = (heap_block *)((uint8_t *)node - 0x10);
        uint32_t size = block->size & 0x7fffffff;

        ui_widget_history[controller] = node->next;
        halo::memory::heap_unlink_block(block, widget_memory_pool);
        widget_memory_pool->bytes_allocated -= (int32_t)size;
        widget_memory_pool->allocation_count -= 1;
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
    *(int16_t *)&((struct widget_instance *)widget)->text = widget->selection_index;
    return 1;
}

/**
 * Slot 100 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a1bf0
 */
uint8_t UiEventHandlers::event_4a1bf0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (main_menu_music_pending == 1) {
        datum_index music = halo::cache::tag_lookup(0x6c736e64, (char *)"sound\\music\\title1\\title1");

        if (music != 0xffffffff) {
            halo::sound::sound_looping_stop(music);
        }
        main_menu_music_pending = 0;
    }
    return 1;
}

/**
 * Original UI routine; see docs/original/interface/ui_event_4a1c80.c.txt for the recovery notes.
 *
 * @address 0x4a1c80
 */
uint8_t UiEventHandlers::event_4a1c80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    split_screen_quit_prompt_armed = 0;
    ui_event_byte_0071975b = 1;
    movie_playback_abort = 1;
    return 1;
}

/**
 * Original UI routine; see docs/original/interface/ui_event_4a1ca0.c.txt for the recovery notes.
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
        halo::input::input_queue_push_event(0, &queued);
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
        halo::input::input_queue_push_event(0, &queued);
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
        halo::input::input_queue_push_event(0, &queued);
    }
    return 1;
}

}
