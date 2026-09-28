// ui_network_host_setup_refresh  (Ghidra: FUN_004a2cb0, renamed)
// renamed from FUN_004a2cb0 in the naming pass
// address 0x4a2cb0, size 605 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.15
// evidence: functions.md: "Refreshes the network game host setup widgets (player/team name, list
// selections, local IP address) and commits changes via set_profile_name." Reuses
// heap_reallocate's 1-arg convention (ui_string_replace_all.c precedent), the resolution lookup
// tables introduced in FUN_004a2ad0.c, and set_profile_name's established 2-parameter shape.
// register convention: cdecl, the one recognized stack parameter (widget). Ghidra shows this as
// void; treated as returning nothing meaningful.
// UNSURE (significant): this function walks widget's children strictly by next_sibling/first_child
// position (not a spinner_list search like the sibling controls-options functions), which was
// preserved literally rather than reinterpreted. set_profile_name's implicit EBX/widget argument
// at the final call is not recoverable from this decompile; modeled as the outer `widget`
// parameter itself, which is almost certainly wrong but is the least-arbitrary guess available.
// The inet_ntoa byte-swizzle, the string_format_wide_va_bounded destination pointer arithmetic,
// and the resolution-table double-clamp are all preserved exactly as decompiled without an
// attempt to simplify or rationalize them.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern uint16_t network_host_name_00719170[144];  // 0x00719170
extern uint16_t network_host_subname_007191f0[9]; // 0x007191f0
extern int32_t quality_selection_00692b04;         // 0x00692b04
extern int32_t resolution_row_count_table_0065bfb4[5][1]; // 0x0065bfb4
extern int32_t resolution_selection_00719204;      // 0x00719204
extern int32_t resolution_index_table_0065bf74[];  // 0x0065bf74
extern uint32_t resolution_selected_value_00699584; // 0x00699584
extern uint32_t local_ip_address_006869b4;          // 0x006869b4, TYPES-GAP
extern uint16_t local_port_006869b6;                 // 0x006869b6, TYPES-GAP
extern uint16_t ip_port_format_string_0066a564[];    // 0x0066a564, TYPES-GAP (format string)
extern uint32_t port_format_arg_00698208;             // 0x00698208, TYPES-GAP
extern uint8_t profile_globals_block[0x60a4];         // 0x00712dd8

extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern void _wcsncpy(uint16_t *dest, const uint16_t *src, uint32_t count);
extern char *__stdcall inet_ntoa(uint32_t addr); // ws2_32 import; struct in_addr passed by value as uint32
extern void string_convert_ascii_to_unicode(void); // 0x557990, UNSURE args
extern wchar_t *string_format_wide_va_bounded(wchar_t *dest, const wchar_t *format, ...); // 0x557910
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source); // 0x49c710

