// ui_list_default_item_format  (not a Ghidra function; the row formatter the list builders hand to
//   ui_list_widget_rebuild_rows 0x4a7db0, pushed as an immediate at 0x4a4667, 0x4a6827, 0x4a8377, 0x4a84e7,
//   0x4a8567 and 0x4a8607; no C existed, so its callers' references were unbound)
// address 0x4a8310, size 77 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x4a8310..0x4a835c: copies up to 0x3f characters of the name of the current
//   ui list's item at item_index (the empty string at 0x00660c34 when out of range) into the row buffer,
//   terminates it at [0x3f] and returns whether the name is non-empty. list_items is not read.
// blam-cc: stack -> item_buffer, item_index, list_items (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include <wchar.h>

extern int32_t ui_list_current; // 0x00692c04
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)

uint8_t ui_list_default_item_format(void *item_buffer, int32_t item_index, void *list_items)
{
    uint16_t *out = (uint16_t *)item_buffer;
    const uint16_t *name = (const uint16_t *)L"";

    if (item_index >= 0 && item_index < ui_lists[ui_list_current].count) {
        name = ((ui_list_item *)ui_lists[ui_list_current].data)[item_index].name;
    }
    wcsncpy((wchar_t *)out, (const wchar_t *)name, 0x3f);
    out[0x3f] = 0;
    return (uint8_t)(out[0] != 0);
}
