// ui_event_4a1d60  (not a Ghidra function; ui_event_function_table[128])
// address 0x4a1d60, size 48 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.9
// evidence: ui_event_function_table 0x006927d0 slot 0x006929d0 (index 128); widget_instance_handle_input_event
//   runs it for a widget event. Only reachable through that table. Campaign track: reached from the main menu.
// objdump 0x4a1d60..0x4a1d8f: while the input event queue is up (its first byte, 0x00712cc0), pushes a
//   button event (kind 3, code 0x2, pressed 1; the other bytes are left as stack garbage, zeroed here) into
//   queue 0 (input_queue_push_event, EAX = 0, EDI = the event); returns 1 either way. The arguments are not read.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t input_event_queue_active; // 0x00712cc0
extern void input_queue_push_event(int16_t queue_index, ui_input_event *record); // 0x492340, blam-cc: EAX, EDI

uint8_t ui_event_4a1d60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (input_event_queue_active) {
        ui_input_event queued;

        memset(&queued, 0, sizeof(queued));
        queued.kind = 3;
        queued.code = 0x2;
        queued.pressed = 1;
        input_queue_push_event(0, &queued);
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