void ui_network_host_setup_refresh(widget_instance *widget)
{
    widget_instance *tab_group = widget->extended_description;
    widget_instance *row = widget->first_child;
    widget_instance *control = row->first_child->next_sibling;
    int32_t tab_index = -1;
    uint16_t *buffer;
    int32_t quality;
    int32_t resolution_row_count;
    int32_t resolution_index;
    widget_instance *ip_control;

    buffer = (uint16_t *)heap_reallocate(control->text, 0x80, widget_memory_pool);
    control->text = buffer;
    if (buffer != (uint16_t *)0) {
        _wcsncpy(buffer, network_host_name_00719170, 0x3f);
        ((uint16_t *)control->text)[0x3f] = 0;
    }
    if (row->parent->focused_child == row) {
        tab_index = 0;
    }

    row = row->next_sibling;
    control = row->first_child->next_sibling;
    buffer = (uint16_t *)heap_reallocate(control->text, 0x12, widget_memory_pool);
    control->text = buffer;
    if (buffer != (uint16_t *)0) {
        _wcsncpy(buffer, network_host_subname_007191f0, 8);
        ((uint16_t *)control->text)[8] = 0;
    }
    if (row->parent->focused_child == row) {
        tab_index = 1;
    }

    row = row->next_sibling;
    quality = (row->first_child->next_sibling)->selection_index;
    if (row->parent->focused_child == row) {
        tab_index = quality + 2;
    }

    row = row->next_sibling;
    control = row->first_child->next_sibling;
    if (control->selection_index < 0) {
        resolution_index = 0;
    } else {
        resolution_row_count = resolution_row_count_table_0065bfb4[quality][0] - 1;
        resolution_index = (control->selection_index <= resolution_row_count) ? control->selection_index
                                                                                : resolution_row_count;
    }
    quality_selection_00692b04 = quality;
    control->selection_index = (int16_t)resolution_index;
    resolution_selection_00719204 = resolution_index;
    control->item_count = *(uint16_t *)((uint8_t *)resolution_row_count_table_0065bfb4 + quality * 4);
    control->selection_index = (int16_t)resolution_index;

    if (resolution_index < 0) {
        resolution_index = 0;
    } else {
        resolution_row_count = resolution_row_count_table_0065bfb4[quality][0] - 1;
        if (resolution_index > resolution_row_count) {
            resolution_index = resolution_row_count;
            resolution_selection_00719204 = resolution_row_count;
        }
    }
    resolution_selected_value_00699584 = resolution_index_table_0065bf74[resolution_index];
    if (row->parent->focused_child == row) {
        tab_index = 7;
    }

    row = row->next_sibling;
    row->hidden = 1;
    ip_control = row->first_child->next_sibling;
    ip_control->text = heap_reallocate(ip_control->text, 0x40, widget_memory_pool);
    if (ip_control->text != (void *)0) {
        // Byte-swaps local_ip_address_006869b4 into network order before formatting it as a string.
        uint32_t swapped = ((local_ip_address_006869b4 << 0x10 | local_ip_address_006869b4 & 0xff00 |
                             local_ip_address_006869b4 >> 0x10 & 0xff) << 8) |
                            (local_ip_address_006869b4 >> 0x18);
        char *text = inet_ntoa(swapped);
        char *end = text + 1;

        while (*text != '\0') {
            text++;
        }
        string_convert_ascii_to_unicode();
        string_format_wide_va_bounded(
            (wchar_t *)((uint8_t *)ip_control->text + (int32_t)((text - end)) * 2),
            (const wchar_t *)ip_port_format_string_0066a564, port_format_arg_00698208);
        ((uint16_t *)ip_control->text)[0x1f] = 0;
    }

    if (tab_index == -1) {
        tab_group->first_child->state = 0;
    } else {
        tab_group->first_child->selection_index = (int16_t)tab_index;
        tab_group->first_child->state = 1;
    }

    {
        uint8_t profile_copy[0x2000];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
        // UNSURE: set_profile_name's implicit widget/EBX argument, see file header.
        set_profile_name(widget, (const uint16_t *)(profile_copy + 2));
    }
}

