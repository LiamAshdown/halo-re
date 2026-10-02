// ui_event_4a3150  (not a Ghidra function; ui_event_function_table[157])
// address 0x4a3150, size 47 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a44 (index 157); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a3150.
// WRITTEN 2026-09-28 from objdump 0x4a3150..0x4a317e: the parent's first child sends the team allegiance message
//   with 1, its second child with 0 (0x4704d0), returning 1; any other widget returns 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void game_engine_send_team_allegiance_message(char broadcast); // 0x4704d0

uint8_t ui_event_4a3150(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *first = widget->parent->first_child;

    if (first == widget) {
        game_engine_send_team_allegiance_message(1);
        return 1;
    }
    if (first->next_sibling == widget) {
        game_engine_send_team_allegiance_message(0);
        return 1;
    }
    return 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
