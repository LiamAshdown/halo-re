// ui_game_data_input_4a6e50  (not a Ghidra function; game_data_input_function_table[29])
// address 0x4a6e50, size 58 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b8c (index 29); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a6e50.
// WRITTEN 2026-09-28 from objdump 0x4a6e50..0x4a6e89: takes the server game (server +8) or else the client game
//   (client +0xb14); when there is one, the selection (+0x40) becomes 0xc plus (its byte +0x138 != 1).
// blam-cc: stack -> widget (cdecl)

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
extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t *network_client; // 0x0071c2d8 (network_client_globals *)

void ui_game_data_input_4a6e50(widget_instance *widget)
{
    uint8_t *game = network_server != 0 ? (uint8_t *)network_server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;

    if (game != 0) {
        widget->selection_index = (int16_t)((game[0x138] != 1) + 0xc);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
