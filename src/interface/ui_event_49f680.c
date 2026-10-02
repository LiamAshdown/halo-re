// ui_event_49f680  (not a Ghidra function; ui_event_function_table[52])
// address 0x49f680, size 549 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928a0 (index 52); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49f680.
// WRITTEN 2026-09-28 from objdump 0x49f680..0x49f8a4: for a selected variant, the first spinner lists of the first
//   six children show: byte +0x7c == 0; dword +0x80 0x708 / 0xe10 / 0x1518 / 0x2328 / 0x4650 as 1..5 (else 0); byte
//   +0x7e == 0; byte +0x7f == 0; dword +0x58 3 / 5 / 10 / 15 as 1..4 (else 0); dword +0x78 0x4650 .. 0x13c68 as 1..6
//   (else 0). Returns 1, or 0 without a variant.
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

uint8_t ui_event_49f680(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    int32_t value;

    if (variant == 0) {
        return 0;
    }
    group = widget->first_child;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->engine.ctf.assault == 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->engine.ctf.single_flag_time;
    first_list_child(group)->selection_index = (int16_t)(value == 0x708 ? 1 : value == 0xe10 ? 2 : value == 0x1518 ? 3 :
        value == 0x2328 ? 4 : value == 0x4650 ? 5 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->engine.ctf.flag_must_reset == 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->engine.ctf.flag_at_home_to_score == 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->score_limit;
    first_list_child(group)->selection_index = (int16_t)(value == 3 ? 1 : value == 5 ? 2 : value == 10 ? 3 : value == 15 ? 4 : 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->time_limit;
    first_list_child(group)->selection_index = (int16_t)(value == 0x4650 ? 1 : value == 0x6978 ? 2 : value == 0x8ca0 ? 3 :
        value == 0xafc8 ? 4 : value == 0xd2f0 ? 5 : value == 0x13c68 ? 6 : 0);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
