// controls_gamepad_toggle_assignment  (Ghidra: FUN_004b5b20, named in phase 4)
// address 0x4b5b20, size 447 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// phase-4 review: this family was named as a server history / favorites list; every caller
// is on the controls setup gamepad screen (see types/interface.h controls_gamepad_record), so
// it was renamed; the old names are logged in symbols/agent_phase4_interface.txt.
// evidence: rewritten from objdump 0x4b5b20..0x4b5cde in the phase-4 review. The first
// rewrite collected the nodes of the row instead of the screen (ECX is the grandparent of
// the row), dropped the ECX screen of 0x4b55d0 and guarded the index against -1, which the
// binary does not do.
//   The argument is a row widget. If its parent is the assigned list (node 0) the entry moves
// from controls_assigned_gamepads to controls_available_gamepads, if it is the available list (node 5) the other
// way. The row is looked up among the list rows (nodes 1..4 or 6..13); a row that is not
// found leaves the index at -1, and since -1 passes the signed index < count test the entry
// before the list is copied (kept from the binary). The entry is appended to the other list
// when it has room (count below 4 / 8) and then removed from its own list; the result is
// that removal (0 when nothing moved). Refreshes the widgets, then fixes the focus: from the
// assigned list, an empty assigned list focuses the available list at its first row, otherwise the
// assigned list focus walks back over hidden rows (previous_sibling), falling back to its
// first row; from the available list the same walk runs on the available list while it is
// not empty and the assigned list is not full, otherwise the assigned list gets the focus at
// its first row.
// register convention: plain cdecl, one stack argument; returns AL.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern controls_gamepad_record controls_available_gamepads[8]; // 0x006b42d8
extern controls_gamepad_record controls_assigned_gamepads[4];   // 0x006b53d8
extern int32_t controls_assigned_gamepad_count;             // 0x00719448
extern int32_t controls_available_gamepad_count;           // 0x0071944c


static void controls_gamepad_focus_visible_row(widget_instance *list)
{
    widget_instance *row = list->focused_child;

    while (row != 0 && row->hidden != 0) {
        row = row->previous_sibling;
    }
    list->focused_child = row != 0 ? row : list->first_child;
}

uint8_t controls_gamepad_toggle_assignment(widget_instance *row)
{
    widget_instance *list = row->parent;
    widget_instance *screen = list->parent;
    widget_instance *nodes[17];
    controls_gamepad_record *source = 0;
    controls_gamepad_record *target = 0;
    int32_t index = -1;
    int32_t i;
    uint8_t moved = 0;

    controls_gamepad_widget_nodes_collect(nodes, screen);
    if (list == nodes[0]) {
        source = controls_assigned_gamepads;
        target = controls_available_gamepads;
        for (i = 0; i < 4; i++) {
            if (nodes[1 + i] == row) {
                index = i;
                break;
            }
        }
    } else if (list == nodes[5]) {
        source = controls_available_gamepads;
        target = controls_assigned_gamepads;
        for (i = 0; i < 8; i++) {
            if (nodes[6 + i] == row) {
                index = i;
                break;
            }
        }
    }

    if (source == controls_assigned_gamepads || source == controls_available_gamepads) {
        int32_t source_count = source == controls_assigned_gamepads ? controls_assigned_gamepad_count : controls_available_gamepad_count;

        if (index < source_count) {
            controls_gamepad_record entry = source[index];
            int32_t *target_count = target == controls_assigned_gamepads ? &controls_assigned_gamepad_count : &controls_available_gamepad_count;
            int32_t capacity = target == controls_assigned_gamepads ? 4 : 8;

            if (*target_count < capacity) {
                target[*target_count] = entry;
                (*target_count)++;
                moved = controls_gamepad_list_remove(&entry, source);
            }
        }
    }

    controls_gamepad_lists_refresh(screen);
    if (row->parent == nodes[0]) {
        if (controls_assigned_gamepad_count == 0) {
            screen->focused_child = nodes[5];
            nodes[5]->focused_child = nodes[5]->first_child;
        } else {
            controls_gamepad_focus_visible_row(nodes[0]);
        }
    } else if (controls_available_gamepad_count != 0 && controls_assigned_gamepad_count != 4) {
        controls_gamepad_focus_visible_row(nodes[5]);
    } else {
        screen->focused_child = nodes[0];
        nodes[0]->focused_child = nodes[0]->first_child;
    }
    return moved;
}

