// ui_event_4a10f0  (not a Ghidra function; ui_event_function_table[72])
// address 0x4a10f0, size 22 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928f0 (index 72); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a10f0.
// WRITTEN 2026-09-28 from objdump 0x4a10f0..0x4a1105: runs the network client rejoin check (0x4de390) for event
//   word 1; returns 1. The binary passes the zero-extended word and the callee compares it as a dword against sign-
//   extended bytes; the C callee takes int8_t, so values 0x80..0xffff differ (never seen: the word is a machine
//   index).
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "fn_networking.h"


uint8_t ui_event_4a10f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_client_rejoin_check((int8_t)event[1]);
    return 1;
}
