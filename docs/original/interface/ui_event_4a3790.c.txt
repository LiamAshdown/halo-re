// ui_event_4a3790  (not a Ghidra function; ui_event_function_table[161])
// address 0x4a3790, size 215 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a54 (index 161); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a3790.
// WRITTEN 2026-09-28 from objdump 0x4a3790..0x4a3866: for a selected variant: the first spinner lists of the first
//   three children show byte +0x6c (0..3, else 1), dword +0x70 (0x96 -> 1, 0x12c -> 2, 0x1c2 -> 3, else 0) and byte
//   +0x74 != 0. Returns 1 (AL keeps the 1 loaded before the test).
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
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

uint8_t ui_event_4a3790(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = saved_item_working_copy;
    widget_instance *group;
    int32_t time;

    if ((selected_saved_item & 0xf) != 1) {
        return 1;
    }
    group = widget->first_child;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->friendly_fire <= 3 ? ((struct game_variant *)variant)->friendly_fire : 1);
    group = group->next_sibling;
    time = *(int32_t *)(variant + 0x70);
    first_list_child(group)->selection_index = (int16_t)(time == 0x96 ? 1 : time == 0x12c ? 2 : time == 0x1c2 ? 3 : 0);
    first_list_child(group->next_sibling)->selection_index = (int16_t)(((struct game_variant *)variant)->team_autobalance != 0);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
