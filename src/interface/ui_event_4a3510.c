// ui_event_4a3510  (not a Ghidra function; ui_event_function_table[159])
// address 0x4a3510, size 48 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a4c (index 159); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a3510.
// WRITTEN 2026-09-28 from objdump 0x4a3510..0x4a353f: when the selected saved item is a variant (low nibble 1),
//   copies the dwords at 0x00879f34, 0x00879f38 and 0x00719208 into the working copy +0x60..+0x6b (0x00714ee0);
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

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern uint32_t unknown_00879f34; // 0x00879f34, UNSURE
extern uint32_t unknown_00879f38; // 0x00879f38, UNSURE
extern uint32_t unknown_00719208; // 0x00719208, UNSURE

uint8_t ui_event_4a3510(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if ((selected_saved_item & 0xf) == 1) {
        uint32_t *out = (uint32_t *)(saved_item_working_copy + 0x60);

        out[0] = unknown_00879f34;
        out[1] = unknown_00879f38;
        out[2] = unknown_00719208;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
