// ui_event_4a0fb0  (not a Ghidra function; ui_event_function_table[71])
// address 0x4a0fb0, size 305 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928ec (index 71); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a0fb0.
// WRITTEN 2026-09-28 from objdump 0x4a0fb0..0x4a10e0: for a selected profile, the inverse of 0x4a0d60: selection 0
//   / 1 of the first list sets byte +0x12f to 1 / 0; 0..9 of the second sets +0x12e to selection + 1; 0 / 1 of the
//   third sets +0x130 to 0 / 1; of the fourth +0x131 to 1 / 0; of the fifth +0x132 to 1 / 0. Other selections leave
//   them. Returns 1, or 0 without a profile.
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

uint8_t ui_event_4a0fb0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    widget_instance *group;
    int16_t selection;

    if (profile == 0) {
        return 0;
    }
    group = widget->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        profile[0x12f] = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 9) {
        profile[0x12e] = (uint8_t)(selection + 1);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        profile[0x130] = (uint8_t)selection;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        profile[0x131] = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        profile[0x132] = (uint8_t)(selection == 0);
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
