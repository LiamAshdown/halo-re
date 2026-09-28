// ui_event_4a0a80  (not a Ghidra function; ui_event_function_table[63])
// address 0x4a0a80, size 91 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928cc (index 63); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a0a80.
// WRITTEN 2026-09-28 from objdump 0x4a0a80..0x4a0ada: for a selected profile, stores the ui list id of the widget's
//   committed selection (+0x3c; -1 when out of range) as the profile word +0x11a and returns 1; 0 without a profile.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern int32_t ui_list_current; // 0x00692c04
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)

static int32_t list_item_id(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].id;
    }
    return -1;
}

uint8_t ui_event_4a0a80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    int32_t id = list_item_id(*(int16_t *)((uint8_t *)widget + 0x3c));

    if (profile == 0) {
        return 0;
    }
    *(int16_t *)(profile + 0x11a) = (int16_t)id;
    return 1;
}
