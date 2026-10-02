// ui_event_4a0860  (not a Ghidra function; ui_event_function_table[61])
// address 0x4a0860, size 454 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928c4 (index 61); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a0860.
// WRITTEN 2026-09-28 from objdump 0x4a0860..0x4a0a25: resets the three ui lists; for a selected profile clamps its
//   colour word +0x11a to 0..0x11 and makes it the selection (+0x40), committed selection (+0x3c) and first visible
//   item -1 (+0x3e). Grows the widget list (+0x44) to 0x12 bytes (index i holds i) and adds 0x12 ui list items named
//   from the colors_list unicode string list
//   (ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\color_edit\\colors_list; each present
//   string is forced terminated, missing ones read L"<missing string>"), GlobalAlloc copies with id i, the selection
//   marked default. item_count becomes 0x12. Returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <wchar.h>
#include "objects.h"
#include "units.h"

extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)
extern int32_t ui_list_current; // 0x00692c04
extern uint8_t ui_list_has_default; // 0x007192f8
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old_payload, ESI self
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern tag_instance *tag_instances; // 0x0087bc14
extern uint16_t missing_string_text[]; // 0x00671fac, L"<missing string>"
extern uint32_t growable_array_add_element(growable_array *array); // 0x4cf810, blam-cc: ESI array

uint8_t ui_event_4a0860(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    uint8_t *indices;
    datum_index strings;
    int32_t i;

    for (i = 0; i < 3; i++) {
        ui_lists[i].element_size = 0x10;
        ui_lists[i].count = 0;
        ui_lists[i].data = 0;
    }
    ui_list_current = -1;
    ui_list_has_default = 0;
    if (profile != 0) {
        int16_t colour = *(int16_t *)(profile + 0x11a);

        colour = (int16_t)(colour < 0 ? 0 : colour > 0x11 ? 0x11 : colour);
        *(int16_t *)(profile + 0x11a) = colour;
        widget->selection_index = colour;
        *(int16_t *)&((struct widget_instance *)widget)->text = *(int16_t *)(profile + 0x11a);
        *(int16_t *)((uint8_t *)widget + 0x3e) = -1;
    }
    indices = (uint8_t *)heap_reallocate(widget->list_items, 0x12, widget_memory_pool);
    widget->list_items = indices;
    if (indices == 0) {
        return 1;
    }
    strings = tag_lookup(0x75737472, (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\color_edit\\colors_list"); // 'ustr', 0x0066a578
    for (i = 0; i < 0x12; i++) {
        uint16_t *text = missing_string_text;
        uint8_t is_default;
        uint32_t index;

        ((uint8_t *)widget->list_items)[i] = (uint8_t)i;
        if (strings != 0xffffffff) {
            uint8_t *list = (uint8_t *)tag_instances[strings & 0xffff].data;

            if (i < *(int32_t *)list) {
                uint8_t *element = *(uint8_t **)(list + 4) + i * 0x14;
                uint32_t size = *(uint32_t *)element;

                if ((int32_t)size > 0) {
                    text = *(uint16_t **)(element + 0xc);
                    text[(size >> 1) - 1] = 0;
                }
            }
        }
        is_default = (uint8_t)(i == widget->selection_index);
        index = growable_array_add_element(&ui_lists[0]);
        if (index != 0xffffffff) {
            ui_list_item *item = (ui_list_item *)ui_lists[0].data + index;
            uint16_t *copy;

            item->data = 0;
            copy = (uint16_t *)GlobalAlloc(0, (uint32_t)wcslen((const wchar_t *)text) * 2 + 2);
            item->name = copy;
            item->id = i;
            item->is_default = is_default;
            if (is_default) {
                ui_list_has_default = 1;
            }
            wcscpy((wchar_t *)copy, (const wchar_t *)text);
        }
    }
    widget->item_count = 0x12;
    return 1;
}
