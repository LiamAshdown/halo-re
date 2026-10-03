// ui_event_49d5f0  (not a Ghidra function; ui_event_function_table[26])
// address 0x49d5f0, size 422 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692838 (index 26); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d5f0.
// WRITTEN 2026-09-28 from objdump 0x49d5f0..0x49d795: points the widget list at the multiplayer map list
//   (0x00712dcc, 0x00712dd0 entries); when the last multiplayer map record reads, selects the entry whose path
//   matches it case-insensitively (0 when none). The committed selection (+0x3c) takes the selection and the first
//   visible item (+0x3e) -1. Resets the three ui lists and adds, for every map, a GlobalAlloc copy of its friendly
//   name (0x100 characters) with the map index as id, marking the selected one as the default (0x007192f8). Returns
//   1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <wchar.h>
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t *map_list; // 0x00712dcc, map_list_entry[] (0xc bytes, +0 the map path)
extern int32_t map_list_count; // 0x00712dd0
extern uint8_t saved_game_last_mp_map_read(uint8_t *out_data); // 0x53d670
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)
extern int32_t ui_list_current; // 0x00692c04
extern uint8_t ui_list_has_default; // 0x007192f8
extern void map_list_get_friendly_level_name(wchar_t *destination, char *map_path, int32_t destination_capacity); // 0x494f50, blam-cc: EAX map_path, ESI capacity
extern uint32_t growable_array_add_element(growable_array *array); // 0x4cf810, blam-cc: ESI array

uint8_t ui_event_49d5f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t count = map_list_count;
    char last_map[0x104];
    uint16_t name[0x100];
    int32_t i;

    widget->list_items = map_list;
    widget->item_count = (uint16_t)count;
    if (saved_game_last_mp_map_read((uint8_t *)last_map) != 0) {
        widget->selection_index = 0;
        if (count > 0) {
            while (_stricmp(last_map, *(char **)(map_list + widget->selection_index * 0xc)) != 0) {
                widget->selection_index++;
                if (widget->selection_index >= count) {
                    break;
                }
            }
        }
        if (widget->selection_index == count) {
            widget->selection_index = 0;
        }
    }
    *(int16_t *)&((struct widget_instance *)widget)->text = widget->selection_index;
    *(int16_t *)((uint8_t *)widget + 0x3e) = -1;
    for (i = 0; i < 3; i++) {
        ui_lists[i].element_size = 0x10;
        ui_lists[i].count = 0;
        ui_lists[i].data = 0;
    }
    ui_list_current = -1;
    ui_list_has_default = 0;
    for (i = 0; i < count; i++) {
        uint8_t is_default;
        uint32_t index;

        map_list_get_friendly_level_name((wchar_t *)name, *(char **)(map_list + i * 0xc), 0x100);
        is_default = (uint8_t)(i == widget->selection_index);
        index = growable_array_add_element(&ui_lists[0]);
        if (index != 0xffffffff) {
            ui_list_item *item = (ui_list_item *)ui_lists[0].data + index;
            uint16_t *copy;

            item->data = 0;
            copy = (uint16_t *)GlobalAlloc(0, (uint32_t)wcslen((const wchar_t *)name) * 2 + 2);
            item->name = copy;
            item->id = i;
            item->is_default = is_default;
            if (is_default) {
                ui_list_has_default = 1;
            }
            wcscpy((wchar_t *)copy, (const wchar_t *)name);
        }
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