#if 0
Original Ghidra decompilation (0x4a2cb0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004a2cb0(int param_1)

{
  char *pcVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  wchar_t *pwVar6;
  int iVar7;
  char *pcVar8;
  int iVar9;
  int iVar10;
  undefined4 *puVar11;
  undefined4 *puVar12;
  int local_2014;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x4a2cc0;
  iVar9 = *(int *)(param_1 + 0x4c);
  iVar3 = *(int *)(param_1 + 0x34);
  iVar10 = *(int *)(*(int *)(iVar3 + 0x34) + 0x2c);
  local_2014 = -1;
  pwVar6 = (wchar_t *)heap_reallocate(0x80);
  *(wchar_t **)(iVar10 + 0x3c) = pwVar6;
  if (pwVar6 != (wchar_t *)0x0) {
    _wcsncpy(pwVar6,(wchar_t *)&DAT_00719170,0x3f);
    *(undefined2 *)(*(int *)(iVar10 + 0x3c) + 0x7e) = 0;
  }
  if (*(int *)(*(int *)(iVar3 + 0x30) + 0x38) == iVar3) {
    local_2014 = 0;
  }
  iVar3 = *(int *)(iVar3 + 0x2c);
  iVar10 = *(int *)(*(int *)(iVar3 + 0x34) + 0x2c);
  pwVar6 = (wchar_t *)heap_reallocate(0x12);
  *(wchar_t **)(iVar10 + 0x3c) = pwVar6;
  if (pwVar6 != (wchar_t *)0x0) {
    _wcsncpy(pwVar6,(wchar_t *)&DAT_007191f0,8);
    *(undefined2 *)(*(int *)(iVar10 + 0x3c) + 0x10) = 0;
  }
  if (*(int *)(*(int *)(iVar3 + 0x30) + 0x38) == iVar3) {
    local_2014 = 1;
  }
  iVar3 = *(int *)(iVar3 + 0x2c);
  iVar10 = (int)*(short *)(*(int *)(*(int *)(iVar3 + 0x34) + 0x2c) + 0x40);
  if (*(int *)(*(int *)(iVar3 + 0x30) + 0x38) == iVar3) {
    local_2014 = iVar10 + 2;
  }
  iVar3 = *(int *)(iVar3 + 0x2c);
  iVar4 = *(int *)(*(int *)(iVar3 + 0x34) + 0x2c);
  sVar5 = *(short *)(iVar4 + 0x40);
  if (sVar5 < 0) {
    iVar7 = 0;
  }
  else {
    iVar7 = *(int *)(&DAT_0065bfb4 + iVar10 * 4) + -1;
    if ((int)sVar5 <= *(int *)(&DAT_0065bfb4 + iVar10 * 4) + -1) {
      iVar7 = (int)sVar5;
    }
  }
  sVar5 = (short)iVar7;
  DAT_00692b04 = iVar10;
  *(short *)(iVar4 + 0x40) = sVar5;
  iVar7 = (int)sVar5;
  _DAT_00719204 = iVar7;
  *(undefined2 *)(iVar4 + 0x48) = *(undefined2 *)(&DAT_0065bfb4 + iVar10 * 4);
  *(short *)(iVar4 + 0x40) = sVar5;
  if (iVar7 < 0) {
    iVar10 = 0;
  }
  else {
    iVar10 = *(int *)(&DAT_0065bfb4 + iVar10 * 4) + -1;
    if (iVar7 <= iVar10) goto LAB_004a2dee;
  }
  iVar7 = iVar10;
  _DAT_00719204 = iVar10;
LAB_004a2dee:
  DAT_00699584 = (&DAT_0065bf74)[iVar7];
  if (*(int *)(*(int *)(iVar3 + 0x30) + 0x38) == iVar3) {
    local_2014 = 7;
  }
  iVar3 = *(int *)(iVar3 + 0x2c);
  *(undefined1 *)(iVar3 + 0x12) = 1;
  iVar3 = *(int *)(*(int *)(iVar3 + 0x34) + 0x2c);
  iVar10 = heap_reallocate(0x40);
  *(int *)(iVar3 + 0x3c) = iVar10;
  if (iVar10 != 0) {
    pcVar8 = inet_ntoa((in_addr)((DAT_006869b4 << 0x10 | DAT_006869b4 & 0xff00 |
                                 DAT_006869b4 >> 0x10 & 0xff) << 8 | DAT_006869b4 >> 0x18));
    pcVar1 = pcVar8 + 1;
    do {
      cVar2 = *pcVar8;
      pcVar8 = pcVar8 + 1;
    } while (cVar2 != '\0');
    FUN_00557990();
    string_format_wide_va_bounded
              (*(int *)(iVar3 + 0x3c) + ((int)pcVar8 - (int)pcVar1) * 2,&DAT_0066a564,DAT_00698208);
    *(undefined2 *)(*(int *)(iVar3 + 0x3c) + 0x3e) = 0;
  }
  if (local_2014 == -1) {
    *(undefined1 *)(*(int *)(iVar9 + 0x34) + 0x10) = 0;
  }
  else {
    *(short *)(*(int *)(iVar9 + 0x34) + 0x40) = (short)local_2014;
    *(undefined1 *)(*(int *)(iVar9 + 0x34) + 0x10) = 1;
  }
  puVar11 = &DAT_00712dd8;
  puVar12 = &local_2008;
  for (iVar9 = 0x7ff; iVar9 != 0; iVar9 = iVar9 + -1) {
    *puVar12 = *puVar11;
    puVar11 = puVar11 + 1;
    puVar12 = puVar12 + 1;
  }
  set_profile_name((int)&local_2008 + 2);
  return;
}
#endif
