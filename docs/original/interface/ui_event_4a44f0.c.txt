// ui_event_4a44f0  (not a Ghidra function; ui_event_function_table[176])
// address 0x4a44f0, size 125 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a90 (index 176); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a44f0.
// WRITTEN 2026-09-28 from objdump 0x4a44f0..0x4a456c: resets the three ui lists, enumerates the checkpoints
//   (autosaves included, newest first) into list 0 through checkpoint_list_add_row (0x4a4280), clears the pending
//   delete name (0x00718fd0), and with no checkpoints closes the widget restoring the previous one. Returns whether
//   any checkpoint was listed.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)
extern int32_t ui_list_current; // 0x00692c04
extern uint8_t ui_list_has_default; // 0x007192f8
extern int32_t game_checkpoint_enumerate_files(uint8_t include_autosaves, uint8_t sort_newest_first, void *callback, void *user_data); // 0x538e70
extern uint8_t checkpoint_list_add_row(int32_t index, const char *name, int32_t level_index, int32_t difficulty, int32_t game_time, const void *time, void *user_data); // 0x4a4280
extern char pending_delete_saved_game_name_00718fd0[]; // 0x00718fd0, UNSURE name
extern void widget_instance_close_and_restore_previous(widget_instance *widget); // 0x49c3e0, blam-cc: EAX

uint8_t ui_event_4a44f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t count;
    int32_t i;

    for (i = 0; i < 3; i++) {
        ui_lists[i].element_size = 0x10;
        ui_lists[i].count = 0;
        ui_lists[i].data = 0;
    }
    ui_list_current = -1;
    ui_list_has_default = 0;
    count = game_checkpoint_enumerate_files(1, 1, (void *)checkpoint_list_add_row, 0);
    pending_delete_saved_game_name_00718fd0[0] = 0;
    if (count == 0) {
        widget_instance_close_and_restore_previous(widget);
    }
    return (uint8_t)(count != 0);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
