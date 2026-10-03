// ui_event_4a0700  (not a Ghidra function; ui_event_function_table[59])
// address 0x4a0700, size 224 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928bc (index 59); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a0700.
// WRITTEN 2026-09-28 from objdump 0x4a0700..0x4a07df: for a selected variant: the first spinner list of the first
//   child shows dword +0x3c (1 or 2, else 0); that of the second shows 2 when flag bit 0 (+0x38) is clear, else flag
//   bit 6; that of the third shows 1 when flag bit 1 is clear, else 0. Returns 1, or 0 without a variant.
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

static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

uint8_t ui_event_4a0700(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    int32_t value;
    uint32_t flags;

    if (variant == 0) {
        return 0;
    }
    group = widget->first_child;
    value = ((struct game_variant *)variant)->objective_indicator;
    first_list_child(group)->selection_index = (int16_t)(value == 1 || value == 2 ? value : 0);
    group = group->next_sibling;
    flags = ((struct game_variant *)variant)->flags;
    first_list_child(group)->selection_index = (int16_t)((flags & 1) == 0 ? 2 : (flags >> 6) & 1);
    first_list_child(group->next_sibling)->selection_index = (int16_t)(((((struct game_variant *)variant)->flags >> 1) & 1) == 0 ? 1 : 0);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
