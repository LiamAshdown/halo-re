// ui_event_4a11e0  (not a Ghidra function; ui_event_function_table[75])
// address 0x4a11e0, size 153 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928fc (index 75); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a11e0.
// WRITTEN 2026-09-28 from objdump 0x4a11e0..0x4a1278: caches the widget list (+0x44) entry at the ui list id of its
//   committed selection (index -1 when out of range) as the profile slot. -1: sound 4, returns 0. Bit 30 set: sound
//   4, raises quit confirm error 0x1a when none is up, returns 0. Otherwise 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "objects.h"
#include "units.h"

extern int32_t ui_list_current; // 0x00692c04
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)
extern int32_t profile_slot_lookup_cache_00692ac8; // 0x00692ac8, TYPES-GAP
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id
extern int16_t quit_confirm_error_string_index; // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae; // 0x00718fae
extern uint8_t quit_confirm_error_modal; // 0x00718fb0
extern uint8_t quit_confirm_error_is_error; // 0x00718fb1

static int32_t list_item_id(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].id;
    }
    return -1;
}

uint8_t ui_event_4a11e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t id = list_item_id(*(int16_t *)&((struct widget_instance *)widget)->text);
    int32_t item = ((int32_t *)widget->list_items)[id];

    profile_slot_lookup_cache_00692ac8 = item;
    if (item == -1) {
        widget_play_sound_effect(4);
        return 0;
    }
    if ((item & 0x40000000) == 0) {
        return 1;
    }
    widget_play_sound_effect(4);
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x1a;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
    }
    return 0;
}
