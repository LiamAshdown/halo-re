// ui_event_4b4c40  (not a Ghidra function; ui_event_function_table[127])
// address 0x4b4c40, size 8 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006929cc (index 127); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4b4c40.
// WRITTEN 2026-09-28 from objdump 0x4b4c40..0x4b4c47: sets 0x00719444; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t ui_flag_00719444; // 0x00719444, UNSURE (only ever set to 1 here)

uint8_t ui_event_4b4c40(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    ui_flag_00719444 = 1;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
