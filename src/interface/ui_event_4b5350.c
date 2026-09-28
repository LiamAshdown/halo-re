// ui_event_4b5350  (not a Ghidra function; ui_event_function_table[152])
// address 0x4b5350, size 70 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a30 (index 152); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4b5350.
// WRITTEN 2026-09-28 from objdump 0x4b5350..0x4b5395: in controls list mode (0x00719445): hides the third child
//   (state 0, hidden 1), focuses and shows the second (state 1, hidden 0), leaves list mode, returns 1. Otherwise
//   closes the widget restoring the previous one, *out_handled = 1, returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern uint8_t controls_menu_list_mode; // 0x00719445, UNSURE name
extern void widget_instance_close_and_restore_previous(widget_instance *widget); // 0x49c3e0, blam-cc: EAX

uint8_t ui_event_4b5350(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (controls_menu_list_mode != 0) {
        widget_instance *second = widget->first_child->next_sibling;
        widget_instance *third = second->next_sibling;

        third->state = 0;
        third->hidden = 1;
        widget->focused_child = second;
        second->state = 1;
        second->hidden = 0;
        controls_menu_list_mode = 0;
        return 1;
    }
    widget_instance_close_and_restore_previous(widget);
    *out_handled = 1;
    return 1;
}
