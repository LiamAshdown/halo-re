// ui_event_4a21c0  (not a Ghidra function; ui_event_function_table[122])
// address 0x4a21c0, size 165 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006929b8 (index 122); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a21c0.
// WRITTEN 2026-09-28 from objdump 0x4a21c0..0x4a2264: for a selected profile: under the grandparent's first three
//   children, the first spinner list (type 2) of each gives profile bytes +0x954 (selection + 1), +0x955 (selection +
//   1) and +0x12f (selection == 1). Returns 1, or 0 without a profile.
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

uint8_t ui_event_4a21c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    widget_instance *group;

    if (profile == 0) {
        return 0;
    }
    group = widget->parent->parent->first_child;
    profile[0x954] = (uint8_t)(first_list_child(group)->selection_index + 1);
    group = group->next_sibling;
    profile[0x955] = (uint8_t)(first_list_child(group)->selection_index + 1);
    group = group->next_sibling;
    profile[0x12f] = (uint8_t)(first_list_child(group)->selection_index == 1);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
