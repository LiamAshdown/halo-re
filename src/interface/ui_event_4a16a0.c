// ui_event_4a16a0  (not a Ghidra function; ui_event_function_table[85])
// address 0x4a16a0, size 19 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692924 (index 85); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a16a0.
// WRITTEN 2026-09-28 from objdump 0x4a16a0..0x4a16b2: with a server up, clears its byte +0x9d5; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)

uint8_t ui_event_4a16a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (network_server_pointer != 0) {
        ((uint8_t *)network_server_pointer)[0x9d5] = 0;
    }
    return 1;
}
