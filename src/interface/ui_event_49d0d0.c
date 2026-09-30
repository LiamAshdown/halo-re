// ui_event_49d0d0  (not a Ghidra function; ui_event_function_table[104])
// address 0x49d0d0, size 42 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692970 (index 104); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d0d0.
// WRITTEN 2026-09-28 from objdump 0x49d0d0..0x49d0f9: one local player, marks 0x00719010, then starts the campaign
//   from level one with the same arguments (0x49cfd0); returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "fn_interface.h"

extern int16_t local_player_count; // 0x006894b8
extern uint8_t save_in_progress_00719010; // 0x00719010


uint8_t ui_event_49d0d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    local_player_count = 1;
    save_in_progress_00719010 = 1;
    ui_start_campaign_from_level_one(widget, event);
    return 1;
}
