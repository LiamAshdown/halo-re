// ui_event_49f560  (not a Ghidra function; ui_event_function_table[50])
// address 0x49f560, size 97 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692898 (index 50); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49f560.
// WRITTEN 2026-09-28 from objdump 0x49f560..0x49f5c0: for a selected variant, maps its game type (+0x30) 1..5 to
//   selection 0, 2, 3, 1, 4 (0 otherwise), then focuses that child (walking next siblings, stopping at the end);
//   returns 1, or 0 without a variant.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself

uint8_t ui_event_49f560(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *child;
    int32_t i;

    if (variant == 0) {
        return 0;
    }
    switch (((struct game_variant *)variant)->game_engine_index) {
    case 2:
        widget->selection_index = 2;
        break;
    case 3:
        widget->selection_index = 3;
        break;
    case 4:
        widget->selection_index = 1;
        break;
    case 5:
        widget->selection_index = 4;
        break;
    default:
        widget->selection_index = 0;
        break;
    }
    child = widget->first_child;
    for (i = 0; i < widget->selection_index && child != 0; i++) {
        child = child->next_sibling;
    }
    widget->focused_child = child;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
