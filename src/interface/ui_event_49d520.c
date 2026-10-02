// ui_event_49d520  (not a Ghidra function; ui_event_function_table[22])
// address 0x49d520, size 17 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692828 (index 22); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d520.
// WRITTEN 2026-09-28 from objdump 0x49d520..0x49d530: one local player and marks 0x00719010; returns 1 (AL from the
//   stored EAX = 1).
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t local_player_count; // 0x006894b8
extern uint8_t save_in_progress_00719010; // 0x00719010

uint8_t ui_event_49d520(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    local_player_count = 1;
    save_in_progress_00719010 = 1;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
