// ui_event_4a0bc0  (not a Ghidra function; ui_event_function_table[66])
// address 0x4a0bc0, size 54 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006928d8 (index 66); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a0bc0.
// WRITTEN 2026-09-28 from objdump 0x4a0bc0..0x4a0bf5: when the selected saved item is a profile (low nibble 0),
//   opens the virtual keyboard on its name (working copy +2, 0x18 characters, field kind 8); returns whether the
//   keyboard opened (0 when not a profile).
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind); // 0x4a89a0, blam-cc: ESI destination

uint8_t ui_event_4a0bc0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    const uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;

    if (profile == 0 || virtual_keyboard_open((uint16_t *)(profile + 2), 0x18, 8) == 0) {
        return 0;
    }
    return 1;
}
