// ui_event_49dca0  (not a Ghidra function; ui_event_function_table[33])
// address 0x49dca0, size 195 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692854 (index 33); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49dca0.
// WRITTEN 2026-09-28 from objdump 0x49dca0..0x49dd62: with a client: a state word (+0xeda) of 1 reads the
//   performance counter (result unused); in state 2, unless one of the 16 player entries of the server (+8) or client
//   (+0xb14) game (0x20 each from +0x1a2) is valid and has machine byte (+0x1c) equal to the client word +0 and
//   player byte (+0x1d) equal to event word 1, sends the game settings ack for event word 1 (zero-extended). Returns
//   1.
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
extern uint8_t *network_client; // 0x0071c2d8 (network_client_globals *)
extern network_server_globals *network_server; // 0x0071c2d4
extern uint32_t time_query_performance_counter_ms(void); // 0x449210
extern char network_player_entry_validate(void *entry); // 0x4de9f0, blam-cc: EAX -> entry
extern char network_game_settings_ack_send(uint8_t *client, int16_t template_row); // 0x4d9f50

uint8_t ui_event_49dca0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *client = network_client;
    int16_t *state;
    uint8_t *game;

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
    game = network_server != 0 ? (uint8_t *)network_server + 8 : network_client != 0 ? network_client + 0xb14 : 0;
    if (network_client != 0 && *(int16_t *)network_client != -1) {
        int16_t key = *(int16_t *)network_client;
        int16_t i;

        for (i = 0; i < 0x10; i++) {
            uint8_t *entry = game + i * 0x20 + 0x1a2;

            if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == key &&
                (int16_t)(int8_t)entry[0x1d] == event[1]) {
                return 1;
            }
        }
    }
    network_game_settings_ack_send(client, (int16_t)(uint16_t)event[1]);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
