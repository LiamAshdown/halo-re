// ui_game_data_input_4a6a60  (not a Ghidra function; game_data_input_function_table[24])
// address 0x4a6a60, size 69 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b78 (index 24); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a6a60.
// WRITTEN 2026-09-28 from objdump 0x4a6a60..0x4a6aa4: for a selected profile, grows the widget text (+0x3c) to 0x18
//   bytes in the widget pool and copies up to 0xb characters of the profile name (0x00714e82) into it, terminated at
//   [0xb].
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <wchar.h>

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old_payload, ESI self

void ui_game_data_input_4a6a60(widget_instance *widget)
{
    uint16_t *text;

    if ((selected_saved_item & 0xf) != 0) {
        return;
    }
    text = (uint16_t *)heap_reallocate(widget->text, 0x18, widget_memory_pool);
    widget->text = text;
    if (text != 0) {
        wcsncpy((wchar_t *)text, (const wchar_t *)(saved_item_working_copy + 2), 0xb);
        text[0xb] = 0;
    }
}
