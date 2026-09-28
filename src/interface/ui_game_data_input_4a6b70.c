// ui_game_data_input_4a6b70  (not a Ghidra function; game_data_input_function_table[27])
// address 0x4a6b70, size 471 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b84 (index 27); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a6b70.
// WRITTEN 2026-09-28 from objdump 0x4a6b70..0x4a6d46: with a server (+8) or client (+0xb14) game, the first of
//   beavercreek, sidewinder, damnation, ratrace, prisoner, hangemhigh, chillout, carousel, boardingaction,
//   bloodgulch, wizard, putput, longest found (strstr) in its map path (+0x84) gives selection (+0x40) 0..12;
//   icefields gives 0xd; anything else 0x13.
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern uint8_t *network_client; // 0x0071c2d8 (network_client_globals *)
extern char *strstr(const char *haystack, const char *needle); // 0x625430

void ui_game_data_input_4a6b70(widget_instance *widget)
{
    uint8_t *game = network_server_pointer != 0 ? (uint8_t *)network_server_pointer + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;
    static const char *const maps[] = {
        "beavercreek", "sidewinder", "damnation", "ratrace", "prisoner",
        "hangemhigh", "chillout", "carousel", "boardingaction", "bloodgulch",
        "wizard", "putput", "longest"
    };
    int16_t i;

    if (game == 0) {
        return;
    }
    for (i = 0; i < (int16_t)(sizeof(maps) / sizeof(maps[0])); i++) {
        if (strstr((char *)(game + 0x84), maps[i]) != 0) {
            widget->selection_index = i;
            return;
        }
    }
    widget->selection_index = (int16_t)(strstr((char *)(game + 0x84), "icefields") != 0 ? 0xd : 0x13);
}
