// ui_event_4a47b0  (not a Ghidra function; ui_event_function_table[179])
// address 0x4a47b0, size 8 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a9c (index 179); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a47b0.
// WRITTEN 2026-09-28 from objdump 0x4a47b0..0x4a47b7: saves a new checkpoint; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "fn_saved_games.h"


uint8_t ui_event_4a47b0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    game_checkpoint_save_new();
    return 1;
}
