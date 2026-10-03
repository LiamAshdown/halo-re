// ui_event_4a3a70  (not a Ghidra function; ui_event_function_table[186])
// address 0x4a3a70, size 186 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692ab8 (index 186); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a3a70.
// WRITTEN 2026-09-28 from objdump 0x4a3a70..0x4a3b29: with the grandparent's second child focused: formats option a
//   (0x00719210) as L"%d" into 0x0071921c and opens the virtual keyboard on it (0x10 characters, field kind 0xd); on
//   success edit field 4, 0x00719218 = 1, result 1. Otherwise, with the third child focused, the same for option b
//   (edit field 5, 0x00719218 = 2) returning 1. Else 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t network_game_option_a_00719210; // 0x00719210, TYPES-GAP
extern uint32_t network_game_option_b_00719214; // 0x00719214, TYPES-GAP
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest
extern int32_t network_host_number_field_00719218; // 0x00719218, UNSURE (1 or 2: which option is being typed)
extern uint16_t network_host_number_text_0071921c[0x10]; // 0x0071921c, UNSURE
extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind); // 0x4a89a0, blam-cc: ESI destination
extern int32_t network_host_edit_field_00719410; // 0x00719410, UNSURE name (3 after the name, 0 after the subname)

uint8_t ui_event_4a3a70(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *second = widget->parent->parent->first_child->next_sibling;
    widget_instance *third;
    uint8_t result = 0;

    if (second->parent->focused_child == second) {
        string_format_wide_va(network_host_number_text_0071921c, (const uint16_t *)L"%d", network_game_option_a_00719210);
        if (virtual_keyboard_open(network_host_number_text_0071921c, 0x10, 0xd) != 0) {
            network_host_edit_field_00719410 = 4;
            network_host_number_field_00719218 = 1;
            result = 1;
        }
    }
    third = second->next_sibling;
    if (result == 0 && third->parent->focused_child == third) {
        string_format_wide_va(network_host_number_text_0071921c, (const uint16_t *)L"%d", network_game_option_b_00719214);
        if (virtual_keyboard_open(network_host_number_text_0071921c, 0x10, 0xd) != 0) {
            network_host_edit_field_00719410 = 5;
            network_host_number_field_00719218 = 2;
            return 1;
        }
    }
    return result;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
