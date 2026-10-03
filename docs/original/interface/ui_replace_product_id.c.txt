// ui_replace_product_id  (not a Ghidra function; ui_replace_function_table[3])
// address 0x4a8810, size 40 bytes
// name confidence: 0.6  rewrite confidence: 1.0
// evidence: ui_replace_function_table 0x00692c08 (the widget text search/replace callbacks 0x4a8750, 0x4a8840,
//   0x4a8760, 0x4a8810); only reachable through that table.
// objdump 0x4a8810..0x4a8837: the first time (the wide buffer 0x00719368 starts with 0) formats
//   registry_get_product_id() with the wide format 0x0066a888 (L"%S") into it; returns the buffer.
// blam-cc: stack -> widget (cdecl, not read); returns EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint16_t ui_product_id_text[]; // 0x00719368
extern uint16_t ui_format_narrow_string[]; // 0x0066a888, L"%S"
extern void *registry_get_product_id(void); // 0x4a8790
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX, stack

void *ui_replace_product_id(widget_instance *widget)
{
    if (ui_product_id_text[0] == 0) {
        string_format_wide_va(ui_product_id_text, ui_format_narrow_string, registry_get_product_id());
    }
    return ui_product_id_text;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
