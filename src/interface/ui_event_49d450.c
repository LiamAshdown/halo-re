// ui_event_49d450  (not a Ghidra function; ui_event_function_table[19])
// address 0x49d450, size 48 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x0069281c (index 19); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d450.
// WRITTEN 2026-09-28 from objdump 0x49d450..0x49d47f: disposes any server host (clearing the pointer and
//   0x0071c2dd), then the network client globals, tears down the game setup widget; returns 1.
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

extern network_server_globals *network_server; // 0x0071c2d4
extern void network_game_server_host_dispose(void *host); // 0x4deda0
extern uint8_t unknown_0071c2dd; // 0x0071c2dd, UNSURE identity
extern void network_client_globals_dispose(void); // 0x4dde70
extern void network_game_setup_teardown(void); // 0x495520

uint8_t ui_event_49d450(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (network_server != 0) {
        network_game_server_host_dispose(network_server);
        network_server = 0;
        unknown_0071c2dd = 0;
    }
    network_client_globals_dispose();
    network_game_setup_teardown();
    return 1;
}
