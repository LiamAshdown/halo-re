// network_channel_gap_4ba660  (not a Ghidra function; no C existed)
// address 0x4ba660, size 225 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ba660..0x4ba740: the ServerBrowser list callback (sb, reason, server,
//   instance) registered by server_browser_open, active once the browser is initialized: server added (0) with basic
//   or full keys, and server updated (1), add the server to the locked list (0x4ba760 / 0x4ba8a0); deleted (2, 3)
//   removes it (0x4ba870 / 0x4ba940), dropping the selection and refreshing the UI when it was selected; query
//   complete (4) resets the elapsed time to 9999 when a query was pending (0x00719488). (Named as its registrant
//   names it.)
// blam-cc: cdecl (a serverbrowsing callback)

// FIXED 2026-09-28: DAT_00719488 here is the global at its address comment, server_browser_query_pending (the name belonged to another
// global at a different address, so the link bound it there).

#include "tags.h"

typedef struct server_list_globals server_list_globals;
extern uint8_t server_browser_initialized;       // 0x00719470
extern uint8_t DAT_00719488;     // 0x00719488, UNSURE name
extern int32_t server_browser_query_elapsed_ms;  // 0x007196c8
extern int32_t server_browser_selected_index;    // 0x006953f4
extern int32_t server_browser_last_click_ms;   // 0x0071947c, UNSURE name
extern int32_t SBServerHasBasicKeys(void *server); // 0x6175c0 SBServerHasBasicKeys
extern int32_t SBServerHasFullKeys(void *server); // 0x6175d0 SBServerHasFullKeys
extern server_list_globals *server_list_mutex_try_lock(uint32_t timeout_ms); // 0x4ba760
extern int32_t dynamic_pointer_array_add_unique(void *value, server_list_globals *array); // 0x4ba8a0
extern void server_list_mutex_unlock(server_list_globals **list_slot); // 0x4ba7a0
extern int32_t dynamic_pointer_array_find_index(server_list_globals *array, void *value); // 0x4ba870
extern void dynamic_pointer_array_remove_at(int32_t index, server_list_globals *array); // 0x4ba940
extern void server_browser_ui_refresh(void); // 0x4b73a0

void network_channel_gap_4ba660(void *sb, uint32_t reason, void *server, void *instance)
{
    server_list_globals *list;

    (void)sb;
    (void)instance;
    if (server_browser_initialized == 0 || reason > 4) {
        return;
    }
    switch (reason) {
    case 0:
        if (SBServerHasBasicKeys(server) == 0 && SBServerHasFullKeys(server) == 0) {
            return;
        }
        // fall through
    case 1:
        list = server_list_mutex_try_lock(100);
        if (list != 0) {
            dynamic_pointer_array_add_unique(server, list);
            server_list_mutex_unlock(&list);
        }
        return;
    case 2:
    case 3:
        list = server_list_mutex_try_lock(100);
        if (list != 0) {
            int32_t index = dynamic_pointer_array_find_index(list, server);

            if (index != -1) {
                dynamic_pointer_array_remove_at(index, list);
                if (index == server_browser_selected_index) {
                    server_browser_selected_index = -1;
                    server_browser_last_click_ms = 0;
                    server_browser_ui_refresh();
                }
            }
            server_list_mutex_unlock(&list);
        }
        return;
    default:
        if (DAT_00719488 != 0) {
            server_browser_query_elapsed_ms = 9999;
        }
        DAT_00719488 = 0;
        return;
    }
}
