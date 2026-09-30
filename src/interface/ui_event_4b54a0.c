// ui_event_4b54a0  (not a Ghidra function; ui_event_function_table[153])
// address 0x4b54a0, size 28 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a34 (index 153); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4b54a0.
// WRITTEN 2026-09-28 from objdump 0x4b54a0..0x4b54bb: switches the controls binding rows of the great-grandparent
//   widget to device mode 1; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "fn_interface.h"


uint8_t ui_event_4b54a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    controls_binding_rows_toggle_device_mode(widget->parent->parent->parent, 1);
    return 1;
}
