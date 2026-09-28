// ui_event_49d1a0  (not a Ghidra function; ui_event_function_table[15])
// address 0x49d1a0, size 8 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x0069280c (index 15); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d1a0.
// WRITTEN 2026-09-28 from objdump 0x49d1a0..0x49d1a7: tears down the multiplayer game setup widget; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern void network_game_setup_teardown(void); // 0x495520

uint8_t ui_event_49d1a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_game_setup_teardown();
    return 1;
}
