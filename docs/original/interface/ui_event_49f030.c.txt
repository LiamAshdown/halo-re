// ui_event_49f030  (not a Ghidra function; ui_event_function_table[47])
// address 0x49f030, size 630 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x0069288c (index 47); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49f030.
// WRITTEN 2026-09-28 from objdump 0x49f030..0x49f2a5: for a selected variant, the inverse of 0x4a02a0 from the
//   grandparent's first eight spinner lists: 0..3 set dword +0x50 to 0, 1, 3, 5; 0..5 set float +0x54 to 0.5, 1, 1.5,
//   2, 3, 4; 0 / 1 clear / set flag bit 3 (+0x38); 0..3 set dword +0x48 to 0, 0x96, 0x12c, 0x1c2; the same for +0x44;
//   0 / 1 set byte +0x40 to 1 / 0; 0 / 1 set / clear flag bit 4; and, when an eighth child exists, 0..3 set dword
//   +0x4c like +0x48. Other selections leave them. Returns 1, or 0 without a variant.
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

uint8_t ui_event_49f030(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    static const int32_t kills[] = {0, 1, 3, 5};
    static const float scales[] = {0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f};
    static const int32_t times[] = {0, 0x96, 0x12c, 0x1c2};
    widget_instance *group;
    uint32_t *flags;
    int16_t selection;

    if (variant == 0) {
        return 0;
    }
    flags = (uint32_t *)(variant + 0x38);
    group = widget->parent->parent->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 3) {
        ((struct game_variant *)variant)->lives_per_round = kills[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 5) {
        ((struct game_variant *)variant)->health = scales[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0) {
        *flags &= ~8u;
    } else if (selection == 1) {
        *flags |= 8;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 3) {
        ((struct game_variant *)variant)->respawn_time = times[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 3) {
        ((struct game_variant *)variant)->respawn_time_growth = times[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->odd_man_out = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0) {
        *flags |= 0x10;
    } else if (selection == 1) {
        *flags &= ~0x10u;
    }
    group = group->next_sibling;
    if (group != 0) {
        selection = first_list_child(group)->selection_index;
        if (selection >= 0 && selection <= 3) {
            ((struct game_variant *)variant)->suicide_penalty = times[selection];
        }
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
