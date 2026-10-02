// ui_network_name_fields_refresh  (Ghidra: FUN_004a4b60, renamed)
// renamed from FUN_004a4b60 in the naming pass
// address 0x4a4b60, size 272 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.9
// evidence: functions.md: "Refreshes the name/team display widgets from the pending name-entry
// globals and commits the change." The original is a two-row variant of FUN_004a2cb0.c's name/team
// refresh, with the same tab-group commit pattern and set_profile_name call at the end.
// register convention: cdecl, the one recognized stack parameter (widget).
// FIXED 2026-09-30 (disassembly): set_profile_name's EBX is tab_group->first_child->next_sibling (0x4a4c5d), not the outer widget; and the
// profile block copy is 0x7ff dwords (0x1ffc bytes), not 0x2000. Everything else (two heap_reallocate/wcsncpy text rows, the
// tab-group selection commit) matches the disassembly 0x4a4b60..0x4a4c70.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint16_t network_host_name_field_00719238[32]; // 0x00719238
extern uint16_t network_host_subname_007191f0[9];       // 0x007191f0
extern uint8_t profile_globals_block[0x60a4];             // 0x00712dd8

extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source); // 0x49c710

void ui_network_name_fields_refresh(widget_instance *widget)
{
    widget_instance *tab_group = widget->extended_description;
    widget_instance *row = widget->first_child;
    widget_instance *control = row->first_child->next_sibling;
    int32_t tab_index = -1;
    uint16_t *buffer;

    buffer = (uint16_t *)heap_reallocate(control->text, 0x40, widget_memory_pool);
    control->text = buffer;
    if (buffer != (uint16_t *)0) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)network_host_name_field_00719238, 0x1f);
        ((uint16_t *)control->text)[0x1f] = 0;
    }
    if (row->parent->focused_child == row) {
        tab_index = 0;
    }

    row = row->next_sibling;
    control = row->first_child->next_sibling;
    buffer = (uint16_t *)heap_reallocate(control->text, 0x12, widget_memory_pool);
    control->text = buffer;
    if (buffer != (uint16_t *)0) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)network_host_subname_007191f0, 8);
        ((uint16_t *)control->text)[8] = 0;
    }

    if (row->parent->focused_child == row) {
        tab_group->first_child->selection_index = 1;
        tab_group->first_child->state = 1;
    } else if (tab_index == -1) {
        tab_group->first_child->state = 0;
    } else {
        tab_group->first_child->selection_index = (int16_t)tab_index;
        tab_group->first_child->state = 1;
    }

    {
        uint8_t profile_copy[0x1ffc];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
        set_profile_name(tab_group->first_child->next_sibling, (const uint16_t *)(profile_copy + 2));
    }
}

#if 0
Original Ghidra decompilation (0x4a4b60):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a4b60(int param_1)

{
  int iVar1;
  int iVar2;
  wchar_t *pwVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  int local_2010;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x4a4b70;
  iVar4 = *(int *)(param_1 + 0x4c);
  iVar1 = *(int *)(param_1 + 0x34);
  iVar2 = *(int *)(*(int *)(iVar1 + 0x34) + 0x2c);
  local_2010 = -1;
  pwVar3 = (wchar_t *)heap_reallocate(0x40);
  *(wchar_t **)(iVar2 + 0x3c) = pwVar3;
  if (pwVar3 != (wchar_t *)0x0) {
    _wcsncpy(pwVar3,(wchar_t *)&DAT_00719238,0x1f);
    *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x3e) = 0;
  }
  if (*(int *)(*(int *)(iVar1 + 0x30) + 0x38) == iVar1) {
    local_2010 = 0;
  }
  iVar1 = *(int *)(iVar1 + 0x2c);
  iVar2 = *(int *)(*(int *)(iVar1 + 0x34) + 0x2c);
  pwVar3 = (wchar_t *)heap_reallocate(0x12);
  *(wchar_t **)(iVar2 + 0x3c) = pwVar3;
  if (pwVar3 != (wchar_t *)0x0) {
    _wcsncpy(pwVar3,(wchar_t *)&DAT_007191f0,8);
    *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x10) = 0;
  }
  if (*(int *)(*(int *)(iVar1 + 0x30) + 0x38) == iVar1) {
    local_2010 = 1;
  }
  else if (local_2010 == -1) {
    *(undefined1 *)(*(int *)(iVar4 + 0x34) + 0x10) = 0;
    goto LAB_004a4c46;
  }
  *(undefined2 *)(*(int *)(iVar4 + 0x34) + 0x40) = (undefined2)local_2010;
  *(undefined1 *)(*(int *)(iVar4 + 0x34) + 0x10) = 1;
LAB_004a4c46:
  puVar5 = &DAT_00712dd8;
  puVar6 = &local_2008;
  for (iVar4 = 0x7ff; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar6 = puVar6 + 1;
  }
  set_profile_name((int)&local_2008 + 2);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
