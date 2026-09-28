// ui_event_49d100  (not a Ghidra function; ui_event_function_table[11])
// address 0x49d100, size 31 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006927fc (index 11); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d100.
// WRITTEN 2026-09-28 from objdump 0x49d100..0x49d11e: clears 0x0071973c and 0x0071974f, resets the split screen
//   quit prompt string to -1, sets 0x0071973a; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern uint8_t network_join_error_reason; // 0x0071973c
extern uint8_t main_globals_byte_0071974f; // 0x0071974f
extern uint16_t split_screen_quit_prompt_string; // 0x00719754, word stores
extern uint8_t main_globals_byte_0071973a; // 0x0071973a

uint8_t ui_event_49d100(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    network_join_error_reason = 0;
    main_globals_byte_0071974f = 0;
    split_screen_quit_prompt_string = 0xffff;
    main_globals_byte_0071973a = 1;
    return 1;
}
