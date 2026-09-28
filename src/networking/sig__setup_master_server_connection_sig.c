// sig__setup_master_server_connection_sig  (Chimera's signature name; not a Ghidra function; no C existed)
// address 0x4b5f80, size 121 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4b5f80..0x4b5ff8: the master-server thread master_server_connection_start
//   creates. It marks the browser live (0x719491), creates the GameSpy ServerBrowser (ServerBrowserNew 0x616eb0:
//   the qr2 game name at 0x722798 for both query-for and query-from, the secret key at 0x7227a0, version 0, 10
//   concurrent updates, query version 1, callback network_channel_gap_4ba660, no instance); then, until the stop
//   bit (2) of the request flags is set, runs master_server_process_pending_requests whenever no result is
//   outstanding and sleeps 10 ms. Finally frees the browser (ServerBrowserFree 0x616f30) and returns 0.
// blam-cc: __stdcall (a thread routine; the parameter is unused)

#include "tags.h"

extern uint8_t server_browser_join_requested;    // 0x00719491
extern void *master_server_query_engine;         // 0x0071946c, the ServerBrowser
extern uint32_t master_server_request_flags;     // 0x0071969c
extern int32_t master_server_last_result;        // 0x007196a4
extern char network_session_start_host_name[];   // 0x00722798 (the qr2 game name)
extern char network_session_start_map_name[];    // 0x007227a0 (the qr2 secret key)
extern void master_server_process_pending_requests(void); // 0x4b5d70
extern void network_channel_gap_4ba660(void *sb, uint32_t reason, void *server, void *instance); // 0x4ba660
extern void *ServerBrowserNew(const char *queryForGamename, const char *queryFromGamename, const char *queryFromKey,
    int32_t queryFromVersion, int32_t maxConcurrentUpdates, int32_t queryVersion, void *callback, void *instance); // 0x616eb0
extern void ServerBrowserFree(void *sb);         // 0x616f30
extern void __stdcall Sleep(uint32_t milliseconds);

uint32_t __stdcall sig__setup_master_server_connection_sig(void *parameter)
{
    (void)parameter;
    server_browser_join_requested = 1;
    master_server_query_engine = ServerBrowserNew(network_session_start_host_name, network_session_start_host_name,
        network_session_start_map_name, 0, 10, 1, (void *)network_channel_gap_4ba660, 0);
    if ((master_server_request_flags & 2) == 0) {
        do {
            if (master_server_last_result == 0) {
                master_server_process_pending_requests();
            }
            Sleep(10);
        } while ((master_server_request_flags & 2) == 0);
    }
    ServerBrowserFree(master_server_query_engine);
    master_server_query_engine = 0;
    return 0;
}
