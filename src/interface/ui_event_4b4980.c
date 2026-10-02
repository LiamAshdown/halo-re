// ui_event_4b4980  (not a Ghidra function; ui_event_function_table[113])
// address 0x4b4980, size 366 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692994 (index 113); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4b4980.
// WRITTEN 2026-09-28 from objdump 0x4b4980..0x4b4aed: resets 0x006953e8 to -1, hides the third child (state 0,
//   hidden 1), focuses and shows the second, leaves controls list mode. For a selected profile (else returns 0)
//   copies its controls section back into the live block at 0x006b3a48 (the inverse of 0x4b4af0), clears 0x00719444,
//   rebuilds the device label table and points the second child's nested list (first -> first -> next) at it
//   (selection 0, 0x00719440 items, data 0x006932e8), then refreshes the binding rows from page 0. Returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t controls_capture_row; // 0x006953e8, UNSURE identity
extern uint8_t controls_menu_list_mode; // 0x00719445, UNSURE name
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern uint8_t input_controls_live_006b3a48[0x890]; // 0x006b3a48, UNSURE: the live controls configuration the profile copy mirrors
extern uint8_t ui_flag_00719444; // 0x00719444, UNSURE (only ever set to 1 here)
extern void controls_build_device_label_table(void); // 0x4b4890
extern int32_t controls_device_label_count; // 0x00719440
extern uint8_t controls_device_labels[]; // 0x006932e8
extern int32_t controls_binding_list_refresh_rows(widget_instance *widget, int32_t page); // 0x4b4790, blam-cc: EAX widget

uint8_t ui_event_4b4980(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *second = widget->first_child->next_sibling;
    widget_instance *third = second->next_sibling;
    uint8_t *live = input_controls_live_006b3a48;
    uint8_t *profile;
    widget_instance *list;

    controls_capture_row = -1;
    third->state = 0;
    third->hidden = 1;
    widget->focused_child = second;
    second->state = 1;
    second->hidden = 0;
    controls_menu_list_mode = 0;
    if ((selected_saved_item & 0xf) != 0) {
        return 0;
    }
    profile = saved_item_working_copy;
    memcpy(live + 0x220, profile + 0x134, 0xda);
    memcpy(live + 0x10, profile + 0x20e, 0x10);
    memcpy(live + 0x880, profile + 0x21e, 0xc);
    memcpy(live + 0x380, profile + 0x22a, 0x100);
    memcpy(live + 0x0, profile + 0x32a, 0x10);
    memcpy(live + 0x20, profile + 0x33a, 0x200);
    memcpy(live + 0x480, profile + 0x53a, 0x400);
    ui_flag_00719444 = 0;
    memcpy(live + 0x2fc, profile + 0x956, 4);
    memcpy(live + 0x88c, profile + 0x95a, 4);
    controls_build_device_label_table();
    list = second->first_child->first_child->next_sibling;
    list->selection_index = 0;
    list->item_count = (uint16_t)controls_device_label_count;
    list->list_items = controls_device_labels;
    controls_binding_list_refresh_rows(second, 0);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
