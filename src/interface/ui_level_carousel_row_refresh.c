// ui_level_carousel_row_refresh  (Ghidra: FUN_004a4e20, renamed)
// renamed from FUN_004a4e20 in the naming pass
// address 0x4a4e20, size 185 bytes
// name confidence: 0.3   rewrite confidence: 0.7
// evidence: functions.md's phase-2 pass read this as "refreshes controller/profile-assignment
// display widgets for one local player slot", but the body reads `level_select_entries` (the
// same 0x00719018 array `ui_build_level_select_list` and its siblings fill, per README's
// level_select_entry note listing 0x49c8f0, 0x49cc80, 0x49ce00 and 0x4a4e20 as its users) and the
// level_select_flags_0071916a/_0071916b packed-byte pair from the same cluster: it sets one
// carousel row's lock icon and unlock-flag rows from `level_select_entries[level_index]`. The
// caller ui_level_carousel_refresh (0x4a4ee0, renamed in this same pass) confirms this: it names
// its own EAX argument "level index" and hands this function one visible carousel row at a time,
// matching README's calling-conventions table entry "FUN_004a4e20 level carousel row | ECX row
// widget, EAX level index".
// register convention: widget in ECX (in_ECX), level index in EAX (in_EAX), both
// unresolved register reads. // blam-cc: ECX -> widget, EAX -> level_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern level_select_entry level_select_entries[10]; // 0x00719018
extern int8_t level_select_flags_0071916a;  // 0x0071916a, compared with movsx
extern uint8_t level_select_flags_0071916b; // 0x0071916b

// blam-cc: ECX -> widget, EAX -> level_index
void ui_level_carousel_row_refresh(widget_instance *widget, int32_t level_index)
{
    widget_instance *row1 = widget->first_child;
    widget_instance *row2 = row1->next_sibling;
    widget_instance *row3 = row2->next_sibling;
    widget_instance *row4 = row3->next_sibling;
    widget_instance *row5 = row4->next_sibling;
    widget_instance *row6 = row5->next_sibling;
    level_select_entry *entry = &level_select_entries[level_index];

    row4->background_bitmap_frame = 1;
    row5->background_bitmap_frame = 2;
    row6->background_bitmap_frame = 3;

    if (entry->valid == 0 && entry->flag_bit1 == 0 && entry->flag_bit2 == 0 && entry->flag_bit3 == 0) {
        row1->selection_index = 10;
        row2->background_bitmap_frame = 10;
        row3->selection_index = 10;
        row4->state = 0;
        row5->state = 0;
        row6->state = 0;
        return;
    }

    row1->selection_index = (int16_t)level_index;
    row2->background_bitmap_frame = (int16_t)level_index;
    row3->selection_index = (int16_t)level_index;
    if (level_select_flags_0071916b == 1 && level_index == level_select_flags_0071916a) {
        row3->selection_index = 0xb;
    }
    row4->state = entry->flag_bit1;
    row5->state = entry->flag_bit2;
    row6->state = entry->flag_bit3;
}

#if 0
Original Ghidra decompilation (0x4a4e20):

void FUN_004a4e20(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined2 uVar7;
  int in_EAX;
  int in_ECX;

  iVar1 = *(int *)(in_ECX + 0x34);
  iVar2 = *(int *)(iVar1 + 0x2c);
  iVar3 = *(int *)(iVar2 + 0x2c);
  iVar4 = *(int *)(iVar3 + 0x2c);
  iVar5 = *(int *)(iVar4 + 0x2c);
  iVar6 = *(int *)(iVar5 + 0x2c);
  *(undefined2 *)(iVar4 + 0x58) = 1;
  *(undefined2 *)(iVar5 + 0x58) = 2;
  *(undefined2 *)(iVar6 + 0x58) = 3;
  if ((((*(char *)(&DAT_0071901c + in_EAX * 2) == '\0') &&
       (*(char *)((int)&DAT_0071901c + in_EAX * 8 + 1) == '\0')) &&
      (*(char *)((int)&DAT_0071901c + in_EAX * 8 + 2) == '\0')) &&
     (*(char *)((int)&DAT_0071901c + in_EAX * 8 + 3) == '\0')) {
    *(undefined2 *)(iVar1 + 0x40) = 10;
    *(undefined2 *)(iVar2 + 0x58) = 10;
    *(undefined2 *)(iVar3 + 0x40) = 10;
    *(undefined1 *)(iVar4 + 0x10) = 0;
    *(undefined1 *)(iVar5 + 0x10) = 0;
    *(undefined1 *)(iVar6 + 0x10) = 0;
    return;
  }
  uVar7 = (undefined2)in_EAX;
  *(undefined2 *)(iVar1 + 0x40) = uVar7;
  *(undefined2 *)(iVar2 + 0x58) = uVar7;
  *(undefined2 *)(iVar3 + 0x40) = uVar7;
  if (((char)((uint)DAT_0071916a >> 8) == '\x01') && (in_EAX == (char)DAT_0071916a)) {
    *(undefined2 *)(iVar3 + 0x40) = 0xb;
  }
  *(undefined1 *)(iVar4 + 0x10) = *(undefined1 *)((int)&DAT_0071901c + in_EAX * 8 + 1);
  *(undefined1 *)(iVar5 + 0x10) = *(undefined1 *)((int)&DAT_0071901c + in_EAX * 8 + 2);
  *(undefined1 *)(iVar6 + 0x10) = *(undefined1 *)((int)&DAT_0071901c + in_EAX * 8 + 3);
  return;
}
#endif
