// ui_event_4a3870  (not a Ghidra function; ui_event_function_table[162])
// address 0x4a3870, size 221 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a58 (index 162); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a3870.
// WRITTEN 2026-09-28 from objdump 0x4a3870..0x4a394c: for a selected variant, the inverse of 0x4a3790 from the
//   grandparent's first three children: byte +0x6c = selection (0..3, else 1), dword +0x70 = 0x96 / 0x12c / 0x1c2 for
//   selection 1..3 (else 0), byte +0x74 = (selection == 1). Returns 1.
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

uint8_t ui_event_4a3870(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = saved_item_working_copy;
    widget_instance *group;
    int16_t selection;

    if ((selected_saved_item & 0xf) != 1) {
        return 1;
    }
    group = widget->parent->parent->first_child;
    selection = first_list_child(group)->selection_index;
    variant[0x6c] = (uint8_t)(selection >= 0 && selection <= 3 ? selection : 1);
    group = group->next_sibling;
    switch (first_list_child(group)->selection_index) {
    case 1:
        *(int32_t *)(variant + 0x70) = 0x96;
        break;
    case 2:
        *(int32_t *)(variant + 0x70) = 0x12c;
        break;
    case 3:
        *(int32_t *)(variant + 0x70) = 0x1c2;
        break;
    default:
        *(int32_t *)(variant + 0x70) = 0;
        break;
    }
    variant[0x74] = (uint8_t)(first_list_child(group->next_sibling)->selection_index == 1);
    return 1;
}
