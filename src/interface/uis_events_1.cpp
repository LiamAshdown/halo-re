/**
 * Widget event handlers reachable only through ui_event_function_table. Each handler takes the widget instance,
 * the event record and an out-flag and returns whether the event was consumed.
 */

#include "crt.h"
#include "halo/interface/engine_state.hpp"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

#include "halo/interface/uis_event_handlers.hpp"

extern "C" {
extern uint8_t level_select_entries[0x50];
extern void ui_list_free_all(void);
extern int16_t pending_difficulty;
extern void widget_play_sound_effect(int16_t effect_id);
extern int16_t local_player_count;
extern uint8_t save_in_progress_00719010;
extern uint32_t ui_start_campaign_from_level_one(void *widget, int16_t *event);
extern uint8_t network_join_error_reason;
extern uint8_t main_globals_byte_0071974f;
extern uint16_t split_screen_quit_prompt_string;
extern uint8_t main_globals_byte_0071973a;
extern uint8_t split_screen_quit_prompt_armed;
extern void network_client_globals_dispose(void);
extern network_server_globals *network_server;
extern void network_game_server_host_dispose(void *host);
extern uint8_t network_server_host_valid;
extern uint8_t local_team_00714dd8;
extern uint8_t coop_profile_globals_block_00714ddc[0x1ffc];
extern void network_game_setup_teardown(void);
extern void *network_session_create(void);
extern network_client_globals *network_client;
extern int16_t network_game_mode;
extern uint8_t network_host_handoff_requested;
extern uint8_t network_disconnect_timeout_flag;
extern uint32_t game_engine_ensure_variant_history_has_entry(void);
extern int32_t network_game_server_host_create(void);
extern int32_t game_variant_history_current;
extern void game_engine_apply_current_custom_variant(void);
extern void game_engine_sync_variant_defaults(void);
extern uint8_t main_menu_music_pending;
extern int32_t selected_saved_item;
extern void main_menu_play_title_music(void);
extern void network_dispatch_initialize(void);
extern uint8_t *map_list;
extern int32_t map_list_count;
extern uint8_t saved_game_last_mp_map_read(uint8_t *out_data);
extern growable_array ui_lists[3];
extern int32_t ui_list_current;
extern uint8_t ui_list_has_default;
extern void map_list_get_friendly_level_name(wchar_t *destination, char *map_path, int32_t destination_capacity);
extern uint32_t growable_array_add_element(growable_array *array);
extern int32_t profile_slot_lookup_cache_00692ac8;
extern uint8_t variant_carousel_slots[0x1d4];
extern heap *widget_memory_pool;
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self);
extern uint8_t playlist_profiles_need_defaults;
extern void playlist_profile_create_default_profiles_on_disk(void);
extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only, uint16_t *capacity_and_count);
extern uint8_t saved_game_last_mp_variant_read(uint8_t *out_data);
extern int32_t saved_game_find_by_name(char *name, int16_t type);
extern uint8_t saved_game_get_variant(int32_t handle, void *out);
extern void ui_list_add_entry(int32_t group_index, const uint16_t *name, int32_t id, const void *data_blob, uint32_t data_size, uint8_t is_default);
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern uint8_t quit_confirm_error_modal;
extern uint8_t quit_confirm_error_is_error;
extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory);
extern void saved_game_last_mp_variant_clear(const void *data);
extern uint8_t game_variant_saved_default[0x98];
extern uint8_t game_variant_saved_default_valid;
extern void widget_close_all(void);
extern void game_engine_begin_end_game_sequence(void);
extern void saved_item_select(int32_t item);
extern uint8_t saved_item_working_copy[0x1ffc];
extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind);
extern int32_t network_host_edit_field_00719410;
extern widget_history_node *ui_widget_history[3];
extern void heap_unlink_block(heap_block *block, heap *self);
}

#ifdef interface
#undef interface
#endif

namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static int32_t list_item_id(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].id;
    }
    return -1;
}

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
static void widget_history_pop(int16_t controller)
{
    widget_history_node *node;

    if (controller == -1) {
        controller = 0;
    }
    node = ui_widget_history[controller];
    if (node != 0) {
        heap_block *block = (heap_block *)((uint8_t *)node - 0x10);
        uint32_t size = block->size & 0x7fffffff;

        ui_widget_history[controller] = node->next;
        heap_unlink_block(block, widget_memory_pool);
        widget_memory_pool->bytes_allocated -= (int32_t)size;
        widget_memory_pool->allocation_count -= 1;
    }
}

}

/**
 * Slot 7 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49cdd0
 */
uint8_t UiEventHandlers::event_49cdd0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    memset(level_select_entries, 0, 0x50);
    widget->list_items = 0;
    widget->item_count = 0;
    ui_list_free_all();
    return 1;
}

/**
 * Slot 9 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49cfa0
 */
uint8_t UiEventHandlers::event_49cfa0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int16_t selection = *(int16_t *)((uint8_t *)widget->parent->parent + 0x3c);

    if (selection < 4) {
        if (selection >= 0) {
            pending_difficulty = selection;
        }
        widget_play_sound_effect(2);
    }
    return 1;
}

/**
 * Slot 104 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d0d0
 */
uint8_t UiEventHandlers::event_49d0d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    local_player_count = 1;
    save_in_progress_00719010 = 1;
    ui_start_campaign_from_level_one(widget, event);
    return 1;
}

/**
 * Slot 11 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d100
 */
uint8_t UiEventHandlers::event_49d100(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_join_error_reason = 0;
    main_globals_byte_0071974f = 0;
    split_screen_quit_prompt_string = 0xffff;
    main_globals_byte_0071973a = 1;
    return 1;
}

/**
 * Slot 12 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d120
 */
uint8_t UiEventHandlers::event_49d120(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_join_error_reason = 0;
    main_globals_byte_0071974f = 0;
    split_screen_quit_prompt_string = 0xffff;
    halo::interface::state::round_reset_pending = 1;
    return 1;
}

/**
 * Slot 13 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d140
 */
uint8_t UiEventHandlers::event_49d140(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    split_screen_quit_prompt_string = 0xffff;
    network_join_error_reason = 0;
    split_screen_quit_prompt_armed = 1;
    return 1;
}

/**
 * Slot 14 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d160
 */
uint8_t UiEventHandlers::event_49d160(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_client_globals_dispose();
    if (network_server != 0) {
        network_game_server_host_dispose(network_server);
        network_server = 0;
        network_server_host_valid = 0;
    }
    local_team_00714dd8 = 0;
    coop_profile_globals_block_00714ddc[0] = 0;
    network_game_setup_teardown();
    return 1;
}

/**
 * Slot 15 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d1a0
 */
uint8_t UiEventHandlers::event_49d1a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_game_setup_teardown();
    return 1;
}

/**
 * Slot 16 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d1b0
 */
uint8_t UiEventHandlers::event_49d1b0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_client_globals_dispose();
    if (network_server != 0) {
        network_game_server_host_dispose(network_server);
        network_server = 0;
        network_server_host_valid = 0;
    }
    network_game_setup_teardown();
    network_client = (network_client_globals *)network_session_create();
    if (network_client == 0) {
        return 0;
    }
    network_game_mode = 1;
    network_host_handoff_requested = 0;
    return 1;
}

/**
 * Slot 18 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d440
 */
uint8_t UiEventHandlers::event_49d440(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget->list_items = 0;
    widget->item_count = 0;
    return 1;
}

/**
 * Slot 19 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d450
 */
uint8_t UiEventHandlers::event_49d450(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (network_server != 0) {
        network_game_server_host_dispose(network_server);
        network_server = 0;
        network_server_host_valid = 0;
    }
    network_client_globals_dispose();
    network_game_setup_teardown();
    return 1;
}

/**
 * Slot 21 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d480
 */
