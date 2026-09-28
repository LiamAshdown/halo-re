// ui_event_49d120  (not a Ghidra function; ui_event_function_table[12])
// address 0x49d120, size 31 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692800 (index 12); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d120.
// WRITTEN 2026-09-28 from objdump 0x49d120..0x49d13e: clears 0x0071973c and 0x0071974f, resets the split screen
//   quit prompt string to -1, sets 0x00719738; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern uint8_t main_globals_byte_0071973c; // 0x0071973c
extern uint8_t main_globals_byte_0071974f; // 0x0071974f
extern uint16_t split_screen_quit_prompt_string; // 0x00719754, word stores
extern uint8_t unknown_00719738; // 0x00719738, UNSURE

uint8_t ui_event_49d120(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    main_globals_byte_0071973c = 0;
    main_globals_byte_0071974f = 0;
    split_screen_quit_prompt_string = 0xffff;
    unknown_00719738 = 1;
    return 1;
}
