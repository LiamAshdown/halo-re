#include "halo/interface/ifr2_network.hpp"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/text/api.hpp"
#include "crt.h"
#include <string.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/main/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "saved_games.h"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/interface/constants.hpp"
#include "halo/interface/widget_pool.hpp"
#include "halo/interface/wide_text.hpp"

#ifdef interface
#undef interface
#endif

static auto &game_variant_history_current = halo::link::ref<int32_t>(halo::ui::vars().game_variant_history_current);
static auto &variant_carousel_slots = halo::link::ref<variant_carousel_slot [3]>(halo::ui::vars().variant_carousel_slots);
static auto &profile_globals_block = halo::link::ref<saved_player_profile_slot [k_maximum_local_player_profiles]>(halo::ui::vars().profile_globals_block);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &missing_string_text = halo::link::ref<uint16_t []>(halo::ui::vars().missing_string_text);
static auto &default_profile_data = halo::link::ref<uint8_t [k_saved_player_profile_size]>(halo::ui::vars().default_profile_data);
static auto &k_empty_string = halo::link::ref<char []>(halo::networking::vars().k_empty_string);
static auto &local_team_00714dd8 = halo::link::ref<uint8_t>(halo::ui::vars().local_team_00714dd8);
static auto &player_profile_cache_initialized = halo::link::ref<uint8_t>(halo::game::vars().player_profile_cache_initialized);
static auto &player_profile_cache = halo::link::ref<player_profile [16]>(halo::game::vars().player_profile_cache);
static auto &game_engine_active_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_active_variant);

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

    halo::networking::network_client_globals_dispose();
    halo::interface::network_game_setup_teardown();
    halo::networking::globals().disconnect_timeout_flag = 1;

    if (halo::networking::globals().server == (network_server_globals *)0) {
        halo::game::game_engine_ensure_variant_history_has_entry();
        ok = halo::networking::network_game_server_host_create();
        if (ok == 1) {
            network_server_globals *server = halo::networking::globals().server;

            server->handshake_timer.remaining_ms = 0;
            server->handshake_timer.last_tick_ms = 0;
            server->unknown_9d0 = 0;
            server->handshake_state = 0;
            server->handshake_blocked = 0;
            server->handshake_flag = 0;
            server->pad_9d7 = 0;
            server->handshake_blocked = 1;
            game_variant_history_current = -1;
            halo::game::game_engine_apply_current_custom_variant();
            halo::game::game_engine_sync_variant_defaults();
            halo::networking::globals().game_mode = 2;
        }
    }

    if (ok != 0) {
        if (halo::networking::globals().client == (network_client_globals *)0) {
            halo::networking::globals().client = halo::networking::network_session_create();
            ok = (halo::networking::globals().client != (network_client_globals *)0);
            if (ok) {
                halo::networking::globals().host_handoff_requested = 0;
            }
        }
        if (ok != 0) {
            return ok;
        }
    }

    if (halo::networking::globals().server != (network_server_globals *)0) {
        halo::networking::network_game_server_host_dispose(halo::networking::globals().server);
        halo::networking::globals().server = (network_server_globals *)0;
        halo::networking::globals().server_host_valid = 0;
    }
    halo::networking::network_client_globals_dispose();
    halo::networking::globals().disconnect_timeout_flag = 0;
    halo::interface::network_game_setup_teardown();
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

    halo::interface::widget_list_scroll_window(window, widget);
    for (i = 0; i < 3; i++) {
        ids[i] = window[i] == -1 ? -1 : ((int32_t *)widget->list_items)[window[i]];
    }
    halo::interface::ui_variant_carousel_slot_cache_populate(ids, 3);

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
                halo::interface::multiplayer_settings_select_list_update_item(row, (const uint16_t *)variant_carousel_slots[slot].unknown);
                break;
            }
        }
    }

    if (widget->extended_description != 0) {
        saved_player_profile profile_copy;

        memcpy(&profile_copy, &profile_globals_block[0].profile, sizeof(profile_copy));
        halo::interface::set_profile_name(widget->extended_description, profile_copy.name);
    }
}

/** Fields of a game-variant list record (record is a word pointer: the first is a word index, the others byte offsets). */
static constexpr size_t k_record_variant_word = 0x4a;
static constexpr size_t k_record_game_type_byte = 0x30;
static constexpr size_t k_record_team_play_byte = 0x34;

/**
 * blam-cc: both recognized stack parameters
 *
 * @address 0x4a5bc0
 */
