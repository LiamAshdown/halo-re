// ui_event_4a2490  (not a Ghidra function; ui_event_function_table[123])
// address 0x4a2490, size 45 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006929bc (index 123); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a2490.
// WRITTEN 2026-09-28 from objdump 0x4a2490..0x4a24bc: when the selected saved item is a profile, fills the widget
//   as a controls input row from it and returns 1; otherwise 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern void ui_controls_populate_input_row(widget_instance *widget, const uint8_t *profile_record); // 0x4a22e0, blam-cc: EAX widget, EDI profile_record

uint8_t ui_event_4a2490(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    const uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;

    if (profile == 0) {
        return 0;
    }
    ui_controls_populate_input_row(widget, profile);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
