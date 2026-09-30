// ui_event_49cdd0  (not a Ghidra function; ui_event_function_table[7])
// address 0x49cdd0, size 37 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006927ec (index 7); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49cdd0.
// WRITTEN 2026-09-28 from objdump 0x49cdd0..0x49cdf4: clears the ten level select entries (0x50 bytes at
//   0x00719018), empties the widget list (+0x44, +0x48), frees every ui list; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "fn_interface.h"
#include <string.h>

extern uint8_t level_select_entries[0x50]; // 0x00719018


uint8_t ui_event_49cdd0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    memset(level_select_entries, 0, 0x50);
    widget->list_items = 0;
    widget->item_count = 0;
    ui_list_free_all();
    return 1;
}
