// ui_event_4a0ae0  (not a Ghidra function; ui_event_function_table[64])
// address 0x4a0ae0, size 100 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928d0 (index 64); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a0ae0.
// WRITTEN 2026-09-28 from objdump 0x4a0ae0..0x4a0b43: forgets the cached profile slot and reads the first child's
//   list (+0x44) at its selection (+0x40). -1: sound 4, returns 0. Negative: selects that saved item, returns 1.
//   Otherwise raises quit confirm error 0x1f when none is up, sound 4, returns 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t profile_slot_lookup_cache_00692ac8; // 0x00692ac8, TYPES-GAP
extern void saved_item_select(int32_t item); // 0x495be0, blam-cc: EBX -> item
extern int16_t quit_confirm_error_string_index; // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae; // 0x00718fae
extern uint8_t quit_confirm_error_modal; // 0x00718fb0
extern uint8_t quit_confirm_error_is_error; // 0x00718fb1
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id

uint8_t ui_event_4a0ae0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *list = widget->first_child;
    int32_t item = ((int32_t *)list->list_items)[list->selection_index];

    profile_slot_lookup_cache_00692ac8 = -1;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
