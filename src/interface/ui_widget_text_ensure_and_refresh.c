// ui_widget_text_ensure_and_refresh  (Ghidra: FUN_004a4fe0, renamed)
// renamed from FUN_004a4fe0 in the naming pass
// address 0x4a4fe0, size 89 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: functions.md: "Ensures a widget's text buffer exists and refreshes it from a global
// text field." A simple text_box-style widget->text allocate-if-needed then wcsncpy, matching
// widget_instance::text's documented 0x3c offset in types/interface.h.
// register convention: cdecl, the one recognized stack parameter (widget).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint16_t global_text_field_00719278[0x40]; // 0x00719278, TYPES-GAP

extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern void _wcsncpy(uint16_t *dest, const uint16_t *src, uint32_t count);

void ui_widget_text_ensure_and_refresh(widget_instance *widget)
{
    if (widget->text == (void *)0) {
        uint32_t *block = (uint32_t *)heap_reallocate(widget->text, 0x80, widget_memory_pool);

        widget->text = block;
        if (block != (uint32_t *)0) {
            int32_t i;

            for (i = 0; i < 0x20; i++) {
                block[i] = 0;
            }
        }
    }
    if (widget->text != (void *)0) {
        _wcsncpy((uint16_t *)widget->text, global_text_field_00719278, 0x3f);
        ((uint16_t *)widget->text)[0x3f] = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4a4fe0):

void FUN_004a4fe0(int param_1)

{
  undefined4 *puVar1;
  int iVar2;

  if (*(int *)(param_1 + 0x3c) == 0) {
    puVar1 = (undefined4 *)heap_reallocate(0x80);
    *(undefined4 **)(param_1 + 0x3c) = puVar1;
    if (puVar1 != (undefined4 *)0x0) {
      for (iVar2 = 0x20; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar1 = 0;
        puVar1 = puVar1 + 1;
      }
    }
  }
  if (*(wchar_t **)(param_1 + 0x3c) != (wchar_t *)0x0) {
    _wcsncpy(*(wchar_t **)(param_1 + 0x3c),(wchar_t *)&DAT_00719278,0x3f);
    *(undefined2 *)(*(int *)(param_1 + 0x3c) + 0x7e) = 0;
  }
  return;
}
#endif
