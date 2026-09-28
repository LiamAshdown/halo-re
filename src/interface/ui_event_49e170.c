// ui_event_49e170  (not a Ghidra function; ui_event_function_table[38])
// address 0x49e170, size 145 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692868 (index 38); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49e170.
// WRITTEN 2026-09-28 from objdump 0x49e170..0x49e200: forgets the cached profile slot, maps the widget's committed
//   selection (int16 +0x3c) to its ui list id (-1 when out of range) and reads the widget list (+0x44) at that id
//   (index -1 included, as the binary does). -1: sound 4, returns 0. Negative: selects that saved item, returns 1.
//   Otherwise raises quit confirm error 0x1f when none is up, sound 4, returns 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern int32_t profile_slot_lookup_cache_00692ac8; // 0x00692ac8, TYPES-GAP
extern int32_t ui_list_current; // 0x00692c04
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)
extern void saved_item_select(int32_t item); // 0x495be0, blam-cc: EBX -> item
extern int16_t quit_confirm_error_string_index; // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae; // 0x00718fae
extern uint8_t quit_confirm_error_modal; // 0x00718fb0
extern uint8_t quit_confirm_error_is_error; // 0x00718fb1
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id

static int32_t list_item_id(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].id;
    }
    return -1;
}

uint8_t ui_event_49e170(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t id = list_item_id(*(int16_t *)((uint8_t *)widget + 0x3c));
    int32_t item;

    profile_slot_lookup_cache_00692ac8 = -1;
    item = ((int32_t *)widget->list_items)[id];
    if (item != -1) {
        if (item < 0) {
            saved_item_select(item);
            return 1;
        }
        if (quit_confirm_error_string_index == -1) {
            quit_confirm_error_string_index = 0x1f;
            quit_confirm_error_unknown_ae = -1;
            quit_confirm_error_modal = 1;
            quit_confirm_error_is_error = 0;
        }
    }
    widget_play_sound_effect(4);
    return 0;
}
