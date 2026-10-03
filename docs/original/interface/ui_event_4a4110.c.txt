// ui_event_4a4110  (not a Ghidra function; ui_event_function_table[169])
// address 0x4a4110, size 124 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a74 (index 169); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a4110.
// WRITTEN 2026-09-28 from objdump 0x4a4110..0x4a418b: finds the widget among its parent's first four children (not
//   found: returns 1). When the parent's committed selection (+0x3c) already equals that position: a position below 4
//   becomes the pending difficulty (when not negative) with sound 2, then the saved game restarts (0x4a1110). The
//   parent's committed selection becomes the position; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t pending_difficulty; // 0x00696564
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id
extern uint32_t ui_restart_saved_game(void); // 0x4a1110

uint8_t ui_event_4a4110(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *parent = widget->parent;
    widget_instance *child = parent->first_child;
    int16_t *committed = (int16_t *)((uint8_t *)parent + 0x3c);
    int32_t i;

    for (i = 0; child != widget; i++) {
        if (i + 1 >= 4) {
            return 1;
        }
        child = child->next_sibling;
    }
    if (*committed == i) {
        if (i < 4) {
            if (i >= 0) {
                pending_difficulty = (int16_t)i;
            }
            widget_play_sound_effect(2);
        }
        ui_restart_saved_game();
    }
    *committed = (int16_t)i;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
