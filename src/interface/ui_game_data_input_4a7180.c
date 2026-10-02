// ui_game_data_input_4a7180  (not a Ghidra function; game_data_input_function_table[33])
// address 0x4a7180, size 117 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b9c (index 33); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a7180.
// WRITTEN 2026-09-28 from objdump 0x4a7180..0x4a71f4: with a server (+8) or client (+0xb14) game, its dword +0x134
//   1..5 sets the background frame (+0x58) to 0, 2, 3, 1, 4; anything else 5.
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

void ui_game_data_input_4a7180(widget_instance *widget)
{
    uint8_t *game = network_server != 0 ? (uint8_t *)network_server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;

    if (game == 0) {
        return;
    }
    switch (*(int32_t *)(game + 0x134)) {
    case 1:
        widget->background_bitmap_frame = 0;
        break;
    case 2:
        widget->background_bitmap_frame = 2;
        break;
    case 3:
        widget->background_bitmap_frame = 3;
        break;
    case 4:
        widget->background_bitmap_frame = 1;
        break;
    case 5:
        widget->background_bitmap_frame = 4;
        break;
    default:
        widget->background_bitmap_frame = 5;
        break;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
