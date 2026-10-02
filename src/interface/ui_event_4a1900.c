// ui_event_4a1900  (not a Ghidra function; ui_event_function_table[94])
// address 0x4a1900, size 61 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692948 (index 94); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1900.
// WRITTEN 2026-09-28 from objdump 0x4a1900..0x4a193c: when the selected saved item is neither a profile (0) nor a
//   variant (1), finds the root widget and closes it (+0x1c auto close = 1 ms, +0x10 state = 0) and returns 0;
//   otherwise returns 1.
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

uint8_t ui_event_4a1900(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *root = widget;
    int32_t kind = selected_saved_item & 0xf;

    if (kind == 0 || kind == 1) {
        return 1;
    }
    while (root->parent != 0) {
        root = root->parent;
    }
    root->milliseconds_to_auto_close = 1;
    root->state = 0;
    return 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
