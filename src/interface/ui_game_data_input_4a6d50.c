// ui_game_data_input_4a6d50  (not a Ghidra function; game_data_input_function_table[28])
// address 0x4a6d50, size 230 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b88 (index 28); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a6d50.
// WRITTEN 2026-09-28 from objdump 0x4a6d50..0x4a6e35: with a server (+8) or client (+0xb14) game, its dword +0x134
//   picks the selection (+0x40): 1 -> byte +0x180 == 1 ? (dword +0x184 ? 0x1c : 0x1d) : (dword +0x184 ? 0x1e : 3); 2
//   -> 4; 3 -> dword +0x190 1 / 2 give 0x1f / 0x20, else 5; 4 -> 6; 5 -> dword +0x180 == 2 ? 0x21 : 7; else 8.
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

extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t *network_client; // 0x0071c2d8 (network_client_globals *)

void ui_game_data_input_4a6d50(widget_instance *widget)
{
    uint8_t *game = network_server != 0 ? (uint8_t *)network_server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;

    if (game == 0) {
        return;
    }
    switch (*(int32_t *)(game + 0x134)) {
    case 1:
        if (game[0x180] == 1) {
            widget->selection_index = (int16_t)(*(int32_t *)(game + 0x184) != 0 ? 0x1c : 0x1d);
        } else {
            widget->selection_index = (int16_t)(*(int32_t *)(game + 0x184) != 0 ? 0x1e : 3);
        }
        break;
    case 2:
        widget->selection_index = 4;
        break;
    case 3:
        switch (*(int32_t *)(game + 0x190)) {
        case 1:
            widget->selection_index = 0x1f;
            break;
        case 2:
            widget->selection_index = 0x20;
            break;
        default:
            widget->selection_index = 5;
            break;
        }
        break;
    case 4:
        widget->selection_index = 6;
        break;
    case 5:
        widget->selection_index = (int16_t)(*(int32_t *)(game + 0x180) == 2 ? 0x21 : 7);
        break;
    default:
        widget->selection_index = 8;
        break;
    }
}
