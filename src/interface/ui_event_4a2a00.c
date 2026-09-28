// ui_event_4a2a00  (not a Ghidra function; ui_event_function_table[139])
// address 0x4a2a00, size 195 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006929fc (index 139); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a2a00.
// WRITTEN 2026-09-28 from objdump 0x4a2a00..0x4a2ac2: forgets the cached profile slot and reads the widget list
//   (+0x44) at the ui list id of its committed selection (index -1 when out of range). Negative (not -1): fetches
//   that profile into a 0x1ffc byte local and loads it for player 0 (0x495970), returning 1; a failed fetch returns
//   0. Not negative: raises quit confirm error 0x36 when none is up, sound 4, returns 0. -1 returns 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "objects.h"
#include "units.h"

extern int32_t profile_slot_lookup_cache_00692ac8; // 0x00692ac8, TYPES-GAP
extern int32_t ui_list_current; // 0x00692c04
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)
extern uint8_t player_profile_get(int32_t index, void *out_buffer); // 0x53a770, blam-cc: ECX out_buffer
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970, blam-cc: AX, EDX, stack
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

uint8_t ui_event_4a2a00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t profile[0x1ffc];
    int32_t id = list_item_id(*(int16_t *)&((struct widget_instance *)widget)->text);
    int32_t item;

    profile_slot_lookup_cache_00692ac8 = -1;
    item = ((int32_t *)widget->list_items)[id];
    if (item == -1) {
        return 0;
    }
    if (item < 0) {
        if (player_profile_get(item, profile) == 0) {
            return 0;
        }
        player_profile_load(0, profile, item);
        return 1;
    }
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x36;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
    }
    widget_play_sound_effect(4);
    return 0;
}
