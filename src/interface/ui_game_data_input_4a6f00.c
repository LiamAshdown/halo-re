// ui_game_data_input_4a6f00  (not a Ghidra function; game_data_input_function_table[31])
// address 0x4a6f00, size 132 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b94 (index 31); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a6f00.
// WRITTEN 2026-09-28 from objdump 0x4a6f00..0x4a6f83: with a server (+8) or client (+0xb14) game, its dword +0x134
//   1..5 sets the selection (+0x40) to 0x16, 0x18, 0x17 + (dword +0x190 == 2), 0x17, 0x19; anything else 0x18.
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern uint8_t *network_client; // 0x0071c2d8 (network_client_globals *)

void ui_game_data_input_4a6f00(widget_instance *widget)
{
    uint8_t *game = network_server_pointer != 0 ? (uint8_t *)network_server_pointer + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;

    if (game == 0) {
        return;
    }
    switch (*(int32_t *)(game + 0x134)) {
    case 1:
        widget->selection_index = 0x16;
        break;
    case 3:
        widget->selection_index = (int16_t)(0x17 + (*(int32_t *)(game + 0x190) == 2));
        break;
    case 4:
        widget->selection_index = 0x17;
        break;
    case 5:
        widget->selection_index = 0x19;
        break;
    default:
        widget->selection_index = 0x18;
        break;
    }
}
