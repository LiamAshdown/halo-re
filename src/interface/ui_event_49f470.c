// ui_event_49f470  (not a Ghidra function; ui_event_function_table[49])
// address 0x49f470, size 228 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692894 (index 49); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49f470.
// WRITTEN 2026-09-28 from objdump 0x49f470..0x49f553: for a selected variant, from the grandparent's first three
//   children's spinner lists: selection 0..2 sets dword +0x3c to 0..2; the second list's 0 / 1 / 2 set flags (+0x38)
//   to (flags & ~0x40) | 1, flags | 0x41, flags & ~0x41; the third list's 0 / 1 set / clear flag bit 1. Other
//   selections leave the value. Returns 1, or 0 without a variant.
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

uint8_t ui_event_49f470(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    uint32_t *flags;

    if (variant == 0) {
        return 0;
    }
    flags = (uint32_t *)(variant + 0x38);
    group = widget->parent->parent->first_child;
    switch (first_list_child(group)->selection_index) {
    case 0:
        ((struct game_variant *)variant)->objective_indicator = 0;
        break;
    case 1:
        ((struct game_variant *)variant)->objective_indicator = 1;
        break;
    case 2:
        ((struct game_variant *)variant)->objective_indicator = 2;
        break;
    }
    group = group->next_sibling;
    switch (first_list_child(group)->selection_index) {
    case 0:
        *flags = (*flags & ~0x40u) | 1;
        break;
    case 1:
        *flags |= 0x41;
        break;
    case 2:
        *flags &= ~0x41u;
        break;
    }
    switch (first_list_child(group->next_sibling)->selection_index) {
    case 0:
        *flags |= 2;
        break;
    case 1:
        *flags &= ~2u;
        break;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
