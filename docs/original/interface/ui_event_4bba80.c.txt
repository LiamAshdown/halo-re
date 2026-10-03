// ui_event_4bba80  (not a Ghidra function; ui_event_function_table[147])
// address 0x4bba80, size 8 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a1c (index 147); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4bba80.
// WRITTEN 2026-09-28 from objdump 0x4bba80..0x4bba87: sets 0x007196d2; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t ui_flag_007196d2; // 0x007196d2, UNSURE (only ever set to 1 here)

uint8_t ui_event_4bba80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    ui_flag_007196d2 = 1;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
