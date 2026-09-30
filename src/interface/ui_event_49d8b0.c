// ui_event_49d8b0  (not a Ghidra function; ui_event_function_table[29])
// address 0x49d8b0, size 509 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692844 (index 29); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d8b0.
// WRITTEN 2026-09-28 from objdump 0x49d8b0..0x49daac: builds the saved variant list: forgets the cached profile
//   slot, fills the variant carousel slots (0x00879d60, 0x1d4 bytes) with 0xff, grows the widget list (+0x44) to
//   0x190 bytes, creates the default playlist profiles once (0x0069e8d0), enumerates up to 100 variant handles (type
//   1, built-in) into it, pads to at least three with -1 and stores the count (+0x48). Resets the three ui lists;
//   when the last multiplayer variant record reads and names a saved variant, selects its row. Each row: -1 applies
//   the current custom variant (0x463b90); otherwise a loaded variant is added as a ui list item in group 0 (no team
//   list child) or, with a spinner list first grandchild, group 0 / 1 / 2 by flag bits 8 / 7 (+0x38), named by the
//   variant, id = row, data = the 0x98 byte variant, default when it is the last one. Finally committed selection
//   (+0x3c) = selection and first visible item (+0x3e) = -1; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include "fn_game.h"

extern int32_t profile_slot_lookup_cache_00692ac8; // 0x00692ac8, TYPES-GAP
extern uint8_t variant_carousel_slots[0x1d4]; // 0x00879d60 (variant_carousel_slot[3])
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old_payload, ESI self
extern uint8_t playlist_profiles_need_defaults; // 0x0069e8d0
extern void playlist_profile_create_default_profiles_on_disk(void); // 0x53bc70
extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only, uint16_t *capacity_and_count); // 0x53c4e0, blam-cc: EBX capacity_and_count
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)
extern int32_t ui_list_current; // 0x00692c04
extern uint8_t ui_list_has_default; // 0x007192f8
extern uint8_t saved_game_last_mp_variant_read(uint8_t *out_data); // 0x53d3f0
extern int32_t saved_game_find_by_name(char *name, int16_t type); // 0x53d4a0

extern uint8_t saved_game_get_variant(int32_t handle, void *out); // 0x53bee0
extern void ui_list_add_entry(int32_t group_index, const uint16_t *name, int32_t id, const void *data_blob, uint32_t data_size, uint8_t is_default); // 0x4a7ba0, blam-cc: EAX group_index, CL is_default

uint8_t ui_event_49d8b0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t grouped = (uint8_t)(widget->first_child != 0 && widget->first_child->first_child != 0 &&
        widget->first_child->first_child->widget_type == 2);
    int32_t *handles;
    uint16_t count = 0x64;
    int32_t last = -1;
    char last_name[0x100];
    uint32_t variant[0x26];
    int32_t i;

    profile_slot_lookup_cache_00692ac8 = -1;
    memset(variant_carousel_slots, 0xff, sizeof(variant_carousel_slots));
    handles = (int32_t *)heap_reallocate(widget->list_items, 0x190, widget_memory_pool);
    widget->list_items = handles;
    if (handles != 0) {
        if (playlist_profiles_need_defaults == 1) {
            playlist_profile_create_default_profiles_on_disk();
            playlist_profiles_need_defaults = 0;
        }
        saved_game_enumerate_by_type(1, handles, 1, &count);
        for (; count < 3; count++) {
            handles[count] = -1;
        }
        widget->item_count = count;
        for (i = 0; i < 3; i++) {
            ui_lists[i].element_size = 0x10;
            ui_lists[i].count = 0;
            ui_lists[i].data = 0;
        }
        ui_list_current = -1;
        ui_list_has_default = 0;
        if (saved_game_last_mp_variant_read((uint8_t *)last_name) != 0) {
            last = saved_game_find_by_name(last_name, 1);
            if (last != -1) {
                uint16_t row;

                for (row = 0; row < count; row++) {
                    if (handles[row] == last) {
                        widget->selection_index = (int16_t)row;
                        break;
                    }
                }
            }
        }
        for (i = 0; i < count; i++) {
            if (handles[i] == -1) {
                game_engine_apply_current_custom_variant();
            } else if (saved_game_get_variant(handles[i], variant) != 0) {
                int32_t group = 0;

                if (grouped) {
                    uint32_t flags = variant[0x38 / 4];

                    group = (flags & 0x100) != 0 ? 0 : (flags & 0x80) != 0 ? 1 : 2;
                }
                ui_list_add_entry(group, (const uint16_t *)variant, i, variant, 0x98, (uint8_t)(last == handles[i]));
            }
        }
    }
    *(int16_t *)&((struct widget_instance *)widget)->text = widget->selection_index;
    *(int16_t *)((uint8_t *)widget + 0x3e) = -1;
    return 1;
}
