// ui_event_4a2c50  (not a Ghidra function; ui_event_function_table[142])
// address 0x4a2c50, size 46 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a08 (index 142); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a2c50.
// WRITTEN 2026-09-28 from objdump 0x4a2c50..0x4a2c7d: opens the virtual keyboard on the network host name
//   (0x00719170, 0x80 characters, field kind 0xb); if it opened, the edit field (0x00719410) becomes 3 and returns 1,
//   else 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint16_t network_host_name_00719170[0x40]; // 0x00719170
extern int32_t network_host_edit_field_00719410; // 0x00719410, UNSURE name (3 after the name, 0 after the subname)
extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind); // 0x4a89a0, blam-cc: ESI destination

uint8_t ui_event_4a2c50(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (virtual_keyboard_open(network_host_name_00719170, 0x80, 0xb) == 0) {
        return 0;
    }
    network_host_edit_field_00719410 = 3;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
