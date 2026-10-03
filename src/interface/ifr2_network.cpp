#include "halo/interface/ifr2_network.hpp"
#include "crt.h"
#include <string.h>

#ifdef interface
#undef interface
#endif

extern "C" {
extern network_server_globals *network_server;
extern uint8_t network_disconnect_timeout_flag;
extern int32_t game_variant_history_current;
extern int16_t network_game_mode;
extern network_client_globals *network_client;
extern uint8_t network_host_handoff_requested;
extern uint8_t network_server_host_valid;
extern void game_engine_sync_variant_defaults(void);
extern uint8_t game_engine_ensure_variant_history_has_entry(void);
extern void game_engine_apply_current_custom_variant(void);
extern void network_game_setup_teardown(void);
extern network_client_globals *network_session_create(void);
extern uint8_t network_game_server_host_create(void);
extern void network_client_globals_dispose(void);
extern void network_game_server_host_dispose(network_server_globals *server);
extern variant_carousel_slot variant_carousel_slots[3];
extern uint8_t profile_globals_block[0x60a4];
extern void widget_list_scroll_window(int32_t out[3], widget_instance *widget);
extern void ui_variant_carousel_slot_cache_populate(int32_t *candidate_ids, int32_t count);
extern void multiplayer_settings_select_list_update_item(widget_instance *widget, const uint16_t *record);
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source);
extern tag_instance *tag_instances;
extern heap *widget_memory_pool;
extern uint16_t missing_string_text[];
extern datum_index tag_lookup(tag_group group, char *path);
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self);
extern uint16_t *text_string_list_get_string(void);
extern uint8_t default_profile_data[0x1ffc];
extern char k_empty_string[];
extern uint8_t command_line_check_flag(const char *flag, const char **out_value);
extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dest, int32_t dest_bytes, const char *source);
extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only, uint16_t *capacity_and_count);
extern uint8_t player_profile_get(int32_t slot, void *out_profile);
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id);
extern uint8_t network_game_client_connect_to_address_async(const char *address, const char *password);
extern uint8_t local_team_00714dd8;
extern int32_t time_query_performance_counter_ms(void);
extern void network_game_settings_ack_send(void *client, int32_t unknown);
extern uint8_t network_player_entry_validate(void);
extern uint8_t game_variant_saved_default_valid;
extern game_engine_definition *current_game_engine;
extern uint8_t player_profile_cache_initialized;
extern player_profile player_profile_cache[16];
extern game_variant game_engine_active_variant;
extern game_engine_state game_engine_state_value;
extern void game_engine_player_profile_cache_sync_all(int32_t unknown);
extern void game_engine_reset_round_objects(void);
extern void game_engine_send_round_reset_message(void);
extern void chimera__console_out(ColorARGB *color, char *format, ...);
extern void widget_close_all(void);
}

