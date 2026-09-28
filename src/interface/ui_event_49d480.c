// ui_event_49d480  (not a Ghidra function; ui_event_function_table[21])
// address 0x49d480, size 155 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692824 (index 21); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d480.
// WRITTEN 2026-09-28 from objdump 0x49d480..0x49d51a: clears 0x0071c2dc; without a server, makes sure the variant
//   history has an entry and creates the server host (its low byte is the result): on 1 forgets the current variant
//   history entry, applies the current custom variant, syncs the variant defaults and sets network_game_mode = 2.
//   While still good, creates the client session unless there is one (clearing the handoff request) and the result
//   becomes whether one exists. On failure disposes the server host, the client globals and the game setup widget.
//   Returns the result.
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

extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern void network_game_server_host_dispose(void *host); // 0x4deda0
extern uint8_t unknown_0071c2dd; // 0x0071c2dd, UNSURE identity
extern uint8_t network_session_starting_0071c2dc; // 0x0071c2dc, TYPES-GAP
extern uint32_t game_engine_ensure_variant_history_has_entry(void); // 0x463b20
extern int32_t network_game_server_host_create(void); // 0x4ddd40
extern int32_t game_variant_history_current; // 0x00687b18
extern void game_engine_apply_current_custom_variant(void); // 0x463b90
extern void game_engine_sync_variant_defaults(void); // 0x45fc80
extern int16_t network_game_mode; // 0x00719720
extern network_client_globals *network_client;
extern void *network_session_create(void); // 0x4d8a80, blam-cc: EAX -> client
extern uint8_t network_host_handoff_requested; // 0x0071c2de
extern void network_client_globals_dispose(void); // 0x4dde70
extern void network_game_setup_teardown(void); // 0x495520

uint8_t ui_event_49d480(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t ok = 1;

    network_session_starting_0071c2dc = 0;
    if (network_server_pointer == 0) {
        game_engine_ensure_variant_history_has_entry();
        ok = (uint8_t)network_game_server_host_create();
        if (ok == 1) {
            game_variant_history_current = -1;
            game_engine_apply_current_custom_variant();
            game_engine_sync_variant_defaults();
            network_game_mode = 2;
        }
    }
    if (ok != 0 && network_client == 0) {
        network_client = (network_client_globals *)network_session_create();
        if (network_client != 0) {
            network_host_handoff_requested = 0;
        }
        ok = (uint8_t)(network_client != 0);
    }
    if (ok == 0) {
        if (network_server_pointer != 0) {
            network_game_server_host_dispose(network_server_pointer);
            network_server_pointer = 0;
            unknown_0071c2dd = 0;
        }
        network_client_globals_dispose();
        network_game_setup_teardown();
    }
    return ok;
}
