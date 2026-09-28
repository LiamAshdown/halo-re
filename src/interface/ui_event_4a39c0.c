// ui_event_4a39c0  (not a Ghidra function; ui_event_function_table[166])
// address 0x4a39c0, size 32 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a68 (index 166); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a39c0.
// WRITTEN 2026-09-28 from objdump 0x4a39c0..0x4a39df: passes the widget and the working copy when the selected
//   saved item is a profile (0 otherwise) to 0x4a3960 and returns its result.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern uint8_t ui_network_game_options_populate(widget_instance *widget, const uint8_t *options_record); // 0x4a3960, blam-cc: ECX widget, ESI options_record

uint8_t ui_event_4a39c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    const uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;

    return ui_network_game_options_populate(widget, profile);
}
