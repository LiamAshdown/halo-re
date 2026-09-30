// server_browser_closed_event  (reached only through a .data code pointer; no C existed)
// address 0x4b7920, size 341 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4b7920..0x4b7a74: ui_event_function_table slot 0x6929a4, a server-browser
//   widget event. the browser closing: a running browser stops (waiting for the master-server thread, or freeing the
//   ServerBrowser); the server list and its block are reset, both tickers emptied, autopatch downloads shut down, the
//   join target cleared (-1). With a profile selected, the browser settings (sort column / order, password,
//   dedicated, classic, unknown map, empty, full, game type, team play, ping) go into the working copy
//   (+0xc80..+0xc8a; only when the selected item is a profile) and it is saved when changed, else the selection is
//   dropped; 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

// FIXED 2026-09-28: DAT_00695420 here is the global at its address comment, server_browser_join_target (the name belonged to another
// global at a different address, so the link bound it there).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "fn_networking.h"
#include "fn_interface.h"

extern uint8_t server_browser_initialized;                // 0x00719470
extern void *server_list_thread;                          // 0x007196ac
extern void *master_server_query_engine;                  // 0x0071946c
extern void *server_list;                           // 0x007196bc
extern int32_t server_list_block_used;                    // 0x007196c0
extern int32_t server_list_block_capacity;                // 0x007196c4
extern int32_t server_browser_query_elapsed_ms;           // 0x007196c8
extern uint8_t server_browser_player_ticker[0x1c];        // 0x006b5e58
extern uint8_t server_browser_variant_ticker[0x1c];       // 0x006b5e74
extern int32_t DAT_00695420;                // 0x00695420
extern int32_t saved_player_profile_slots_handle;                  // 0x00714dd4
extern int32_t selected_saved_item;                       // 0x00714e7c
extern uint8_t saved_item_working_copy[0x1ffc];           // 0x00714e80
extern uint8_t server_browser_sort_column;                // 0x00719489
extern uint8_t server_browser_sort_ascending;             // 0x006953f8
extern uint8_t server_browser_allow_password;             // 0x006953f9
extern uint8_t server_browser_filter_dedicated_only;      // 0x0071948b
extern uint8_t server_browser_filter_classic_only;        // 0x0071948c
extern uint8_t server_browser_filter_allow_unknown_map;   // 0x0071948d
extern uint8_t server_browser_allow_empty;                // 0x006953fa
extern uint8_t server_browser_allow_full;                 // 0x006953fb
extern uint8_t server_browser_filter_gametype;            // 0x0071948e
extern uint8_t server_browser_filter_teamplay;            // 0x0071948f
extern uint8_t server_browser_filter_ping_limit_index;    // 0x00719490

extern void ServerBrowserFree(void *sb);                  // 0x616f30
extern void server_list_reset(uint8_t *entry);            // 0x4b65f0, EAX
extern void ticker_text_buffer_reset(void *self);         // 0x4b8a00, EDI

extern void saved_item_select(int32_t item);              // 0x495be0, EBX


uint8_t server_browser_closed_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t profile;
    uint8_t *working;

    (void)widget;
    (void)event;
    (void)out_handled;
    if (server_browser_initialized) {
        if (server_list_thread != 0) {
            master_server_connection_wait_thread();
        } else {
            ServerBrowserFree(master_server_query_engine);
            master_server_query_engine = 0;
        }
    }
    server_browser_initialized = 0;
    server_list_reset(0);
    if (server_list != 0) {
        GlobalFree(server_list);
    }
    server_list = 0;
    server_list_block_used = 0;
    server_list_block_capacity = 0;
    server_browser_query_elapsed_ms = 0;
    ticker_text_buffer_reset(server_browser_player_ticker);
    ticker_text_buffer_reset(server_browser_variant_ticker);
    autopatch_download_pool_shutdown();
    profile = saved_player_profile_slots_handle;
    DAT_00695420 = -1;
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
