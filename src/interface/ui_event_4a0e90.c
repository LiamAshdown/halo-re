// ui_event_4a0e90  (not a Ghidra function; ui_event_function_table[70])
// address 0x4a0e90, size 241 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928e8 (index 70); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a0e90.
// WRITTEN 2026-09-28 from objdump 0x4a0e90..0x4a0f80: for a selected profile: the first spinner list of the first
//   child writes profile byte +0x12d (selection 0..3), that of the second byte +0x12c (selection 0..4); other
//   selections leave them. Returns 1, or 0 without a profile.
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

uint8_t ui_event_4a0e90(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    int16_t selection;

    if (profile == 0) {
        return 0;
    }
    selection = first_list_child(widget->first_child)->selection_index;
    if (selection >= 0 && selection <= 3) {
        profile[0x12d] = (uint8_t)selection;
    }
    selection = first_list_child(widget->first_child->next_sibling)->selection_index;
    if (selection >= 0 && selection <= 4) {
        profile[0x12c] = (uint8_t)selection;
    }
    return 1;
}
