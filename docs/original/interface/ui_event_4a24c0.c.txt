// ui_event_4a24c0  (not a Ghidra function; ui_event_function_table[124])
// address 0x4a24c0, size 465 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006929c0 (index 124); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a24c0.
// WRITTEN 2026-09-28 from objdump 0x4a24c0..0x4a2690: for a selected profile, from the grandparent's first seven
//   children's spinner lists: bytes +0xb78 / +0xb79 / +0xb7a = selection clamped to 0..10, +0xb7c = (selection != 0),
//   +0xb7d = selection clamped to 0..2, +0xb7b = 1 when the sixth list's selection is at least 1 and +0xb7c is set
//   (else 0), +0xb7f = seventh clamped to 0..2. Returns 1, or 0 without a profile.
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

static uint8_t clamp_selection(widget_instance *group, int16_t maximum)
{
    int16_t selection = first_list_child(group)->selection_index;

    return (uint8_t)(selection < 0 ? 0 : selection > maximum ? maximum : selection);
}

uint8_t ui_event_4a24c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    widget_instance *group;
    int16_t selection;

    if (profile == 0) {
        return 0;
    }
    group = widget->parent->parent->first_child;
    profile[0xb78] = clamp_selection(group, 10);
    group = group->next_sibling;
    profile[0xb79] = clamp_selection(group, 10);
    group = group->next_sibling;
    profile[0xb7a] = clamp_selection(group, 10);
    group = group->next_sibling;
    profile[0xb7c] = (uint8_t)(first_list_child(group)->selection_index != 0);
    group = group->next_sibling;
    profile[0xb7d] = clamp_selection(group, 2);
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    profile[0xb7b] = (uint8_t)(selection >= 1 && profile[0xb7c] != 0);
    group = group->next_sibling;
    profile[0xb7f] = clamp_selection(group, 2);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
