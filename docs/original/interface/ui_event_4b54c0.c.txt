// ui_event_4b54c0  (not a Ghidra function; ui_event_function_table[154])
// address 0x4b54c0, size 154 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a38 (index 154); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4b54c0.
// WRITTEN 2026-09-28 from objdump 0x4b54c0..0x4b5559: for a selected profile: the first spinner list under the
//   grandparent's first child and under that child's next sibling give sensitivity a / b (selection + 1) of the
//   selected device. Then, at the great-great-grandparent: hides its third child, focuses and shows its second,
//   leaves controls list mode. Returns 1.
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
extern int32_t controls_selected_device; // 0x006953ec
extern uint8_t controls_device_sensitivity_a[]; // 0x007157d4, indexed by device; UNSURE name
extern uint8_t controls_device_sensitivity_b[]; // 0x007157d8, indexed by device; UNSURE name
extern uint8_t controls_menu_list_mode; // 0x00719445, UNSURE name

static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

uint8_t ui_event_4b54c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *screen;
    widget_instance *second;
    widget_instance *third;

    if ((selected_saved_item & 0xf) == 0) {
        widget_instance *group = widget->parent->parent->first_child;
        int32_t device = controls_selected_device;

        controls_device_sensitivity_a[device] = (uint8_t)(first_list_child(group)->selection_index + 1);
        controls_device_sensitivity_b[device] = (uint8_t)(first_list_child(group->next_sibling)->selection_index + 1);
    }
    screen = widget->parent->parent->parent->parent;
    second = screen->first_child->next_sibling;
    third = second->next_sibling;
    third->state = 0;
    third->hidden = 1;
    screen->focused_child = second;
    second->state = 1;
    second->hidden = 0;
    controls_menu_list_mode = 0;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
