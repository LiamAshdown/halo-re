// ui_event_4a1650  (not a Ghidra function; ui_event_function_table[83])
// address 0x4a1650, size 31 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x0069291c (index 83); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1650.
// WRITTEN 2026-09-28 from objdump 0x4a1650..0x4a166e: with a server up, sets bit 0 of its word +0x06 and byte
//   +0xae0 of the record its first dword points at; returns 1.
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

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_server_globals *network_server; // 0x0071c2d4

uint8_t ui_event_4a1650(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *server = (uint8_t *)network_server;

    if (server != 0) {
        *(uint16_t *)(server + 6) |= 1;
        (*(uint8_t **)server)[0xae0] = 1;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