uint8_t UiEventHandlers::event_49d480(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t ok = 1;

    network_disconnect_timeout_flag = 0;
    if (network_server == 0) {
        game_engine_ensure_variant_history_has_entry();
        ok = (uint8_t)network_game_server_host_create();
        if (ok == 1) {
            game_variant_history_current = -1;
            game_engine_apply_current_custom_variant();
            game_engine_sync_variant_defaults();
            network_game_mode = 2;
        }
    }
    if (ok != 0 && network_client == 0) {
        network_client = (network_client_globals *)network_session_create();
        if (network_client != 0) {
            network_host_handoff_requested = 0;
        }
        ok = (uint8_t)(network_client != 0);
    }
    if (ok == 0) {
        if (network_server != 0) {
            network_game_server_host_dispose(network_server);
            network_server = 0;
            network_server_host_valid = 0;
        }
        network_client_globals_dispose();
        network_game_setup_teardown();
    }
    return ok;
}

/**
 * Slot 22 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d520
 */
uint8_t UiEventHandlers::event_49d520(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    local_player_count = 1;
    save_in_progress_00719010 = 1;
    return 1;
}

/**
 * Slot 23 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d540
 */
uint8_t UiEventHandlers::event_49d540(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t music_pending;

    local_team_00714dd8 = 0;
    coop_profile_globals_block_00714ddc[0] = 0;
    network_game_setup_teardown();
    network_client_globals_dispose();
    if (network_server != 0) {
        network_game_server_host_dispose(network_server);
        network_server = 0;
        network_server_host_valid = 0;
    }
    music_pending = main_menu_music_pending;
    network_disconnect_timeout_flag = 0;
    network_game_mode = 0;
    save_in_progress_00719010 = 0;
    local_player_count = 1;
    selected_saved_item = -1;
    if (music_pending == 0) {
        main_menu_play_title_music();
    }
    return 1;
}

/**
 * Slot 24 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d5b0
 */
uint8_t UiEventHandlers::event_49d5b0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    local_player_count = 1;
    save_in_progress_00719010 = 0;
    network_dispatch_initialize();
    return 1;
}

/**
 * Slot 25 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d5d0
 */
uint8_t UiEventHandlers::event_49d5d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    game_engine_ensure_variant_history_has_entry();
    game_engine_apply_current_custom_variant();
    network_server_host_valid = 1;
    return 1;
}

/**
 * Slot 26 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d5f0
 */
uint8_t UiEventHandlers::event_49d5f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t count = map_list_count;
    char last_map[0x104];
    uint16_t name[0x100];
    int32_t i;

    widget->list_items = map_list;
    widget->item_count = (uint16_t)count;
    if (saved_game_last_mp_map_read((uint8_t *)last_map) != 0) {
        widget->selection_index = 0;
        if (count > 0) {
            while (_stricmp(last_map, *(char **)(map_list + widget->selection_index * 0xc)) != 0) {
                widget->selection_index++;
                if (widget->selection_index >= count) {
                    break;
                }
            }
        }
        if (widget->selection_index == count) {
            widget->selection_index = 0;
        }
    }
    *(int16_t *)&((struct widget_instance *)widget)->text = widget->selection_index;
    *(int16_t *)((uint8_t *)widget + 0x3e) = -1;
    for (i = 0; i < 3; i++) {
        ui_lists[i].element_size = 0x10;
        ui_lists[i].count = 0;
        ui_lists[i].data = 0;
    }
    ui_list_current = -1;
    ui_list_has_default = 0;
    for (i = 0; i < count; i++) {
        uint8_t is_default;
        uint32_t index;

        map_list_get_friendly_level_name((wchar_t *)name, *(char **)(map_list + i * 0xc), 0x100);
        is_default = (uint8_t)(i == widget->selection_index);
        index = growable_array_add_element(&ui_lists[0]);
        if (index != 0xffffffff) {
            ui_list_item *item = (ui_list_item *)ui_lists[0].data + index;
            uint16_t *copy;

            item->data = 0;
            copy = (uint16_t *)GlobalAlloc(0, (uint32_t)wcslen((const wchar_t *)name) * 2 + 2);
            item->name = copy;
            item->id = i;
            item->is_default = is_default;
            if (is_default) {
                ui_list_has_default = 1;
            }
            wcscpy((wchar_t *)copy, (const wchar_t *)name);
        }
    }
    return 1;
}

