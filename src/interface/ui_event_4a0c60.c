// ui_event_4a0c60  (not a Ghidra function; ui_event_function_table[68])
// address 0x4a0c60, size 191 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928e0 (index 68); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a0c60.
// WRITTEN 2026-09-28 from objdump 0x4a0c60..0x4a0d1e: for a selected profile: the first spinner list of the first
//   child shows profile byte +0x12d (0..3, else 0), that of the second child byte +0x12c (0..4, else 0). Returns 1,
//   or 0 without a profile.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself

static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

uint8_t ui_event_4a0c60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    widget_instance *list;

    if (profile == 0) {
        return 0;
    }
    list = first_list_child(widget->first_child);
    list->selection_index = (int16_t)(profile[0x12d] <= 3 ? profile[0x12d] : 0);
    list = first_list_child(widget->first_child->next_sibling);
    list->selection_index = (int16_t)(profile[0x12c] <= 4 ? profile[0x12c] : 0);
    return 1;
}
