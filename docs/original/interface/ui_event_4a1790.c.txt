// ui_event_4a1790  (not a Ghidra function; ui_event_function_table[93])
// address 0x4a1790, size 363 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692944 (index 93); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1790.
// WRITTEN 2026-09-28 from objdump 0x4a1790..0x4a18fa: with a client in state 2 (a state of 1 first reads the
//   performance counter, unused) and a machine word other than -1: counts the valid player entries (16 of 0x20 bytes
//   from client +0xcb6) of this machine, remembering the one whose player byte matches event word 1. None: returns 1.
//   With a match, sends it as a session info packet and clears that local player's profile byte (0x00714dd8 + player
//   * 0x2004). Exactly one entry: with a server up and 0x0071c2dc not 1 returns the game stats reset result
//   (0x4a1670, the pushed arguments are not read); otherwise disposes the client globals and the server host and
//   returns 1; either way 0x00714dd8 takes the byte at 0x00714ddc. More than one entry returns 0. Without a client,
//   or not in state 2, returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_client_globals *network_client;
extern uint32_t time_query_performance_counter_ms(void); // 0x449210
extern char network_player_entry_validate(void *entry); // 0x4de9f0, blam-cc: EAX -> entry
extern char network_session_info_packet_send(const uint32_t *source, void *client); // 0x4d9050, blam-cc: EAX source, stack client
extern uint8_t local_team_00714dd8[]; // 0x00714dd8, 0x2004 bytes per local player
extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_disconnect_timeout_flag; // 0x0071c2dc, TYPES-GAP
extern uint32_t network_server_reset_game_stats(void); // 0x4a1670
extern uint8_t coop_profile_globals_block_00714ddc[0x1ffc]; // 0x00714ddc, TYPES-GAP
extern void network_client_globals_dispose(void); // 0x4dde70
extern void network_game_server_host_dispose(void *host); // 0x4deda0
extern uint8_t network_server_host_valid; // 0x0071c2dd, UNSURE identity

uint8_t ui_event_4a1790(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *client = (uint8_t *)network_client; // still addressed by byte offset below
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
