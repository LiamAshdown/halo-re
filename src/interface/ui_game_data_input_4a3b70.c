// ui_game_data_input_4a3b70  (not a Ghidra function; game_data_input_function_table[58])
// address 0x4a3b70, size 363 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692c00 (index 58); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a3b70.
// WRITTEN 2026-09-28 from objdump 0x4a3b70..0x4a3cda: commits the number being typed (0x00719218: 1 option a, 2
//   option b) from 0x0071921c with _wtoi, clamped to 0xffff; an empty entry gives option a the default 0x00698208 and
//   option b 0. Then the second and third children show options a and b as L"%d" in their second child's text (0x10
//   bytes), hidden at 0x3eaa7efa in split screen, else shown at 1.0; syncs the extended description selection and
//   clears 0x00719218. (The binary returns 0x4a66b0's leftover AL; this table ignores it.)
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <stdlib.h>

extern uint8_t ui_split_screen; // 0x00718fc9
extern int32_t network_host_number_field_00719218; // 0x00719218, UNSURE (1 or 2: which option is being typed)
extern uint16_t network_host_number_text_0071921c[0x10]; // 0x0071921c, UNSURE
extern uint32_t network_game_socket_port; // 0x00698208, UNSURE name
extern uint32_t network_game_option_a_00719210; // 0x00719210, TYPES-GAP
extern uint32_t network_game_option_b_00719214; // 0x00719214, TYPES-GAP
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old_payload, ESI self
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest
extern void widget_extended_description_sync_selection(widget_instance *widget); // 0x4a66b0

static void set_option_text(widget_instance *row, uint32_t value, uint8_t hidden)
{
    widget_instance *label = row->first_child->next_sibling;
    uint16_t *text;

    text = (uint16_t *)heap_reallocate(label->text, 0x10, widget_memory_pool);
    label->text = text;
    if (text != 0) {
        string_format_wide_va(text, (const uint16_t *)L"%d", value);
        text[7] = 0;
    }
    if (hidden) {
        row->hidden = 1;
        *(uint32_t *)&row->scale = 0x3eaa7efa;
    } else {
        row->hidden = 0;
        row->scale = 1.0f;
    }
}

void ui_game_data_input_4a3b70(widget_instance *widget)
{
    uint8_t hidden = (uint8_t)(ui_split_screen == 0);
    widget_instance *row;

    if (network_host_number_field_00719218 == 1) {
        if (network_host_number_text_0071921c[0] == 0) {
            network_game_option_a_00719210 = network_game_socket_port;
        } else {
            network_game_option_a_00719210 = (uint32_t)_wtoi((const wchar_t *)network_host_number_text_0071921c);
            if (network_game_option_a_00719210 > 0xffff) {
                network_game_option_a_00719210 = 0xffff;
            }
        }
    } else if (network_host_number_field_00719218 == 2) {
        if (network_host_number_text_0071921c[0] == 0) {
            network_game_option_b_00719214 = 0;
        } else {
            network_game_option_b_00719214 = (uint32_t)_wtoi((const wchar_t *)network_host_number_text_0071921c);
            if (network_game_option_b_00719214 > 0xffff) {
                network_game_option_b_00719214 = 0xffff;
            }
        }
    }
    row = widget->first_child->next_sibling;
    set_option_text(row, network_game_option_a_00719210, hidden);
    row = row->next_sibling;
    set_option_text(row, network_game_option_b_00719214, hidden);
    widget_extended_description_sync_selection(widget);
    network_host_number_field_00719218 = 0;
}
