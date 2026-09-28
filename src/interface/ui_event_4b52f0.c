// ui_event_4b52f0  (not a Ghidra function; ui_event_function_table[115])
// address 0x4b52f0, size 81 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x0069299c (index 115); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4b52f0.
// WRITTEN 2026-09-28 from objdump 0x4b52f0..0x4b5340: finds the widget among the eight siblings starting at its
//   parent's third child (not found: returns 1); the position goes to 0x006953e8, key binding scan starts, and byte
//   +0x54 (text pulse) is set on the widget's second child, and on the first two children of its third child. Returns
//   1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern int32_t controls_capture_row; // 0x006953e8, UNSURE identity
extern void input_bind_scan_set_active(uint8_t enable_scan); // 0x48b6b0, blam-cc: AL

uint8_t ui_event_4b52f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *child = widget->parent->first_child->next_sibling->next_sibling;
    widget_instance *second;
    widget_instance *third;
    int32_t i;

    for (i = 0; child != widget; i++) {
        if (i + 1 >= 8) {
            return 1;
        }
        child = child->next_sibling;
    }
    controls_capture_row = i;
    input_bind_scan_set_active(1);
    second = child->first_child->next_sibling;
    third = second->next_sibling;
    *((uint8_t *)second + 0x54) = 1;
    *((uint8_t *)third->first_child + 0x54) = 1;
    *((uint8_t *)third->first_child->next_sibling + 0x54) = 1;
    return 1;
}
