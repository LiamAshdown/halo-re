// ui_event_4a02a0  (not a Ghidra function; ui_event_function_table[57])
// address 0x4a02a0, size 630 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928b4 (index 57); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a02a0.
// WRITTEN 2026-09-28 from objdump 0x4a02a0..0x4a0515: for a selected variant, the first spinner lists of the first
//   eight children show: dword +0x50 1 / 3 / 5 as 1..3 (else 0); trunc(float +0x54 * -10.0) -10 / -15 / -20 / -30 /
//   -40 as 1..5 (else 0); flag bit 3 (+0x38); dword +0x48 0x96 / 0x12c / 0x1c2 as 1..3 (else 0); the same for +0x44;
//   byte +0x40 == 0; flag bit 4 clear; dword +0x4c as +0x48. Returns 1, or 0 without a variant.
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

uint8_t ui_event_4a02a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    int32_t value;
    uint32_t flags;

    if (variant == 0) {
        return 0;
    }
    flags = ((struct game_variant *)variant)->flags;
    group = widget->first_child;
    value = ((struct game_variant *)variant)->lives_per_round;
    first_list_child(group)->selection_index = (int16_t)(value == 1 ? 1 : value == 3 ? 2 : value == 5 ? 3 : 0);
    group = group->next_sibling;
    value = (int32_t)((double)((struct game_variant *)variant)->health * -10.0); // fmul by -10.0f (0x00672da4), _ftol
    first_list_child(group)->selection_index = (int16_t)(value == -10 ? 1 : value == -15 ? 2 : value == -20 ? 3 :
        value == -30 ? 4 : value == -40 ? 5 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)((flags >> 3) & 1);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->respawn_time;
    first_list_child(group)->selection_index = (int16_t)(value == 0x96 ? 1 : value == 0x12c ? 2 : value == 0x1c2 ? 3 : 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->respawn_time_growth;
    first_list_child(group)->selection_index = (int16_t)(value == 0x96 ? 1 : value == 0x12c ? 2 : value == 0x1c2 ? 3 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->odd_man_out == 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((flags >> 4) & 1) == 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->suicide_penalty;
    first_list_child(group)->selection_index = (int16_t)(value == 0x96 ? 1 : value == 0x12c ? 2 : value == 0x1c2 ? 3 : 0);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
