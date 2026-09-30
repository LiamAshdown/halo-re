// ui_event_4a0c00  (not a Ghidra function; ui_event_function_table[67])
// address 0x4a0c00, size 83 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928dc (index 67); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a0c00.
// WRITTEN 2026-09-28 from objdump 0x4a0c00..0x4a0c52: sound 2; with unsaved changes returns the save result when
//   nonzero; otherwise (or unsaved and the save failed) no selected item, closes the root widget, *out_handled = 1,
//   returns 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "fn_interface.h"

extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id


extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern void widget_close(widget_instance *widget); // 0x497c00

uint8_t ui_event_4a0c00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *root;

    widget_play_sound_effect(2);
    if (saved_item_has_unsaved_changes() != 0) {
        uint8_t saved = player_profile_save();

        if (saved != 0) {
            return saved;
        }
    }
    selected_saved_item = -1;
    root = widget;
    while (root->parent != 0) {
        root = root->parent;
    }
    widget_close(root);
    *out_handled = 1;
    return 0;
}
