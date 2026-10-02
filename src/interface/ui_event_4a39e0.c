// ui_event_4a39e0  (not a Ghidra function; ui_event_function_table[167])
// address 0x4a39e0, size 129 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a6c (index 167); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a39e0.
// WRITTEN 2026-09-28 from objdump 0x4a39e0..0x4a3a60: for a selected profile: the first spinner list (type 2) under
//   the grandparent's first child gives a selection clamped to 0..4 stored as profile byte +0xfc0; the words of
//   0x00719210 / 0x00719214 go to +0x1002 / +0x1004. Returns 1, or 0 without a profile. (No spinner list: the binary
//   reads through null; so does this.)
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
extern uint32_t network_game_option_a_00719210; // 0x00719210, TYPES-GAP
extern uint32_t network_game_option_b_00719214; // 0x00719214, TYPES-GAP

static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

uint8_t ui_event_4a39e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    int16_t selection;

    if (profile == 0) {
        return 0;
    }
    selection = first_list_child(widget->parent->parent->first_child)->selection_index;
    if (selection < 0) {
        selection = 0;
    } else if (selection > 4) {
        selection = 4;
    }
    profile[0xfc0] = (uint8_t)selection;
    *(uint16_t *)(profile + 0x1002) = (uint16_t)network_game_option_a_00719210;
    *(uint16_t *)(profile + 0x1004) = (uint16_t)network_game_option_b_00719214;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
