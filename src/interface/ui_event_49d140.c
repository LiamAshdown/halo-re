// ui_event_49d140  (not a Ghidra function; ui_event_function_table[13])
// address 0x49d140, size 24 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692804 (index 13); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d140.
// WRITTEN 2026-09-28 from objdump 0x49d140..0x49d157: resets the split screen quit prompt string to -1, clears
//   0x0071973c, arms the quit prompt (0x00719757); returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint16_t split_screen_quit_prompt_string; // 0x00719754, word stores
extern uint8_t network_join_error_reason; // 0x0071973c
extern uint8_t split_screen_quit_prompt_armed; // 0x00719757

uint8_t ui_event_49d140(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    split_screen_quit_prompt_string = 0xffff;
    network_join_error_reason = 0;
    split_screen_quit_prompt_armed = 1;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
