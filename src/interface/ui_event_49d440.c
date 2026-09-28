// ui_event_49d440  (not a Ghidra function; ui_event_function_table[18])
// address 0x49d440, size 16 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692818 (index 18); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d440.
// WRITTEN 2026-09-28 from objdump 0x49d440..0x49d44f: empties the widget list (+0x44 = 0, +0x48 = 0); returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

uint8_t ui_event_49d440(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget->list_items = 0;
    widget->item_count = 0;
    return 1;
}
