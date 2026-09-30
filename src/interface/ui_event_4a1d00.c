// ui_event_4a1d00  (not a Ghidra function; ui_event_function_table[110])
// address 0x4a1d00, size 48 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.9
// evidence: ui_event_function_table 0x006927d0 slot 0x00692988 (index 110); widget_instance_handle_input_event
//   runs it for a widget event. Only reachable through that table. Campaign track: reached from the main menu.
// objdump 0x4a1d00..0x4a1d2f: while the input event queue is up (its first byte, 0x00712cc0), pushes a
//   button event (kind 3, code 0xa, pressed 1; the other bytes are left as stack garbage, zeroed here) into
//   queue 0 (input_queue_push_event, EAX = 0, EDI = the event); returns 1 either way. The arguments are not read.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "fn_input.h"
#include <string.h>

extern uint8_t input_event_queue_active; // 0x00712cc0


uint8_t ui_event_4a1d00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (input_event_queue_active) {
        ui_input_event queued;

        memset(&queued, 0, sizeof(queued));
        queued.kind = 3;
        queued.code = 0xa;
        queued.pressed = 1;
        input_queue_push_event(0, &queued);
    }
    return 1;
}
