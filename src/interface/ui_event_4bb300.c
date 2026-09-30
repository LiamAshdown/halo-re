// ui_event_4bb300  (not a Ghidra function; ui_event_function_table[173])
// address 0x4bb300, size 83 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a84 (index 173); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4bb300.
// WRITTEN 2026-09-28 from objdump 0x4bb300..0x4bb352: unless 0x007196d1 is set, the profile's gamma byte (+0xa76;
//   the binary reads address 0xa76 without a profile) becomes the rasterizer gamma exponent and the gamma is applied.
//   Then empties the lists (+0x44) of the second child of the first child and of the second child of the second
//   child's first child. Returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "fn_rasterizer.h"

extern uint8_t ui_flag_007196d1; // 0x007196d1, UNSURE (cleared by 0x4bb290, gates the gamma apply in 0x4bb300)
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern int32_t rasterizer_gamma_exponent; // 0x0071d1e0


uint8_t ui_event_4bb300(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *first = widget->first_child;

    if (ui_flag_007196d1 == 0) {
        uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;

        rasterizer_gamma_exponent = profile[0xa76];
        chimera__gamma();
    }
    first->first_child->next_sibling->list_items = 0;
    first->next_sibling->first_child->next_sibling->list_items = 0;
    return 1;
}
