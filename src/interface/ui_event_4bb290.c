// ui_event_4bb290  (not a Ghidra function; ui_event_function_table[114])
// address 0x4bb290, size 102 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692998 (index 114); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4bb290.
// WRITTEN 2026-09-28 from objdump 0x4bb290..0x4bb2f5: clears 0x007196d1. With 0x007196d2 set: closes the root
//   widget (auto close 1 ms, fade 0, state 0), clears 0x007196d2, returns 1. Otherwise a selected profile populates
//   the video options menu from the working copy (returns 1); else 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern uint8_t ui_flag_007196d1; // 0x007196d1, UNSURE (cleared by 0x4bb290, gates the gamma apply in 0x4bb300)
extern uint8_t ui_flag_007196d2; // 0x007196d2, UNSURE (only ever set to 1 here)
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern void video_options_menu_populate(uint8_t *context, uint8_t *settings); // 0x4baec0

uint8_t ui_event_4bb290(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    ui_flag_007196d1 = 0;
    if (ui_flag_007196d2 != 0) {
        widget_instance *root = widget;

        while (root->parent != 0) {
            root = root->parent;
        }
        root->milliseconds_to_auto_close = 1;
        root->milliseconds_auto_close_fade = 0;
        root->state = 0;
        ui_flag_007196d2 = 0;
        return 1;
    }
    if ((selected_saved_item & 0xf) != 0) {
        return 0;
    }
    video_options_menu_populate((uint8_t *)widget, saved_item_working_copy);
    return 1;
}
