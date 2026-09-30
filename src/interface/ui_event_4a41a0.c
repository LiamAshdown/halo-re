// ui_event_4a41a0  (not a Ghidra function; ui_event_function_table[174])
// address 0x4a41a0, size 194 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a88 (index 174); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a41a0.
// WRITTEN 2026-09-28 from objdump 0x4a41a0..0x4a4261: shows the first child (scale 1, visible) when the 'savegame'
//   file exists (0x538770, name at 0x0066a56c), else dims and hides it and focuses its next sibling; shows the third
//   child when any checkpoint enumerates (> 0), else hides it and focuses the fourth. With either present returns 1.
//   Otherwise: while restoring the previous widget, closes this one restoring the previous (returns 1); else queues
//   the first campaign level (main_queue_map_change), clears 0x00719739 and returns whether a restore started
//   meanwhile.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "fn_saved_games.h"


extern int32_t game_checkpoint_enumerate_files(uint8_t include_autosaves, uint8_t sort_newest_first, void *callback, void *user_data); // 0x538e70
extern uint8_t ui_restoring_previous_widget; // 0x00718fcb
extern void widget_instance_close_and_restore_previous(widget_instance *widget); // 0x49c3e0, blam-cc: EAX
extern char *campaign_level_paths[]; // 0x00696574
extern void main_queue_map_change(char *map_name); // 0x4c8740, blam-cc: EAX
extern uint8_t network_wait_flag_00719739; // 0x00719739

static void show(widget_instance *child, uint8_t visible)
{
    if (visible) {
        child->scale = 1.0f;
        child->hidden = 0;
    } else {
        *(uint32_t *)&child->scale = 0x3eaa7efa;
        child->hidden = 1;
    }
}

uint8_t ui_event_4a41a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t has_save = saved_game_file_exists("savegame");
    uint8_t has_checkpoints = (uint8_t)(game_checkpoint_enumerate_files(1, 1, 0, 0) > 0);
    widget_instance *child = widget->first_child;

    show(child, has_save);
    if (!has_save) {
        widget->focused_child = child->next_sibling;
    }
    child = child->next_sibling->next_sibling;
    show(child, has_checkpoints);
    if (!has_checkpoints) {
        widget->focused_child = child->next_sibling;
    }
    if (has_save || has_checkpoints) {
        return 1;
    }
    if (ui_restoring_previous_widget != 0) {
        widget_instance_close_and_restore_previous(widget);
        return 1;
    }
    main_queue_map_change(campaign_level_paths[0]);
    network_wait_flag_00719739 = 0;
    return (uint8_t)(ui_restoring_previous_widget != 0);
}