/**
 * Slot 27 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d7a0
 */
uint8_t UiEventHandlers::event_49d7a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget->list_items = 0;
    widget->item_count = 0;
    ui_list_free_all();
    return 1;
}

/**
 * Slot 29 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49d8b0
 */
uint8_t UiEventHandlers::event_49d8b0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t grouped = (uint8_t)(widget->first_child != 0 && widget->first_child->first_child != 0 &&
        widget->first_child->first_child->widget_type == 2);
    int32_t *handles;
    uint16_t count = 0x64;
    int32_t last = -1;
    char last_name[0x100];
    uint32_t variant[0x26];
    int32_t i;

    profile_slot_lookup_cache_00692ac8 = -1;
    memset(variant_carousel_slots, 0xff, sizeof(variant_carousel_slots));
    handles = (int32_t *)heap_reallocate(widget->list_items, 0x190, widget_memory_pool);
    widget->list_items = handles;
    if (handles != 0) {
        if (playlist_profiles_need_defaults == 1) {
            playlist_profile_create_default_profiles_on_disk();
            playlist_profiles_need_defaults = 0;
        }
        saved_game_enumerate_by_type(1, handles, 1, &count);
        for (; count < 3; count++) {
            handles[count] = -1;
        }
        widget->item_count = count;
        for (i = 0; i < 3; i++) {
            ui_lists[i].element_size = 0x10;
            ui_lists[i].count = 0;
            ui_lists[i].data = 0;
        }
        ui_list_current = -1;
        ui_list_has_default = 0;
        if (saved_game_last_mp_variant_read((uint8_t *)last_name) != 0) {
            last = saved_game_find_by_name(last_name, 1);
            if (last != -1) {
                uint16_t row;

                for (row = 0; row < count; row++) {
                    if (handles[row] == last) {
                        widget->selection_index = (int16_t)row;
                        break;
                    }
                }
            }
        }
        for (i = 0; i < count; i++) {
            if (handles[i] == -1) {
                game_engine_apply_current_custom_variant();
            } else if (saved_game_get_variant(handles[i], variant) != 0) {
                int32_t group = 0;

                if (grouped) {
                    uint32_t flags = variant[0x38 / 4];

                    group = (flags & 0x100) != 0 ? 0 : (flags & 0x80) != 0 ? 1 : 2;
                }
                ui_list_add_entry(group, (const uint16_t *)variant, i, variant, 0x98, (uint8_t)(last == handles[i]));
            }
        }
    }
    *(int16_t *)&((struct widget_instance *)widget)->text = widget->selection_index;
    *(int16_t *)((uint8_t *)widget + 0x3e) = -1;
    return 1;
}

/**
 * Slot 31 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49dab0
 */
uint8_t UiEventHandlers::event_49dab0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint32_t variant[0x26];
    char directory[0x100];
    int32_t id = list_item_id(*(int16_t *)&((struct widget_instance *)widget)->text);
    int32_t item = ((int32_t *)widget->list_items)[id];

    if (item == -1 || item >= 0) {
        if (item != -1 && quit_confirm_error_string_index == -1) {
            quit_confirm_error_string_index = 0x1f;
            quit_confirm_error_unknown_ae = -1;
            quit_confirm_error_modal = 1;
            quit_confirm_error_is_error = 0;
        }
        widget_play_sound_effect(4);
        return 0;
    }
    if (saved_game_get_variant(item, variant) == 0) {
        return 0;
    }
    if (saved_game_get_directory_by_handle(item, directory) != 0) {
        saved_game_last_mp_variant_clear(directory);
    }
    memcpy(game_variant_saved_default, variant, sizeof(variant));
    game_variant_saved_default_valid = 1;
    if (network_game_mode != 2) {
        return 1;
    }
    game_engine_ensure_variant_history_has_entry();
    widget_close_all();
    game_engine_begin_end_game_sequence();
    return 0;
}

