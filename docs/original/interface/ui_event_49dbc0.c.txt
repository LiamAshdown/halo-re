// ui_event_49dbc0  (not a Ghidra function; ui_event_function_table[32])
// address 0x49dbc0, size 211 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692850 (index 32); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49dbc0.
// WRITTEN 2026-09-28 from objdump 0x49dbc0..0x49dc92: with a server (+8) or client (+0xb14) game whose byte +0x138
//   is 1 and a client with a machine word (+0) other than -1: finds the valid player entry (16 of 0x20 bytes from
//   +0x1a2) with that machine byte (+0x1c) and event word 1 as player byte (+0x1d), copies it, toggles its byte +0x1e
//   (becomes 1 when it was 0, else 0) and sends it as a game record message (0x4da130). Returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

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

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t *network_client; // 0x0071c2d8 (network_client_globals *)
extern char network_player_entry_validate(void *entry); // 0x4de9f0, blam-cc: EAX -> entry
extern int32_t network_game_record_message_send(void *client, const uint32_t *source); // 0x4da130, blam-cc: EDX source, stack client

uint8_t ui_event_49dbc0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *game = network_server != 0 ? (uint8_t *)network_server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;
    int16_t key;
    int32_t i;

    if (game == 0 || game[0x138] != 1 || network_client == 0) {
        return 1;
    }
    key = *(int16_t *)network_client;
    if (key == -1) {
        return 1;
    }
    for (i = 0; i < 0x10; i++) {
        uint8_t *entry = game + 0x1a2 + i * 0x20;

        if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == key &&
            (int16_t)(int8_t)entry[0x1d] == event[1]) {
            uint32_t copy[8];

            memcpy(copy, entry, sizeof(copy));
            ((uint8_t *)copy)[0x1e] = (uint8_t)(((uint8_t *)copy)[0x1e] == 0);
            network_game_record_message_send(network_client, copy);
            return 1;
        }
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
