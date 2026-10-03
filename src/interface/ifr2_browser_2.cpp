#include "win32.h"
#include "halo/interface/engine_state.hpp"
#include "halo/interface/ifr2_browser.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/networking/api.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern uint8_t server_browser_initialized;
extern void *server_list_thread;
extern void *master_server_query_engine;
extern void *server_list;
extern int32_t server_list_block_used;
extern int32_t server_list_block_capacity;
extern int32_t server_browser_query_elapsed_ms;
extern uint8_t server_browser_player_ticker[0x1c];
extern uint8_t server_browser_variant_ticker[0x1c];
extern int32_t selected_saved_item;
extern uint8_t saved_item_working_copy[0x1ffc];
extern uint8_t server_browser_sort_column;
extern uint8_t server_browser_sort_ascending;
extern uint8_t server_browser_allow_password;
extern uint8_t server_browser_filter_dedicated_only;
extern uint8_t server_browser_filter_classic_only;
extern uint8_t server_browser_filter_allow_unknown_map;
extern uint8_t server_browser_allow_empty;
extern uint8_t server_browser_allow_full;
extern uint8_t server_browser_filter_gametype;
extern uint8_t server_browser_filter_teamplay;
extern uint8_t server_browser_filter_ping_limit_index;
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
    saved_item_select(profile);
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
    if (saved_item_has_unsaved_changes()) {
        player_profile_save();
        return 1;
    }
    selected_saved_item = -1;
    return 1;
}

} // namespace halo::interface

extern "C" {

uint8_t server_browser_closed_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::interface::k_closed_event.handle(widget, event, out_handled);
}

}