/**
 * Slot 38 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49e170
 */
uint8_t UiEventHandlers::event_49e170(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t id = list_item_id(*(int16_t *)&((struct widget_instance *)widget)->text);
    int32_t item;

    profile_slot_lookup_cache_00692ac8 = -1;
    item = ((int32_t *)widget->list_items)[id];
    if (item != -1) {
        if (item < 0) {
            saved_item_select(item);
            return 1;
        }
        if (quit_confirm_error_string_index == -1) {
            quit_confirm_error_string_index = 0x1f;
            quit_confirm_error_unknown_ae = -1;
            quit_confirm_error_modal = 1;
            quit_confirm_error_is_error = 0;
        }
    }
    widget_play_sound_effect(4);
    return 0;
}

/**
 * Slot 39 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49e210
 */
uint8_t UiEventHandlers::event_49e210(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    profile_slot_lookup_cache_00692ac8 = -1;
    selected_saved_item = -1;
    return 1;
}

/**
 * Slot 40 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49e220
 */
uint8_t UiEventHandlers::event_49e220(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    int32_t type;

    if (variant == 0) {
        return 0;
    }
    switch (widget->parent->selection_index) {
    case 0:
        type = 1;
        ((struct game_variant *)variant)->teams = 1;
        break;
    case 1:
        type = 4;
        break;
    case 2:
        type = 2;
        break;
    case 3:
        type = 3;
        break;
    case 4:
        type = 5;
        break;
    default:
        type = ((struct game_variant *)variant)->game_engine_index;
        break;
    }
    if (type != ((struct game_variant *)variant)->game_engine_index) {
        memset(variant + 0x7c, 0, 0x18);
    }
    ((struct game_variant *)variant)->game_engine_index = type;
    return 1;
}

/**
 * Slot 41 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49e2c0
 */
uint8_t UiEventHandlers::event_49e2c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;

    if (variant == 0) {
        return 0;
    }
    if (virtual_keyboard_open((uint16_t *)variant, 0x30, 9) != 0) {
        network_host_edit_field_00719410 = 2;
    }
    return 1;
}

/**
 * Slot 42 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49e300
 */
uint8_t UiEventHandlers::event_49e300(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    static const int32_t delays[] = {0, 0x708, 0xe10, 0x1518, 0x2328, 0x4650};
    static const int32_t lives[] = {1, 3, 5, 10, 15};
    static const int32_t times[] = {0, 0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68};
    widget_instance *parent = widget->parent->parent;
    widget_instance *group;
    int16_t selection;

    if (variant == 0) {
        return 1;
    }
    group = parent->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->engine.ctf.assault = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 5) {
        ((struct game_variant *)variant)->engine.ctf.single_flag_time = delays[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->engine.ctf.flag_must_reset = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->engine.ctf.flag_at_home_to_score = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 4) {
        ((struct game_variant *)variant)->score_limit = lives[selection];
    }
    selection = first_list_child(group->next_sibling)->selection_index;
    if (selection >= 0 && selection <= 6) {
        ((struct game_variant *)variant)->time_limit = times[selection];
    }
    widget_history_pop(parent->controller_index);
    return 1;
}

/**
 * Slot 43 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49e5d0
 */
uint8_t UiEventHandlers::event_49e5d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    static const int32_t lives[] = {1, 2, 5, 10, 15};
    static const int32_t times[] = {0, 0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68};
    widget_instance *parent = widget->parent->parent;
    widget_instance *group;
    int16_t selection;

    if (variant == 0) {
        return 0;
    }
    group = parent->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->engine.king.moving_hill = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 4) {
        ((struct game_variant *)variant)->score_limit = lives[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->teams = (uint8_t)(selection == 0);
    }
    selection = first_list_child(group->next_sibling)->selection_index;
    if (selection >= 0 && selection <= 6) {
        ((struct game_variant *)variant)->time_limit = times[selection];
    }
    widget_history_pop(parent->controller_index);
    return 1;
}

}
