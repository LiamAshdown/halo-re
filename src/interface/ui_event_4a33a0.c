// ui_event_4a33a0  (not a Ghidra function; ui_event_function_table[158])
// address 0x4a33a0, size 365 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a48 (index 158); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a33a0.
// WRITTEN 2026-09-28 from objdump 0x4a33a0..0x4a350c: for a selected variant: 0x0071920c = (byte +0x34 != 0),
//   0x00692b0c = 0, dwords +0x60 / +0x64 / +0x68 go to 0x00879f34 / 0x00879f38 / 0x00719208, and the bind rows are
//   populated from +0x60 (0x4a3180). The first child's spinner list shows +0x68 (0, 0x384, 0x708, 0xa8c, 0xe10,
//   0x1518, 0x2328 -> 0..6, else 0); the second child is shown (hidden 0, state 1) when 0x0071920c is set, else
//   hidden (state 0); the third child's list shows the low nibble of 0x00879f34 when below 9, else 0; 0x00692b08
//   takes the second child's list selection. Returns 1, or 0 without a variant.
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
extern uint8_t variant_teams_enabled_0071920c; // 0x0071920c, UNSURE name (variant byte +0x34 != 0)
extern int32_t variant_team_selection_00692b08; // 0x00692b08, UNSURE name
extern int32_t unknown_00692b0c; // 0x00692b0c, UNSURE
extern uint32_t unknown_00879f34; // 0x00879f34, UNSURE
extern uint32_t unknown_00879f38; // 0x00879f38, UNSURE
extern uint32_t unknown_00719208; // 0x00719208, UNSURE
extern void ui_controls_populate_bind_rows(widget_instance *widget, uint32_t packed); // 0x4a3180

static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

uint8_t ui_event_4a33a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *first;
    widget_instance *second;
    widget_instance *second_list;
    int32_t time;

    if (variant == 0) {
        return 0;
    }
    time = ((struct game_variant *)variant)->time_limit;
    variant_teams_enabled_0071920c = (uint8_t)(variant[0x34] != 0);
    unknown_00692b0c = 0;
    unknown_00879f34 = ((struct game_variant *)variant)->vehicle_set;
    unknown_00879f38 = *(uint32_t *)(variant + 0x64);
    unknown_00719208 = (uint32_t)time;
    ui_controls_populate_bind_rows(widget, ((struct game_variant *)variant)->vehicle_set);
    first = widget->first_child;
    first_list_child(first)->selection_index = (int16_t)(time == 0x384 ? 1 : time == 0x708 ? 2 : time == 0xa8c ? 3 :
        time == 0xe10 ? 4 : time == 0x1518 ? 5 : time == 0x2328 ? 6 : 0);
    second = first->next_sibling;
    second_list = first_list_child(second);
    if (variant_teams_enabled_0071920c != 0) {
        second->hidden = 0;
        second->state = 1;
    } else {
        second->hidden = 1;
        second->state = 0;
    }
    first_list_child(second->next_sibling)->selection_index = (int16_t)((unknown_00879f34 & 0xf) < 9 ? (unknown_00879f34 & 0xf) : 0);
    variant_team_selection_00692b08 = second_list->selection_index;
    return 1;
}
