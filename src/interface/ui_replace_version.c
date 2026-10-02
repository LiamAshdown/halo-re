// ui_replace_version  (not a Ghidra function; ui_replace_function_table[2])
// address 0x4a8760, size 39 bytes
// name confidence: 0.7  rewrite confidence: 1.0
// evidence: ui_replace_function_table 0x00692c08 (the widget text search/replace callbacks 0x4a8750, 0x4a8840,
//   0x4a8760, 0x4a8810); only reachable through that table. Campaign track: the menu's version text.
// objdump 0x4a8760..0x4a8786: the first time (the wide buffer 0x00719300 starts with 0) formats the version string
//   0x0066a890 ("01.00.10.0621") with the wide format 0x0066a888 (L"%S") into it (string_format_wide_va, EDX =
//   the buffer); returns the buffer.
// blam-cc: stack -> widget (cdecl, not read); returns EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint16_t ui_version_text[]; // 0x00719300
extern uint16_t ui_format_narrow_string[]; // 0x0066a888, L"%S"
extern char ui_version_string[]; // 0x0066a890
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX, stack

void *ui_replace_version(widget_instance *widget)
{
    if (ui_version_text[0] == 0) {
        string_format_wide_va(ui_version_text, ui_format_narrow_string, ui_version_string);
    }
    return ui_version_text;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
