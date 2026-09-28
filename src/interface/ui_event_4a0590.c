// ui_event_4a0590  (not a Ghidra function; ui_event_function_table[58])
// address 0x4a0590, size 206 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928b8 (index 58); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a0590.
// WRITTEN 2026-09-28 from objdump 0x4a0590..0x4a065d: for a selected variant: the first spinner list of the first
//   child selects 1 when variant flag bit 2 (+0x38) is clear, else 0; that of the second child shows the dword +0x5c
//   (0..13, else 0); that of the third child shows flag bit 5. Returns 1, or 0 without a variant.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "game.h"

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

uint8_t ui_event_4a0590(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    uint32_t flags;
    uint32_t value;

    if (variant == 0) {
        return 0;
    }
    flags = ((struct game_variant *)variant)->flags;
    group = widget->first_child;
    first_list_child(group)->selection_index = (int16_t)(((flags >> 2) & 1) != 0 ? 0 : 1);
    group = group->next_sibling;
    value = *(uint32_t *)&((struct game_variant *)variant)->starting_equipment;
    first_list_child(group)->selection_index = (int16_t)(value <= 0xd ? value : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)((((struct game_variant *)variant)->flags >> 5) & 1);
    return 1;
}
