// ui_event_49d160  (not a Ghidra function; ui_event_function_table[14])
// address 0x49d160, size 59 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692808 (index 14); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d160.
// WRITTEN 2026-09-28 from objdump 0x49d160..0x49d19a: disposes the network client globals, then any server host
//   (clearing the pointer and 0x0071c2dd), clears 0x00714dd8 and the first byte of 0x00714ddc, tears down the game
//   setup widget; returns 1.
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
#include "fn_networking.h"


extern network_server_globals *network_server; // 0x0071c2d4
extern void network_game_server_host_dispose(void *host); // 0x4deda0
extern uint8_t network_server_host_valid; // 0x0071c2dd, UNSURE identity
extern uint8_t local_team_00714dd8; // 0x00714dd8, TYPES-GAP
extern uint8_t coop_profile_globals_block_00714ddc[0x1ffc]; // 0x00714ddc, TYPES-GAP
extern void network_game_setup_teardown(void); // 0x495520

uint8_t ui_event_49d160(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_client_globals_dispose();
    if (network_server != 0) {
        network_game_server_host_dispose(network_server);
        network_server = 0;
        network_server_host_valid = 0;
    }
    local_team_00714dd8 = 0;
    coop_profile_globals_block_00714ddc[0] = 0;
    network_game_setup_teardown();
    return 1;
}