void MenuListView::update_item(const uint16_t *record)
{
    datum_index variant_strings_tag =
        halo::interface::lookup_tag(halo::groups::unicode_string_list, halo::tag_paths::game_variant_descriptions);
    widget_instance *name_widget = widget->first_child;
    widget_instance *desc_widget = name_widget->next_sibling;
    widget_instance *icon_widget = desc_widget->next_sibling;

    icon_widget->hidden = 0;

    if (record == nullptr) {
        uint16_t *name_buf = halo::interface::widget_pool_resize_text(name_widget->text, halo::interface::k_name_text_bytes);

        name_widget->text = name_buf;
        if (name_buf != nullptr) {
            name_buf[0] = 0;
        }
        desc_widget->background_bitmap_frame = 5;

        {
            uint16_t *desc_buf = halo::interface::widget_pool_resize_text(desc_widget->text, halo::interface::k_description_text_bytes);

            desc_widget->text = desc_buf;
            if (desc_buf != nullptr) {
                datum_index labels_tag = halo::interface::lookup_tag(
                    halo::groups::unicode_string_list,
                    halo::tag_paths::profile_description_labels);

                desc_buf[0] = 0;
                if (labels_tag != (datum_index)-1) {
                    UnicodeStringList *list = halo::interface::tag_data<UnicodeStringList>(labels_tag);
                    const uint16_t *source = missing_string_text;

                    if (list->strings.count > 5) {
                        UnicodeStringListString *strings = (UnicodeStringListString *)list->strings.pointer;
                        uint32_t size;

                        size = *(const uint32_t *)((const uint8_t *)strings + 100);
                        if ((int32_t)size > 0) {
                            source = *(uint16_t **)((const uint8_t *)strings + 0x70);
                            *(uint16_t *)((uint8_t *)source + ((size & ~1u) - 2)) = 0;
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
        uint16_t *name_buf = halo::interface::widget_pool_resize_text(name_widget->text, halo::interface::k_name_text_bytes);

        name_widget->text = name_buf;
        if (name_buf != nullptr) {
            wcsncpy((wchar_t *)name_buf, (const wchar_t *)record, 0x7f);
            name_buf[0x7f] = 0;
        }
    }
    desc_widget->background_bitmap_frame = 5;
    {
        uint16_t *desc_buf = halo::interface::widget_pool_resize_text(desc_widget->text, halo::interface::k_description_text_bytes);

        desc_widget->text = desc_buf;
        if (desc_buf != nullptr) {
            desc_buf[0] = 0;
        }
    }

    if ((record[k_record_variant_word] & 1) != 0) {
        switch (*(const uint32_t *)((const uint8_t *)record + k_record_game_type_byte)) {
        case 1: desc_widget->background_bitmap_frame = 0; break;
        case 2: desc_widget->background_bitmap_frame = 2; break;
        case 3: desc_widget->background_bitmap_frame = 3; break;
        case 4: desc_widget->background_bitmap_frame = 1; break;
        case 5: desc_widget->background_bitmap_frame = 4; break;
        default: break;
        }
        if (variant_strings_tag != (datum_index)-1 && desc_widget->text != nullptr) {
            uint16_t *text = halo::text::text_string_list_get_string(variant_strings_tag, (int16_t)((record[k_record_variant_word] >> 8) + 0xa));

            wcsncpy((wchar_t *)(halo::interface::widget_text(desc_widget)), (const wchar_t *)text, 0xff);
            (halo::interface::widget_text(desc_widget))[0xff] = 0;
        }
        icon_widget->hidden = 1;
        return;
    }

    switch (*(const uint32_t *)((const uint8_t *)record + k_record_game_type_byte)) {
    case 1: desc_widget->background_bitmap_frame = 0; break;
    case 2: desc_widget->background_bitmap_frame = 2; break;
    case 3: desc_widget->background_bitmap_frame = 3; break;
    case 4: desc_widget->background_bitmap_frame = 1; break;
    case 5: desc_widget->background_bitmap_frame = 4; break;
    default: return;
    }

    if (variant_strings_tag != (datum_index)-1 && desc_widget->text != nullptr) {
        uint16_t *text = halo::text::text_string_list_get_string(variant_strings_tag, (int16_t)(2 * (*(const uint32_t *)((const uint8_t *)record + k_record_game_type_byte) - 1) + (((const uint8_t *)record)[k_record_team_play_byte] == 1 ? 1 : 0)));

        wcsncpy((wchar_t *)(halo::interface::widget_text(desc_widget)), (const wchar_t *)text, 0xff);
        (halo::interface::widget_text(desc_widget))[0xff] = 0;
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
    saved_player_profile profile;

    if (!halo::shell::command_line_check_flag("-connect", &address) || address == 0) {
        return 0;
    }

    if (halo::shell::command_line_check_flag("-name", &name) && name != 0) {
        uint16_t count = 100;

        halo::text::string_convert_ascii_to_unicode(wide_name, 0x80, name);
        halo::saved_games::saved_game_enumerate_by_type(0, slots, 0, &count);
        while (count > 0) {
            int32_t slot = slots[count - 1];

            if (slot == -1) {
                memcpy(&profile, default_profile_data, sizeof(profile));
            } else if (halo::saved_games::player_profile_get(slot, &profile) != 0 &&
                       wcscmp((const wchar_t *)wide_name, (const wchar_t *)profile.name) == 0) {
                halo::interface::player_profile_load(0, &profile, slot);
                break;
            }
            count--;
        }
    }

    if (!halo::shell::command_line_check_flag("-password", &password) || password == 0) {
        password = k_empty_string;
    }
    halo::main::network_game_client_connect_to_address_async((char *)address, (char *)password);
    return 1;
}

/**
 * @address 0x4a5ad0
 */
void NetworkSetup::clear_player_ready_flags()
{
    network_client_globals *client = halo::networking::globals().client;
    network_server_globals *server = halo::networking::globals().server;
    network_player_entry *player;
    uint8_t local_ready_flags[16];
    int32_t i;

    if (client == nullptr) {
        return;
    }

    if (client->state == 1) {
        halo::cseries::time_query_performance_counter_ms();
    }

    if (client->state != 2) {
        return;
    }

    player = server == nullptr ? client->session.players : server->session.players;
    local_ready_flags[0] = local_team_00714dd8;

    for (i = 0x10; i != 0; i--) {
        if (halo::networking::network_player_entry_validate(player) != 0) {
            bool clear_flag;

            if (player == nullptr || halo::networking::network_player_entry_validate(player) == 0) {
                clear_flag = halo::networking::globals().game_mode != 3 || player->machine_index == 0;
            } else {
                clear_flag = (server == nullptr || (((server->flags >> 2) & 1) == 0)) &&
                             (int16_t)client->machine_index != -1 && (int16_t)client->machine_index == (int16_t)player->machine_index;
            }
            if (clear_flag) {
                local_ready_flags[player->machine_player_index] = 0;
            }
        }
        player++;
    }

    if (local_ready_flags[0] != 0) {
        halo::networking::network_game_settings_ack_send(client, 0);
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
    halo::game::globals().variant_saved_default_valid = 0;
    halo::networking::globals().game_mode = 0;

    if (halo::game::globals().current_engine != (game_engine_definition *)0) {
        if (halo::game::globals().current_engine->dispose != nullptr) {
            ((void (*)(void))halo::game::globals().current_engine->dispose)();
        }
        halo::game::globals().current_engine = (game_engine_definition *)0;
    }

    if (player_profile_cache_initialized == 1) {
        memset(player_profile_cache, 0, sizeof(player_profile_cache));
        player_profile_cache_initialized = 0;
    }

    memset(&game_engine_active_variant, 0, sizeof(game_engine_active_variant));
}

/**
 * Clears the server's handshake timer pair, unknown_9d0 and the handshake state/flag bytes, then sets the handshake-blocked
 * byte. The retail return value is EAX with only its low byte replaced by 1, so the server pointer's upper three bytes survive.
 *
 * @address 0x4a1670
 */
uint32_t NetworkSetup::server_reset_game_stats()
{
    if (halo::networking::globals().server != (network_server_globals *)0) {
        network_server_globals *server = halo::networking::globals().server;

        server->handshake_timer.remaining_ms = 0;
        server->handshake_timer.last_tick_ms = 0;
        server->unknown_9d0 = 0;
        server->handshake_state = 0;
        server->handshake_blocked = 0;
        server->handshake_flag = 0;
        server->pad_9d7 = 0;
        server->handshake_blocked = 1;
    }
    return ((uint32_t)halo::networking::globals().server & 0xffffff00u) | 1;
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
        halo::interface::widget_close_all();
        if (halo::networking::globals().game_mode == 2) {
            if (halo::game::globals().state == 0) {
                halo::game::game_engine_reset_round_objects();
                halo::game::game_engine_send_round_reset_message();
                halo::game::game_engine_player_profile_cache_sync_all(0, (void *)halo::k_dword_none);
            } else {
                halo::interface::chimera__console_out((ColorARGB *)0, "Cannot restart the map when the game is over.");
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

namespace halo::interface {

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
