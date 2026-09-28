// ui_event_49fd30  (not a Ghidra function; ui_event_function_table[55])
// address 0x49fd30, size 693 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928ac (index 55); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49fd30.
// WRITTEN 2026-09-28 from objdump 0x49fd30..0x49ffe4: for a selected variant, the first spinner lists of the first
//   nine children show: dword +0x84 (0..3, else 0); dword +0x88 (0..3, else 0); dword +0x80 0 / 1 / 2 as 1 / 0 / 2
//   (else 0); dword +0x8c 1 / 2 as 1 / 2 (else 0); byte +0x7c == 0; dword +0x90 1..16 as 0..15 (else 0); dword +0x58
//   2 / 5 / 10 / 15 as 1..4 (else 0); byte +0x34 == 0; dword +0x78 0x4650 .. 0x13c68 as 1..6 (else 0). Returns 1, or
//   0 without a variant.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

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

uint8_t ui_event_49fd30(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    int32_t value;

    if (variant == 0) {
        return 0;
    }
    group = widget->first_child;
    value = *(int32_t *)(variant + 0x84);
    first_list_child(group)->selection_index = (int16_t)((uint32_t)value <= 3 ? value : 0);
    group = group->next_sibling;
    value = *(int32_t *)(variant + 0x88);
    first_list_child(group)->selection_index = (int16_t)((uint32_t)value <= 3 ? value : 0);
    group = group->next_sibling;
    value = *(int32_t *)(variant + 0x80);
    first_list_child(group)->selection_index = (int16_t)(value == 0 ? 1 : value == 2 ? 2 : 0);
    group = group->next_sibling;
    value = *(int32_t *)(variant + 0x8c);
    first_list_child(group)->selection_index = (int16_t)(value == 1 ? 1 : value == 2 ? 2 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(variant[0x7c] == 0);
    group = group->next_sibling;
    value = *(int32_t *)(variant + 0x90);
    first_list_child(group)->selection_index = (int16_t)(value > 0 && value <= 0x10 ? value - 1 : 0);
    group = group->next_sibling;
    value = *(int32_t *)(variant + 0x58);
    first_list_child(group)->selection_index = (int16_t)(value == 2 ? 1 : value == 5 ? 2 : value == 10 ? 3 : value == 15 ? 4 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(variant[0x34] == 0);
    group = group->next_sibling;
    value = *(int32_t *)(variant + 0x78);
    first_list_child(group)->selection_index = (int16_t)(value == 0x4650 ? 1 : value == 0x6978 ? 2 : value == 0x8ca0 ? 3 :
        value == 0xafc8 ? 4 : value == 0xd2f0 ? 5 : value == 0x13c68 ? 6 : 0);
    return 1;
}
