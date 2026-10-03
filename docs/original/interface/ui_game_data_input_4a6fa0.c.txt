// ui_game_data_input_4a6fa0  (not a Ghidra function; game_data_input_function_table[32])
// address 0x4a6fa0, size 471 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b98 (index 32); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a6fa0.
// WRITTEN 2026-09-28 from objdump 0x4a6fa0..0x4a7176: the same map path match as 0x4a6b70 (beavercreek, sidewinder,
//   damnation, ratrace, prisoner, hangemhigh, chillout, carousel, boardingaction, bloodgulch, wizard, putput, longest
//   -> 0..12, icefields 0xd, else 0x13), stored as the background frame (+0x58).
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
extern char *strstr(const char *haystack, const char *needle); // 0x625430

void ui_game_data_input_4a6fa0(widget_instance *widget)
{
    uint8_t *game = network_server != 0 ? (uint8_t *)network_server + 8
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
            widget->background_bitmap_frame = i;
            return;
        }
    }
    widget->background_bitmap_frame = (int16_t)(strstr((char *)(game + 0x84), "icefields") != 0 ? 0xd : 0x13);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