namespace halo::interface {

/**
 * Tears down any prior host state, creates a new hosted game engine, applies the active custom game variant,
 * and opens the network session; on any failure, disposes whatever was partially created and restores the
 * torn-down state.
 *
 * @address 0x49d210
 */
uint8_t NetworkSetup::host_session_start()
{
    uint8_t ok = 1;

    network_client_globals_dispose();
    network_game_setup_teardown();
    network_disconnect_timeout_flag = 1;

    if (network_server == (network_server_globals *)0) {
        game_engine_ensure_variant_history_has_entry();
        ok = network_game_server_host_create();
        if (ok == 1) {
            int32_t *raw = (int32_t *)network_server;

            raw[0x272] = 0;
            raw[0x273] = 0;
            raw[0x274] = 0;
            raw[0x275] = 0;
            *((uint8_t *)raw + 0x9d5) = 1;
            game_variant_history_current = -1;
            game_engine_apply_current_custom_variant();
            game_engine_sync_variant_defaults();
            network_game_mode = 2;
        }
        if (ok == 0) {
            goto fail;
        }
    }

    if (network_client == (network_client_globals *)0) {
        network_client = network_session_create();
        ok = (network_client != (network_client_globals *)0);
        if (ok) {
            network_host_handoff_requested = 0;
        }
    }
    if (ok != 0) {
        return ok;
    }

fail:
    if (network_server != (network_server_globals *)0) {
        network_game_server_host_dispose(network_server);
        network_server = (network_server_globals *)0;
        network_server_host_valid = 0;
    }
    network_client_globals_dispose();
    network_disconnect_timeout_flag = 0;
    network_game_setup_teardown();
    return 0;
}

/**
 * @address 0x4a5ff0
 */
void MenuListView::refresh_3wide()
{
    int32_t window[3];
    int32_t ids[3];
    int32_t i;

    widget_list_scroll_window(window, widget);
    for (i = 0; i < 3; i++) {
        ids[i] = window[i] == -1 ? -1 : ((int32_t *)widget->list_items)[window[i]];
    }
    ui_variant_carousel_slot_cache_populate(ids, 3);

    for (i = 0; i < 3 && window[i] != -1; i++) {
        widget_instance *row = widget->first_child;
        int32_t id = ((int32_t *)widget->list_items)[window[i]];
        int32_t depth;
        int32_t slot;

        for (depth = 0; depth < i && row != 0; depth++) {
            row = row->next_sibling;
        }
        for (slot = 0; slot < 3; slot++) {
            if (variant_carousel_slots[slot].id == id) {
                multiplayer_settings_select_list_update_item(row, (const uint16_t *)variant_carousel_slots[slot].unknown);
                break;
            }
        }
    }

    if (widget->extended_description != 0) {
        uint8_t profile_copy[0x7ff * 4];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
        set_profile_name(widget->extended_description, (const uint16_t *)(profile_copy + 2));
    }
}

/**
 * blam-cc: both recognized stack parameters
 *
 * @address 0x4a5bc0
 */
void MenuListView::update_item(const uint16_t *record)
{
    datum_index variant_strings_tag =
        tag_lookup(0x75737472  , (char *)"ui\\shell\\strings\\game_variant_descriptions");
    widget_instance *name_widget = widget->first_child;
    widget_instance *desc_widget = name_widget->next_sibling;
    widget_instance *icon_widget = desc_widget->next_sibling;

    icon_widget->hidden = 0;

    if (record == (const uint16_t *)0) {
        uint16_t *name_buf = (uint16_t *)heap_reallocate(name_widget->text, 0x100, widget_memory_pool);

        name_widget->text = name_buf;
        if (name_buf != (uint16_t *)0) {
            name_buf[0] = 0;
        }
        desc_widget->background_bitmap_frame = 5;

        {
            uint16_t *desc_buf = (uint16_t *)heap_reallocate(desc_widget->text, 0x200, widget_memory_pool);

            desc_widget->text = desc_buf;
            if (desc_buf != (uint16_t *)0) {
                datum_index labels_tag = tag_lookup(
                    0x75737472  ,
                    (char *)"ui\\shell\\main_menu\\player_profiles_select\\profile_description_labels");

                desc_buf[0] = 0;
                if (labels_tag != (datum_index)-1) {
                    UnicodeStringList *list = (UnicodeStringList *)tag_instances[labels_tag & 0xffff].data;
                    const uint16_t *source = missing_string_text;

                    if (list->strings.count > 5) {
                        UnicodeStringListString *strings = (UnicodeStringListString *)list->strings.pointer;
                        uint32_t size;

                        size = *(const uint32_t *)((const uint8_t *)strings + 100);
                        if ((int32_t)size > 0) {
                            source = *(uint16_t **)((const uint8_t *)strings + 0x70);
                            *(uint16_t *)((uint8_t *)source + ((size & 0xfffffffe) - 2)) = 0;
                        }
                    }
                    wcsncpy((wchar_t *)desc_buf, (const wchar_t *)source, 0xff);
                    desc_buf[0xff] = 0;
                }
            }
        }
        return;
    }

    {
        uint16_t *name_buf = (uint16_t *)heap_reallocate(name_widget->text, 0x100, widget_memory_pool);

        name_widget->text = name_buf;
        if (name_buf != (uint16_t *)0) {
            wcsncpy((wchar_t *)name_buf, (const wchar_t *)record, 0x7f);
            name_buf[0x7f] = 0;
        }
    }
    desc_widget->background_bitmap_frame = 5;
    {
        uint16_t *desc_buf = (uint16_t *)heap_reallocate(desc_widget->text, 0x200, widget_memory_pool);

        desc_widget->text = desc_buf;
        if (desc_buf != (uint16_t *)0) {
            desc_buf[0] = 0;
        }
    }

    if ((record[0x4a] & 1) != 0) {
        switch (*(const uint32_t *)(record + 0x18)) {
        case 1: desc_widget->background_bitmap_frame = 0; break;
        case 2: desc_widget->background_bitmap_frame = 2; break;
        case 3: desc_widget->background_bitmap_frame = 3; break;
        case 4: desc_widget->background_bitmap_frame = 1; break;
        case 5: desc_widget->background_bitmap_frame = 4; break;
        default: break;
        }
        if (variant_strings_tag != (datum_index)-1 && desc_widget->text != (void *)0) {
            uint16_t *text = text_string_list_get_string();

            wcsncpy((wchar_t *)((uint16_t *)desc_widget->text), (const wchar_t *)text, 0xff);
            ((uint16_t *)desc_widget->text)[0xff] = 0;
        }
        icon_widget->hidden = 1;
        return;
    }

    switch (*(const uint32_t *)(record + 0x18)) {
    case 1: desc_widget->background_bitmap_frame = 0; break;
    case 2: desc_widget->background_bitmap_frame = 2; break;
    case 3: desc_widget->background_bitmap_frame = 3; break;
    case 4: desc_widget->background_bitmap_frame = 1; break;
    case 5: desc_widget->background_bitmap_frame = 4; break;
    default: return;
    }

    if (variant_strings_tag != (datum_index)-1 && desc_widget->text != (void *)0) {
        uint16_t *text = text_string_list_get_string();

        wcsncpy((wchar_t *)((uint16_t *)desc_widget->text), (const wchar_t *)text, 0xff);
        ((uint16_t *)desc_widget->text)[0xff] = 0;
    }
}

/**
 * @address 0x4c9c80
 */
uint8_t NetworkSetup::autojoin_from_command_line()
{
    const char *address = 0;
    const char *name = 0;
    const char *password = 0;
    uint16_t wide_name[0x40];
    int32_t slots[100];
    uint8_t profile[0x1ffc];

    if (!command_line_check_flag("-connect", &address) || address == 0) {
        return 0;
    }

    if (command_line_check_flag("-name", &name) && name != 0) {
        int16_t count = 100;

        string_convert_ascii_to_unicode(wide_name, 0x80, name);
        saved_game_enumerate_by_type(0, slots, 0, (uint16_t *)&count);
        while (count > 0) {
            int32_t slot = slots[count - 1];

            if (slot == -1) {
                memcpy(profile, default_profile_data, sizeof(profile));
            } else if (player_profile_get(slot, profile) != 0 &&
                       wcscmp((const wchar_t *)wide_name, (const wchar_t *)((const uint16_t *)(profile + 2))) == 0) {
                player_profile_load(0, profile, slot);
                break;
            }
            count--;
        }
    }

    if (!command_line_check_flag("-password", &password) || password == 0) {
        password = k_empty_string;
    }
    network_game_client_connect_to_address_async(address, password);
    return 1;
}

/**
 * @address 0x4a5ad0
 */
void NetworkSetup::clear_player_ready_flags()
{
    int16_t *client = (int16_t *)network_client;
    int16_t *status;
    int16_t *player;
    uint8_t local_ready_flags[16];
    int32_t i;

    if (client == (int16_t *)0) {
        return;
    }

    status = client + 0x76d;
    if (client[0x76d] == 1) {
        time_query_performance_counter_ms();
    }

    if (*status != 2) {
        return;
    }

    if (network_server == (network_server_globals *)0) {
        player = (client == (int16_t *)0) ? (int16_t *)0 : client + 0x58a;
    } else {
        player = (int16_t *)((uint8_t *)network_server + 8);
    }
    local_ready_flags[0] = local_team_00714dd8;
    player = player + 0xd1;

    for (i = 0x10; i != 0; i--) {
        if (network_player_entry_validate() != 0) {
            if (player == (int16_t *)0 || network_player_entry_validate() == 0) {
                if (network_game_mode != 3 || *((int8_t *)player + 0x1c) == 0) {
                    goto clear_flag;
                }
            } else if ((network_server == (network_server_globals *)0 ||
                        (((*((uint8_t *)network_server + 6) >> 2) & 1) == 0)) &&
                       *client != -1 && *client == (int16_t)*((int8_t *)player + 0x1c)) {
            clear_flag:
                local_ready_flags[*((int8_t *)player + 0x1d)] = 0;
            }
        }
        player = player + 0x10;
    }

    if (local_ready_flags[0] != 0) {
        network_game_settings_ack_send(network_client, 0);
    }
}

/**
 * Clears the "game variant has an unsaved default" flag and the per-controller network-setup discriminant,
 * disposes the current game engine definition if one is installed (calling its dispose vtable slot at +8),
 * resets the player-profile cache if it was initialized, and clears the active game_variant working copy.
 *
 * @address 0x495520
 */
void NetworkSetup::game_setup_teardown()
{
    game_variant_saved_default_valid = 0;
    network_game_mode = 0;

    if (current_game_engine != (game_engine_definition *)0) {
        if (current_game_engine->dispose != (void *)0) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = (game_engine_definition *)0;
    }

    if (player_profile_cache_initialized == 1) {
        memset(player_profile_cache, 0, sizeof(player_profile_cache));
        player_profile_cache_initialized = 0;
    }

    memset(&game_engine_active_variant, 0, sizeof(game_engine_active_variant));
}

/**
 * UNSURE: offsets 0x9c8..0x9d5 fall past types/networking.h's own documented fields of network_server_globals
 * (size 0xa10, so still in range but not individually named there).
 *
 * @address 0x4a1670
 */
uint32_t NetworkSetup::server_reset_game_stats()
{
    if (network_server != (network_server_globals *)0) {
        uint8_t *raw = (uint8_t *)network_server;

        *(int32_t *)(raw + 0x9c8) = 0;
        *(int32_t *)(raw + 0x9cc) = 0;
        *(int32_t *)(raw + 0x9d0) = 0;
        *(int32_t *)(raw + 0x9d4) = 0;
        raw[0x9d5] = 1;
    }
    return ((uint32_t)network_server << 8) | 1;
}

/**
 * If `widget` is the dialog's first choice button, closes all widgets and, if hosting and the game has not
 * ended, restarts the current map (else logs a console message); returns 1 for either recognized choice
 * button, 0 otherwise.
 *
 * @address 0x4a30e0
 */
uint32_t MenuListView::choice_handler()
{
    widget_instance *first_choice = widget->parent->first_child->next_sibling;
    uint32_t handled = 0;

    if (widget == first_choice) {
        widget_close_all();
        if (network_game_mode == 2) {
            if (game_engine_state_value == 0) {
                game_engine_reset_round_objects();
                game_engine_send_round_reset_message();
                game_engine_player_profile_cache_sync_all(-1);
            } else {
                chimera__console_out((ColorARGB *)0, (char *)"Cannot restart the map when the game is over.");
            }
        }
        handled = 1;
    }
    if (widget == first_choice->next_sibling) {
        handled = 1;
    }
    return handled;
}

} // namespace halo::interface

extern "C" {

uint8_t multiplayer_host_session_start(void)
{
    return halo::interface::NetworkSetup::host_session_start();
}

void multiplayer_settings_select_list_refresh_3wide(widget_instance *widget)
{
    halo::interface::MenuListView(widget).refresh_3wide();
}

void multiplayer_settings_select_list_update_item(widget_instance *widget, const uint16_t *record)
{
    halo::interface::MenuListView(widget).update_item(record);
}

uint8_t network_autojoin_from_command_line(void)
{
    return halo::interface::NetworkSetup::autojoin_from_command_line();
}

void network_clear_player_ready_flags(void)
{
    halo::interface::NetworkSetup::clear_player_ready_flags();
}

void network_game_setup_teardown(void)
{
    halo::interface::NetworkSetup::game_setup_teardown();
}

uint32_t network_server_reset_game_stats(void)
{
    return halo::interface::NetworkSetup::server_reset_game_stats();
}

uint32_t restart_map_dialog_choice_handler(widget_instance *widget)
{
    return halo::interface::MenuListView(widget).choice_handler();
}

}
