// ui_event_4a0d60  (not a Ghidra function; ui_event_function_table[69])
// address 0x4a0d60, size 303 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928e4 (index 69); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a0d60.
// WRITTEN 2026-09-28 from objdump 0x4a0d60..0x4a0e8e: for a selected profile, the first spinner lists of the first
//   five children show: byte +0x12f == 0 ? 1 : 0; byte +0x12e 1..10 as 0..9 (else 0); byte +0x130 == 1; byte +0x131
//   == 0; byte +0x132 == 0. Returns 1, or 0 without a profile.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
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

uint8_t ui_event_4a0d60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    widget_instance *group;
    uint8_t value;

    if (profile == 0) {
        return 0;
    }
    group = widget->first_child;
    first_list_child(group)->selection_index = (int16_t)(profile[0x12f] == 0 ? 1 : 0);
    group = group->next_sibling;
    value = profile[0x12e];
    first_list_child(group)->selection_index = (int16_t)(value > 0 && value <= 10 ? value - 1 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(profile[0x130] == 1);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(profile[0x131] == 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(profile[0x132] == 0);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
