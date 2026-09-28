// ui_widget_text_from_hud_objective  (Ghidra: FUN_004a6770, unnamed)
// address 0x4a6770, size 151 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x4a6770..0x4a6806 (UI game data input table 0x00692b18, entry 0x692b60). When HUD
//   messaging (*0x006b3a40) has a current entry at +0x470, its string offset (word +0x20) indexes the text data of
//   the scenario's HUD message text tag (scenario +0x5a0 tag id; tag data +0xc is the wide-character text pointer).
//   A non-empty string is copied into the widget's text block (+0x3c, heap_reallocate'd to 2 * length + 2 bytes in
//   the widget heap *0x006926c4) with wcsncpy and terminated.
// blam-cc: stack -> widget

#include "crt.h"
#include <wchar.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern hud_messaging_globals *hud_messaging;
extern Scenario *global_scenario; // 0x00746f8c
extern tag_instance *tag_instances; // 0x0087bc14
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self

void ui_widget_text_from_hud_objective(widget_instance *widget)
{
    uint8_t *entry = *(uint8_t **)&hud_messaging->objective_text;
    uint8_t *text_tag;
    uint16_t *text;
    int32_t length;
    uint16_t *buffer;

    if (entry == 0) {
        return;
    }
    text_tag = (uint8_t *)tag_instances[*(uint32_t *)((uint8_t *)global_scenario + 0x5a0) & 0xffff].data;
    text = (uint16_t *)(*(uint8_t **)(text_tag + 0xc) + (uint32_t)*(uint16_t *)(entry + 0x20) * 2);
    if (text == 0 || *text == 0) {
        return;
    }
    length = wcslen(text);
    if (length <= 0) {
        return;
    }
    buffer = (uint16_t *)heap_reallocate(widget->text, (uint16_t)(length * 2 + 2), widget_memory_pool);
    widget->text = buffer;
    if (buffer == 0) {
        return;
    }
    wcsncpy((wchar_t *)buffer, (const wchar_t *)text, (size_t)length);
    buffer[length] = 0;
}
