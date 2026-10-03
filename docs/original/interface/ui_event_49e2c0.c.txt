// ui_event_49e2c0  (not a Ghidra function; ui_event_function_table[41])
// address 0x49e2c0, size 64 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692874 (index 41); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49e2c0.
// WRITTEN 2026-09-28 from objdump 0x49e2c0..0x49e2ff: for a selected variant, opens the virtual keyboard on its
//   name (working copy +0, 0x30 characters, field kind 9) and on success sets the edit field (0x00719410) to 2;
//   returns 1 either way, 0 without a variant.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind); // 0x4a89a0, blam-cc: ESI destination
extern int32_t network_host_edit_field_00719410; // 0x00719410, UNSURE name (3 after the name, 0 after the subname)

uint8_t ui_event_49e2c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;

    if (variant == 0) {
        return 0;
    }
    if (virtual_keyboard_open((uint16_t *)variant, 0x30, 9) != 0) {
        network_host_edit_field_00719410 = 2;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
