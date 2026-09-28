// ui_event_4b4af0  (not a Ghidra function; ui_event_function_table[126])
// address 0x4b4af0, size 333 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006929c8 (index 126); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4b4af0.
// WRITTEN 2026-09-28 from objdump 0x4b4af0..0x4b4c3c: unless 0x00719444 is set, a selected profile's controls
//   section is copied from the live controls block at 0x006b3a48 (working copy +0x134..+0x95d: keyboard scan table,
//   mouse/gamepad tables and scalars). Then empties the second child's nested list (first child -> next -> first ->
//   first -> next: +0x48 = 0, +0x44 = 0), clears 0x00719444 and, when 0x006953e8 is not -1, drops input capture bit 3
//   (0x00712542), zeroes the 0x280 bytes at 0x00712544 and resets 0x006953e8 to -1. Returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>

extern uint8_t ui_flag_00719444; // 0x00719444, UNSURE (only ever set to 1 here)
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern uint8_t input_controls_live_006b3a48[0x890]; // 0x006b3a48, UNSURE: the live controls configuration the profile copy mirrors
extern int32_t controls_capture_row; // 0x006953e8, UNSURE identity
extern uint8_t controls_input_capture_flags; // 0x00712542, UNSURE
extern uint8_t controls_input_capture_buffer[0xa0 * 4]; // 0x00712544, UNSURE identity (zeroed 0xa0 dwords)

uint8_t ui_event_4b4af0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    const uint8_t *live = input_controls_live_006b3a48;
    widget_instance *list;

    if (ui_flag_00719444 == 0 && (selected_saved_item & 0xf) == 0) {
        uint8_t *profile = saved_item_working_copy;

        memcpy(profile + 0x134, live + 0x220, 0xda);
        memcpy(profile + 0x20e, live + 0x10, 0x10);
        memcpy(profile + 0x21e, live + 0x880, 0xc);
        memcpy(profile + 0x22a, live + 0x380, 0x100);
        memcpy(profile + 0x32a, live + 0x0, 0x10);
        memcpy(profile + 0x33a, live + 0x20, 0x200);
        memcpy(profile + 0x53a, live + 0x480, 0x400);
        memcpy(profile + 0x956, live + 0x2fc, 4);
        memcpy(profile + 0x95a, live + 0x88c, 4);
    }
    list = widget->first_child->next_sibling->first_child->first_child->next_sibling;
    list->item_count = 0;
    list->list_items = 0;
    ui_flag_00719444 = 0;
    if (controls_capture_row != -1) {
        controls_input_capture_flags &= 0xf7;
        memset(controls_input_capture_buffer, 0, sizeof(controls_input_capture_buffer));
        controls_capture_row = -1;
    }
    return 1;
}
