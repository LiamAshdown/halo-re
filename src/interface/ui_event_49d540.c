// ui_event_49d540  (not a Ghidra function; ui_event_function_table[23])
// address 0x49d540, size 111 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x0069282c (index 23); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d540.
// WRITTEN 2026-09-28 from objdump 0x49d540..0x49d5ae: clears 0x00714dd8 and the first byte of 0x00714ddc, tears
//   down the game setup widget, disposes the client globals and any server host, then clears 0x0071c2dc,
//   network_game_mode and 0x00719010, one local player, no selected saved item; restarts the title music unless it is
//   pending; returns 1.
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

extern uint8_t local_team_00714dd8; // 0x00714dd8, TYPES-GAP
extern uint8_t coop_profile_globals_block_00714ddc[0x1ffc]; // 0x00714ddc, TYPES-GAP
extern void network_game_setup_teardown(void); // 0x495520
extern void network_client_globals_dispose(void); // 0x4dde70
extern network_server_globals *network_server; // 0x0071c2d4
extern void network_game_server_host_dispose(void *host); // 0x4deda0
extern uint8_t network_server_host_valid; // 0x0071c2dd, UNSURE identity
extern uint8_t main_menu_music_pending; // 0x00718fc6
extern uint8_t network_disconnect_timeout_flag; // 0x0071c2dc, TYPES-GAP
extern int16_t network_game_mode; // 0x00719720
extern uint8_t save_in_progress_00719010; // 0x00719010
extern int16_t local_player_count; // 0x006894b8
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern void main_menu_play_title_music(void); // 0x4993e0

uint8_t ui_event_49d540(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t music_pending;

    local_team_00714dd8 = 0;
    coop_profile_globals_block_00714ddc[0] = 0;
    network_game_setup_teardown();
    network_client_globals_dispose();
    if (network_server != 0) {
        network_game_server_host_dispose(network_server);
        network_server = 0;
        network_server_host_valid = 0;
    }
    music_pending = main_menu_music_pending;
    network_disconnect_timeout_flag = 0;
    network_game_mode = 0;
    save_in_progress_00719010 = 0;
    local_player_count = 1;
    selected_saved_item = -1;
    if (music_pending == 0) {
        main_menu_play_title_music();
    }
    return 1;
}
