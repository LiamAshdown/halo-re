// ui_event_4a4190  (not a Ghidra function; ui_event_function_table[172])
// address 0x4a4190, size 8 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a80 (index 172); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a4190.
// WRITTEN 2026-09-28 from objdump 0x4a4190..0x4a4197: launches the autopatch updater; returns 1. Also slot 181.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t autopatch_launch_updater(void); // 0x577310

uint8_t ui_event_4a4190(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    autopatch_launch_updater();
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
