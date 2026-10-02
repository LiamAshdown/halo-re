// ui_event_4a07e0  (not a Ghidra function; ui_event_function_table[60])
// address 0x4a07e0, size 127 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928c0 (index 60); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a07e0.
// WRITTEN 2026-09-28 from objdump 0x4a07e0..0x4a085e: with unsaved changes: a profile or variant selection with bit
//   30 set whose name did not change starts a name edit and returns 0; any other case returns the save result
//   (0x495d40, a tail jump). Without changes: no selected item, closes the root widget, *out_handled = 1, returns 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t saved_item_has_unsaved_changes(void); // 0x495ea0
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern int32_t saved_item_name_changed(void); // 0x495c90
extern uint8_t saved_item_name_edit_begin(void); // 0x495cf0
extern uint8_t player_profile_save(void); // 0x495d40
extern void widget_close(widget_instance *widget); // 0x497c00

uint8_t ui_event_4a07e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *root;

    if (saved_item_has_unsaved_changes() != 0) {
        int32_t item = selected_saved_item;

        if (item != -1 && (item & 0xf) <= 1 && ((item >> 30) & 1) != 0 && (uint8_t)saved_item_name_changed() == 0) {
            saved_item_name_edit_begin();
            return 0;
        }
        return player_profile_save();
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
