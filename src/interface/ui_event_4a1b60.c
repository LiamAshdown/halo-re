// ui_event_4a1b60  (not a Ghidra function; ui_event_function_table[99])
// address 0x4a1b60, size 130 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x0069295c (index 99); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1b60.
// WRITTEN 2026-09-28 from objdump 0x4a1b60..0x4a1be1: when 0x0071916b is 1 and the level select path (0x00719068)
//   matches 0x00719779 case-insensitively, selects and focuses child number level_select_frame (0x00719168; also the
//   committed selection +0x3c); otherwise selects and focuses child 1. Returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t level_select_flags_0071916b; // 0x0071916b, TYPES-GAP
extern char level_select_current_path_00719068[0x106]; // 0x00719068, TYPES-GAP
extern char unknown_00719779[]; // 0x00719779, UNSURE: current scenario/level name buffer
extern int16_t level_select_frame_00719168; // 0x00719168, TYPES-GAP

uint8_t ui_event_4a1b60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *child = widget->first_child;
    int16_t selection = 1;
    int32_t i;

    if (level_select_flags_0071916b == 1 && _stricmp(level_select_current_path_00719068, unknown_00719779) == 0) {
        selection = level_select_frame_00719168;
    }
    for (i = 0; i < selection && child != 0; i++) {
        child = child->next_sibling;
    }
    widget->selection_index = selection;
    widget->focused_child = child;
    *(int16_t *)&((struct widget_instance *)widget)->text = widget->selection_index;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
