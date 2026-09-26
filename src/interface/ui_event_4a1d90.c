// ui_event_4a1d90  (not a Ghidra function; ui_event_function_table[164])
// address 0x4a1d90, size 36 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.9
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a60 (index 164); widget_instance_handle_input_event
//   runs it for a widget event. Only reachable through that table. First-boot track: CAMPAIGN -> NEW GAME.
// objdump 0x4a1d90..0x4a1db3: pushes an 8-byte event of kind 5 (everything else zero) into input queue 0
//   (input_queue_push_event, EAX = 0, EDI = the event) and returns 1. The arguments are not read.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include <string.h>

extern void input_queue_push_event(int16_t queue_index, ui_input_event *record); // 0x492340, blam-cc: EAX, EDI

uint8_t ui_event_4a1d90(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    ui_input_event queued;

    memset(&queued, 0, sizeof(queued));
    queued.kind = 5;
    input_queue_push_event(0, &queued);
    return 1;
}
