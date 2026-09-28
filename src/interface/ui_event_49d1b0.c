// ui_event_49d1b0  (not a Ghidra function; ui_event_function_table[16])
// address 0x49d1b0, size 87 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692810 (index 16); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d1b0.
// WRITTEN 2026-09-28 from objdump 0x49d1b0..0x49d206: disposes the network client globals and any server host,
//   tears down the game setup widget, then creates a network client session; with one, network_game_mode = 1, the
//   host handoff request is cleared and 1 returned, else 0.
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

extern void network_client_globals_dispose(void); // 0x4dde70
extern network_server_globals *network_server; // 0x0071c2d4
extern void network_game_server_host_dispose(void *host); // 0x4deda0
extern uint8_t network_server_host_valid; // 0x0071c2dd, UNSURE identity
extern void network_game_setup_teardown(void); // 0x495520
extern void *network_session_create(void); // 0x4d8a80, blam-cc: EAX -> client
extern network_client_globals *network_client;
extern int16_t network_game_mode; // 0x00719720
extern uint8_t network_host_handoff_requested; // 0x0071c2de

uint8_t ui_event_49d1b0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_client_globals_dispose();
    if (network_server != 0) {
        network_game_server_host_dispose(network_server);
        network_server = 0;
        network_server_host_valid = 0;
    }
    network_game_setup_teardown();
    network_client = (network_client_globals *)network_session_create();
    if (network_client == 0) {
        return 0;
    }
    network_game_mode = 1;
    network_host_handoff_requested = 0;
    return 1;
}
