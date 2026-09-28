// ui_event_49f300  (not a Ghidra function; ui_event_function_table[48])
// address 0x49f300, size 312 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692890 (index 48); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49f300.
// WRITTEN 2026-09-28 from objdump 0x49f300..0x49f437: for a selected variant, from the grandparent's first three
//   spinner lists: 0 / 1 set / clear flag bit 2 (+0x38); 0..13 store dword +0x5c; 0 / 1 clear / set flag bit 5. Other
//   selections leave them. Returns 1, or 0 without a variant.
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

uint8_t ui_event_49f300(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    uint32_t *flags;
    int16_t selection;

    if (variant == 0) {
        return 0;
    }
    flags = (uint32_t *)(variant + 0x38);
    group = widget->parent->parent->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection == 0) {
        *flags |= 4;
    } else if (selection == 1) {
        *flags &= ~4u;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 0xd) {
        *(int32_t *)(variant + 0x5c) = selection;
    }
    selection = first_list_child(group->next_sibling)->selection_index;
    if (selection == 0) {
        *flags &= ~0x20u;
    } else if (selection == 1) {
        *flags |= 0x20;
    }
    return 1;
}
