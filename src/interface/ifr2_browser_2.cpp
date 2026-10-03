#include "win32.h"
#include "halo/interface/engine_state.hpp"
#include "halo/interface/ifr2_browser.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "saved_games.h"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"

#ifdef interface
#undef interface
#endif

static auto &server_browser_initialized = halo::link::ref<uint8_t>(halo::networking::vars().server_browser_initialized);
static auto &server_list_thread = halo::link::ref<void *>(halo::networking::vars().server_list_thread);
static auto &master_server_query_engine = halo::link::ref<void *>(halo::networking::vars().master_server_query_engine);
static auto &server_list = halo::link::ref<void *>(halo::networking::vars().server_list);
static auto &server_list_block_used = halo::link::ref<int32_t>(halo::ui::vars().server_list_block_used);
static auto &server_list_block_capacity = halo::link::ref<int32_t>(halo::ui::vars().server_list_block_capacity);
static auto &server_browser_query_elapsed_ms = halo::link::ref<int32_t>(halo::networking::vars().server_browser_query_elapsed_ms);
static auto &server_browser_player_ticker = halo::link::ref<uint8_t [0x1c]>(halo::ui::vars().server_browser_player_ticker);
static auto &server_browser_variant_ticker = halo::link::ref<uint8_t [0x1c]>(halo::ui::vars().server_browser_variant_ticker);
static auto &selected_saved_item = halo::link::ref<int32_t>(halo::ui::vars().selected_saved_item);
static auto &saved_item_working_copy = halo::link::ref<uint8_t [k_saved_player_profile_size]>(halo::ui::vars().saved_item_working_copy);
static auto &server_browser_sort_column = halo::link::ref<uint8_t>(halo::ui::vars().server_browser_sort_column);
static auto &server_browser_sort_ascending = halo::link::ref<uint8_t>(halo::ui::vars().server_browser_sort_ascending);
static auto &server_browser_allow_password = halo::link::ref<uint8_t>(halo::ui::vars().server_browser_allow_password);
static auto &server_browser_filter_dedicated_only = halo::link::ref<uint8_t>(halo::ui::vars().server_browser_filter_dedicated_only);
static auto &server_browser_filter_classic_only = halo::link::ref<uint8_t>(halo::ui::vars().server_browser_filter_classic_only);
static auto &server_browser_filter_allow_unknown_map = halo::link::ref<uint8_t>(halo::ui::vars().server_browser_filter_allow_unknown_map);
static auto &server_browser_allow_empty = halo::link::ref<uint8_t>(halo::ui::vars().server_browser_allow_empty);
static auto &server_browser_allow_full = halo::link::ref<uint8_t>(halo::ui::vars().server_browser_allow_full);
static auto &server_browser_filter_gametype = halo::link::ref<uint8_t>(halo::ui::vars().server_browser_filter_gametype);
static auto &server_browser_filter_teamplay = halo::link::ref<uint8_t>(halo::ui::vars().server_browser_filter_teamplay);
static auto &server_browser_filter_ping_limit_index = halo::link::ref<uint8_t>(halo::ui::vars().server_browser_filter_ping_limit_index);
extern "C" {
extern void ServerBrowserFree(void *sb);
extern void saved_item_select(int32_t item);
extern uint8_t saved_item_has_unsaved_changes(void);
extern uint8_t player_profile_save(void);
}

namespace halo::interface {

namespace {
const ClosedHandler k_closed_event;
} // namespace

/**
 * @address 0x4b7920
 */
uint8_t ClosedHandler::handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const
{
    int32_t profile;
    uint8_t *working;

    (void)widget;
    (void)event;
    (void)out_handled;
    if (server_browser_initialized) {
        if (server_list_thread != 0) {
            halo::networking::master_server_connection_wait_thread();
        } else {
            ServerBrowserFree(master_server_query_engine);
            master_server_query_engine = 0;
        }
    }
    server_browser_initialized = 0;
    halo::networking::server_list_reset(0);
    if (server_list != 0) {
        GlobalFree(server_list);
    }
    server_list = 0;
    server_list_block_used = 0;
    server_list_block_capacity = 0;
    server_browser_query_elapsed_ms = 0;
    halo::networking::ticker_text_buffer_reset((ticker_text_buffer *)server_browser_player_ticker);
    halo::networking::ticker_text_buffer_reset((ticker_text_buffer *)server_browser_variant_ticker);
    halo::networking::autopatch_download_pool_shutdown();
    profile = halo::saved_games::globals().player_profile_slots_handle;
    state::autopatch_active_slot = -1;
    if (profile == -1) {
        return 1;
    }
    halo::interface::saved_item_select(profile);
    working = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    working[0xc81] = server_browser_sort_ascending;
    working[0xc80] = server_browser_sort_column;
    working[0xc82] = server_browser_allow_password;
    working[0xc83] = server_browser_filter_dedicated_only;
    working[0xc84] = server_browser_filter_classic_only;
    working[0xc85] = server_browser_filter_allow_unknown_map;
    working[0xc86] = server_browser_allow_empty;
    working[0xc87] = server_browser_allow_full;
    working[0xc88] = server_browser_filter_gametype;
    working[0xc89] = server_browser_filter_teamplay;
    working[0xc8a] = server_browser_filter_ping_limit_index;
    if (halo::interface::saved_item_has_unsaved_changes()) {
        halo::interface::player_profile_save();
        return 1;
    }
    selected_saved_item = -1;
    return 1;
}

} // namespace halo::interface

namespace halo::interface {

uint8_t server_browser_closed_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::interface::k_closed_event.handle(widget, event, out_handled);
}

}
