// ui_event_4a1700  (not a Ghidra function; ui_event_function_table[90])
// address 0x4a1700, size 52 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692938 (index 90); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1700.
// WRITTEN 2026-09-28 from objdump 0x4a1700..0x4a1733: event word 1 equal to profile_slot_id[0] shows error 0x12
//   (player -1, modal, not an error), sets *out_handled and returns 0; otherwise stores it in profile_slot_id[1] and
//   returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t profile_slot_id[]; // 0x00714dde
extern void display_error(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error); // 0x498f20

uint8_t ui_event_4a1700(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (event[1] == profile_slot_id[0]) {
        display_error(0x12, -1, 1, 0);
        *out_handled = 1;
        return 0;
    }
    profile_slot_id[1] = event[1];
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