#if 0
Original Ghidra decompilation (0x4b5b20):

undefined1 FUN_004b5b20(int param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  undefined4 *puVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  undefined1 local_279;
  undefined4 *local_278;
  int local_270;
  int aiStack_26c [4];
  int local_25c;
  int aiStack_258 [12];
  undefined4 local_228 [137];

  iVar2 = *(int *)(param_1 + 0x30);
  iVar1 = *(int *)(iVar2 + 0x30);
  puVar4 = (undefined4 *)0x0;
  local_279 = 0;
  local_278 = (undefined4 *)0x0;
  iVar5 = -1;
  FUN_004b5560();
  if (iVar2 == local_270) {
    puVar4 = &DAT_006b53d8;
    local_278 = &DAT_006b42d8;
    iVar2 = 0;
    do {
      iVar6 = iVar2;
      if (aiStack_26c[iVar2] == param_1) break;
      iVar2 = iVar2 + 1;
      iVar6 = iVar5;
    } while (iVar2 < 4);
  }
  else {
    iVar6 = iVar5;
    if (iVar2 == local_25c) {
      puVar4 = &DAT_006b42d8;
      local_278 = &DAT_006b53d8;
      iVar2 = 0;
      do {
        iVar6 = iVar2;
        if (aiStack_258[iVar2] == param_1) break;
        iVar2 = iVar2 + 1;
        iVar6 = iVar5;
      } while (iVar2 < 8);
    }
  }
  iVar2 = DAT_00719448;
  if (((puVar4 == &DAT_006b53d8) || (iVar2 = DAT_0071944c, puVar4 == &DAT_006b42d8)) &&
     (iVar6 < iVar2)) {
    puVar4 = puVar4 + iVar6 * 0x88;
    puVar7 = local_228;
    for (iVar2 = 0x88; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar7 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar7 = puVar7 + 1;
    }
    if (local_278 == &DAT_006b53d8) {
      iVar2 = 4;
      piVar3 = &DAT_00719448;
    }
    else {
      if (local_278 != &DAT_006b42d8) goto LAB_004b5c38;
      iVar2 = 8;
      piVar3 = &DAT_0071944c;
    }
    if (*piVar3 < iVar2) {
      puVar4 = local_228;
      puVar7 = local_278 + *piVar3 * 0x88;
      for (iVar2 = 0x88; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar7 = *puVar4;
        puVar4 = puVar4 + 1;
        puVar7 = puVar7 + 1;
      }
      *piVar3 = *piVar3 + 1;
      local_279 = FUN_004b5850(local_228);
    }
  }
LAB_004b5c38:
  FUN_004b55d0();
  if (*(int *)(param_1 + 0x30) != local_270) {
    if ((DAT_0071944c == 0) || (DAT_00719448 == 4)) {
      *(int *)(iVar1 + 0x38) = local_270;
      *(undefined4 *)(local_270 + 0x38) = *(undefined4 *)(local_270 + 0x34);
      return local_279;
    }
    for (iVar2 = *(int *)(local_25c + 0x38); iVar2 != 0; iVar2 = *(int *)(iVar2 + 0x28)) {
      if (*(char *)(iVar2 + 0x12) == '\0') goto LAB_004b5cc1;
    }
    iVar2 = *(int *)(local_25c + 0x34);
LAB_004b5cc1:
    *(int *)(local_25c + 0x38) = iVar2;
    return local_279;
  }
  if (DAT_00719448 == 0) {
    *(int *)(iVar1 + 0x38) = local_25c;
    *(undefined4 *)(local_25c + 0x38) = *(undefined4 *)(local_25c + 0x34);
    return local_279;
  }
  for (iVar2 = *(int *)(local_270 + 0x38); iVar2 != 0; iVar2 = *(int *)(iVar2 + 0x28)) {
    if (*(char *)(iVar2 + 0x12) == '\0') goto LAB_004b5c86;
  }
  iVar2 = *(int *)(local_270 + 0x34);
LAB_004b5c86:
  *(int *)(local_270 + 0x38) = iVar2;
  return local_279;
}
#endif
