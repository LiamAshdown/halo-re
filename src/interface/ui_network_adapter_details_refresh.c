// ui_network_adapter_details_refresh  (Ghidra: FUN_004a4650, renamed)
// renamed from FUN_004a4650 in the naming pass
// address 0x4a4650, size 338 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.15
// evidence: functions.md: "Refreshes detail widgets (type flag and name) for the currently
// selected network adapter list entry." Reuses set_profile_name's established 2-arg shape and
// types/interface.h's ui_list_item (data at +0x04) via the ui_lists[ui_list_current] group.
// register convention: cdecl, the one recognized stack parameter (widget).
// UNSURE: ui_list_widget_rebuild_rows's second argument is a raw code label (&LAB_004a8310) in the original --
// a callback this rewrite cannot reference from a different translation unit; modeled as a
// function pointer to an equally-unresolved stub of the same address. set_profile_name's implicit
// widget/EBX argument (see set_profile_name.c's own header) is again modeled as the outer widget
// parameter. The per-entry blob layout (a type dword at +8, a name string at +8 in uint16 units)
// is a TYPES-GAP guess from the arithmetic alone.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern int32_t ui_list_current;    // 0x00692c04
extern growable_array ui_lists[3]; // 0x006b3830
extern uint8_t profile_globals_block[0x60a4]; // 0x00712dd8

extern uint8_t ui_list_default_item_format(void *item_buffer, int32_t item_index, void *list_items); // 0x4a8310
extern void ui_list_widget_rebuild_rows(widget_instance *widget, ui_list_item_format_function format_item); // 0x4a7db0
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source); // 0x49c710
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern void _wcsncpy(uint16_t *dest, const uint16_t *src, uint32_t count);

void ui_network_adapter_details_refresh(widget_instance *widget)
{
    widget_instance *row;
    widget_instance *c1, *c2, *c3, *c4, *c5, *c6;
    ui_list_item *entry = (ui_list_item *)0;

    ui_list_widget_rebuild_rows(widget, (void *)ui_list_default_item_format);

    {
        uint8_t profile_copy[0x2000];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
        set_profile_name(widget, (const uint16_t *)(profile_copy + 2));
    }

    row = widget->extended_description->first_child->next_sibling;
    c1 = row->first_child;
    c2 = c1->next_sibling;
    c3 = c2->next_sibling;
    c4 = c3->next_sibling;
    c5 = c4->next_sibling;
    c6 = c5->next_sibling;

    if (widget->selection_index >= 0 && widget->selection_index < ui_lists[ui_list_current].count) {
        entry = (ui_list_item *)ui_lists[ui_list_current].data + widget->selection_index;
    }
    row->background_bitmap_frame = 0;

    if (entry == (ui_list_item *)0 || entry->data == (void *)0) {
        c4->state = 0;
        c5->state = 0;
        c6->state = 0;
        c1->selection_index = 0;
        c2->background_bitmap_frame = 0;
        return;
    }

    {
        const uint16_t *blob = (const uint16_t *)entry->data;
        int32_t type = *(const int32_t *)(blob + 2); // byte offset 4 (uint16 index 2)

        c4->background_bitmap_frame = 1;
        c4->state = (type == 1);
        c5->background_bitmap_frame = 2;
        c5->state = (type == 2);
        c6->background_bitmap_frame = 3;
        c6->state = (type == 3);
        c1->selection_index = (int16_t)blob[0];
        c2->background_bitmap_frame = (int16_t)blob[0];

        c3->text = heap_reallocate(c3->text, 0x40, widget_memory_pool);
        if (c3->text != (void *)0) {
            _wcsncpy((uint16_t *)c3->text, blob + 4, 0x1f);
            ((uint16_t *)c3->text)[0x1f] = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4a4650):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a4650(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  wchar_t *_Dest;
  int iVar7;
  int iVar8;
  undefined4 *puVar9;
  undefined4 *puVar10;
  undefined2 *puVar11;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x4a4660;
  FUN_004a7db0(param_1,&LAB_004a8310);
  puVar9 = &DAT_00712dd8;
  puVar10 = &local_2008;
  for (iVar7 = 0x7ff; iVar7 != 0; iVar7 = iVar7 + -1) {
    *puVar10 = *puVar9;
    puVar9 = puVar9 + 1;
    puVar10 = puVar10 + 1;
  }
  set_profile_name((int)&local_2008 + 2);
  iVar8 = (int)*(short *)(param_1 + 0x3c);
  iVar7 = *(int *)(*(int *)(*(int *)(param_1 + 0x4c) + 0x34) + 0x2c);
  iVar1 = *(int *)(iVar7 + 0x34);
  iVar2 = *(int *)(iVar1 + 0x2c);
  iVar3 = *(int *)(iVar2 + 0x2c);
  iVar4 = *(int *)(iVar3 + 0x2c);
  iVar5 = *(int *)(iVar4 + 0x2c);
  iVar6 = *(int *)(iVar5 + 0x2c);
  puVar11 = (undefined2 *)0x0;
  if ((-1 < iVar8) && (iVar8 < (int)(&DAT_006b3834)[DAT_00692c04 * 3])) {
    puVar11 = *(undefined2 **)((&DAT_006b3838)[DAT_00692c04 * 3] + 4 + iVar8 * 0x10);
  }
  *(undefined2 *)(iVar7 + 0x58) = 0;
  if (puVar11 == (undefined2 *)0x0) {
    *(undefined1 *)(iVar4 + 0x10) = 0;
    *(undefined1 *)(iVar5 + 0x10) = 0;
    *(undefined1 *)(iVar6 + 0x10) = 0;
    *(undefined2 *)(iVar1 + 0x40) = 0;
    *(undefined2 *)(iVar2 + 0x58) = 0;
    return;
  }
  *(undefined2 *)(iVar4 + 0x58) = 1;
  *(bool *)(iVar4 + 0x10) = *(int *)(puVar11 + 2) == 1;
  *(undefined2 *)(iVar5 + 0x58) = 2;
  *(bool *)(iVar5 + 0x10) = *(int *)(puVar11 + 2) == 2;
  *(undefined2 *)(iVar6 + 0x58) = 3;
  *(bool *)(iVar6 + 0x10) = *(int *)(puVar11 + 2) == 3;
  *(undefined2 *)(iVar1 + 0x40) = *puVar11;
  *(undefined2 *)(iVar2 + 0x58) = *puVar11;
  _Dest = (wchar_t *)heap_reallocate(0x40);
  *(wchar_t **)(iVar3 + 0x3c) = _Dest;
  _wcsncpy(_Dest,puVar11 + 4,0x1f);
  *(undefined2 *)(*(int *)(iVar3 + 0x3c) + 0x3e) = 0;
  return;
}
#endif
