// ui_event_49e220  (not a Ghidra function; ui_event_function_table[40])
// address 0x49e220, size 131 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692870 (index 40); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49e220.
// WRITTEN 2026-09-28 from objdump 0x49e220..0x49e2a2: for a selected variant (low nibble 1): the parent's selection
//   0..4 maps to game type 1 (also setting variant +0x34 = 1), 4, 2, 3, 5 (any other keeps +0x30); a changed game
//   type (+0x30) zeroes the 0x18 bytes at +0x7c. Returns 1, or 0 without a variant.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself

uint8_t ui_event_49e220(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    int32_t type;

    if (variant == 0) {
        return 0;
    }
    switch (widget->parent->selection_index) {
    case 0:
        type = 1;
        variant[0x34] = 1;
        break;
    case 1:
        type = 4;
        break;
    case 2:
        type = 2;
        break;
    case 3:
        type = 3;
        break;
    case 4:
        type = 5;
        break;
    default:
        type = *(int32_t *)(variant + 0x30);
        break;
    }
    if (type != *(int32_t *)(variant + 0x30)) {
        memset(variant + 0x7c, 0, 0x18);
    }
    *(int32_t *)(variant + 0x30) = type;
    return 1;
}
