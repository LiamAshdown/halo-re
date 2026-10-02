// ui_game_data_input_4a6ab0  (not a Ghidra function; game_data_input_function_table[25])
// address 0x4a6ab0, size 71 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b7c (index 25); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a6ab0.
// WRITTEN 2026-09-28 from objdump 0x4a6ab0..0x4a6af6: for a selected variant, grows the widget text (+0x3c) to 0x30
//   bytes in the widget pool and copies up to 0x17 characters of the variant name (0x00714e80) into it, terminated at
//   [0x17].
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old_payload, ESI self

void ui_game_data_input_4a6ab0(widget_instance *widget)
{
    uint16_t *text;

    if ((selected_saved_item & 0xf) != 1) {
        return;
    }
    text = (uint16_t *)heap_reallocate(widget->text, 0x30, widget_memory_pool);
    widget->text = text;
    if (text != 0) {
        wcsncpy((wchar_t *)text, (const wchar_t *)saved_item_working_copy, 0x17);
        text[0x17] = 0;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
