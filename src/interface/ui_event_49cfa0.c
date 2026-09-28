// ui_event_49cfa0  (not a Ghidra function; ui_event_function_table[9])
// address 0x49cfa0, size 44 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006927f4 (index 9); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49cfa0.
// WRITTEN 2026-09-28 from objdump 0x49cfa0..0x49cfcb: reads the grandparent list's committed selection (int16 at
//   +0x3c); below 4 it becomes the pending difficulty (when not negative) and sound effect 2 plays; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern int16_t pending_difficulty; // 0x00696564
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id

uint8_t ui_event_49cfa0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int16_t selection = *(int16_t *)((uint8_t *)widget->parent->parent + 0x3c);

    if (selection < 4) {
        if (selection >= 0) {
            pending_difficulty = selection;
        }
        widget_play_sound_effect(2);
    }
    return 1;
}
