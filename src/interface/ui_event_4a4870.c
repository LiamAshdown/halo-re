// ui_event_4a4870  (not a Ghidra function; ui_event_function_table[182])
// address 0x4a4870, size 6 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692aa8 (index 182); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a4870.
// WRITTEN 2026-09-28 from objdump 0x4a4870..0x4a4875: returns the autopatch status byte (0x00719234).
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern uint8_t autopatch_status_state_00719234; // 0x00719234, TYPES-GAP

uint8_t ui_event_4a4870(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return autopatch_status_state_00719234;
}
