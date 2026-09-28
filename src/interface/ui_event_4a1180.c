// ui_event_4a1180  (not a Ghidra function; ui_event_function_table[74])
// address 0x4a1180, size 93 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928f8 (index 74); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1180.
// WRITTEN 2026-09-28 from objdump 0x4a1180..0x4a11dc: maps the second child's committed selection (+0x3c) to its ui
//   list id; when valid, caches that child's list (+0x44) entry at the id as the profile slot and returns 1 unless it
//   is -1; otherwise sound 4 and 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern int32_t ui_list_current; // 0x00692c04
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)
extern int32_t profile_slot_lookup_cache_00692ac8; // 0x00692ac8, TYPES-GAP
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id

static int32_t list_item_id(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].id;
    }
    return -1;
}

uint8_t ui_event_4a1180(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *list = widget->first_child->next_sibling;
    int32_t id = list_item_id(*(int16_t *)((uint8_t *)list + 0x3c));

    if (id != -1) {
        profile_slot_lookup_cache_00692ac8 = ((int32_t *)list->list_items)[id];
        if (profile_slot_lookup_cache_00692ac8 != -1) {
            return 1;
        }
    }
    widget_play_sound_effect(4);
    return 0;
}
