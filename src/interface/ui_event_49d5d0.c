// ui_event_49d5d0  (not a Ghidra function; ui_event_function_table[25])
// address 0x49d5d0, size 18 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692834 (index 25); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49d5d0.
// WRITTEN 2026-09-28 from objdump 0x49d5d0..0x49d5e1: makes sure the variant history has an entry, applies the
//   current custom variant, sets 0x0071c2dd; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern uint32_t game_engine_ensure_variant_history_has_entry(void); // 0x463b20
extern void game_engine_apply_current_custom_variant(void); // 0x463b90
extern uint8_t unknown_0071c2dd; // 0x0071c2dd, UNSURE identity

uint8_t ui_event_49d5d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    game_engine_ensure_variant_history_has_entry();
    game_engine_apply_current_custom_variant();
    unknown_0071c2dd = 1;
    return 1;
}
