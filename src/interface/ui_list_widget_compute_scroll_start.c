// ui_list_widget_compute_scroll_start  (Ghidra: FUN_004a7d00, renamed)
// address 0x4a7d00, size 174 bytes
// name confidence: 0.35 (chosen)   rewrite confidence: 0.5
// evidence: types/interface.h widget_instance (first_child, widget_type, item_count all match);
// types/tags.h UIWidgetDefinition::child_widgets is the TagReflexive at the very end of the
// struct (0x3ec total size minus its own 0xc), landing on the `+0x3e0` tag-data read here, and
// flags_2 bit 3 (list_single_preview_no_scroll) on the `+0x150 & 8` test, both already
// established by ui_list_find_default.c and widget_extended_description_sync_selection.c.
// UNSURE: widget_instance + 0x3c/0x3e (nominally the text_box-only `text` pointer) is reused by
// this list-widget family as two packed int16 fields -- a "selected list-item index" at 0x3c and
// a persistent "first visible row" scroll cursor at 0x3e -- carried across rebuilds since the
// widget itself has no scrollbar state of its own. Not in types/interface.h; kept as raw offsets.
// register convention: widget in EAX (in_EAX, unresolved register read).
//   // blam-cc: widget -> EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "fn_interface.h"

extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: widget -> EAX
// Recomputes the first visible row of a scrollable list widget so its window still covers the
// row named by the widget's cached "selected index" (offset 0x3c), clamping to the widget's
// tag-defined row count and its own item_count, and caches the result back at offset 0x3e.
int32_t ui_list_widget_compute_scroll_start(widget_instance *widget)
{
    UIWidgetDefinition *tag_data;
    int32_t visible_rows;    // iVar6
    int16_t *scroll_start = (int16_t *)((uint8_t *)widget + 0x3e); // UNSURE offset, see header
    int16_t *selected_index_field = (int16_t *)((uint8_t *)widget + 0x3c); // UNSURE offset
    int32_t scroll_start_value;
    int32_t selected_index;
    int32_t window_size;   // uVar7
    int32_t item_count;    // uVar5
    uint8_t needs_paging;  // bVar2

    tag_data = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;
    visible_rows = (int32_t)tag_data->child_widgets.count;
    scroll_start_value = *scroll_start;

    if (widget->first_child != 0 && widget->first_child->first_child != 0 &&
        widget->first_child->first_child->widget_type == uiwidgettype_spinner_list) {
        visible_rows = visible_rows - 1;
    }

    item_count = (uint16_t)widget->item_count;
    needs_paging = ((tag_data->flags_2 & 8) == 0 && (visible_rows - 1 < item_count)) ? 0 : 1;
    window_size = visible_rows - (needs_paging ? 1 : 3);
    if (item_count < window_size) {
        window_size = item_count;
    }

    selected_index = *selected_index_field;
    if (selected_index < scroll_start_value || window_size + scroll_start_value <= selected_index) {
        if (selected_index < 0) {
            *scroll_start = 0;
            return 0;
        }
        *scroll_start = (int16_t)((item_count - window_size <= selected_index) ?
                                       (item_count - window_size) : selected_index);
    } else if (scroll_start_value == -1) {
        *scroll_start = 0;
        return *scroll_start;
    }
    return *scroll_start;
}

#if 0
Original Ghidra decompilation (0x4a7d00):

int FUN_004a7d00(void)

{
  int iVar1;
  bool bVar2;
  uint *in_EAX;
  short sVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  int iVar8;

  iVar4 = *(int *)((*in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar6 = *(int *)(iVar4 + 0x3e0);
  iVar8 = (int)*(short *)((int)in_EAX + 0x3e);
  if (((in_EAX[0xd] != 0) && (iVar1 = *(int *)(in_EAX[0xd] + 0x34), iVar1 != 0)) &&
     (*(short *)(iVar1 + 0xe) == 2)) {
    iVar6 = iVar6 + -1;
  }
  if (((*(byte *)(iVar4 + 0x150) & 8) == 0) && (iVar6 + -1 < (int)(uint)(ushort)in_EAX[0x12])) {
    bVar2 = false;
  }
  else {
    bVar2 = true;
  }
  uVar7 = iVar6 - ((uint)!bVar2 * 2 + 1);
  uVar5 = (uint)(ushort)in_EAX[0x12];
  if ((int)uVar5 < (int)uVar7) {
    uVar7 = uVar5;
  }
  sVar3 = (short)in_EAX[0xf];
  iVar4 = (int)sVar3;
  if ((iVar4 < iVar8) || ((int)(uVar7 + iVar8) <= iVar4)) {
    if (iVar4 < 0) {
      *(undefined2 *)((int)in_EAX + 0x3e) = 0;
      return 0;
    }
    if ((int)(uVar5 - uVar7) <= iVar4) {
      sVar3 = (short)(uVar5 - uVar7);
    }
    *(short *)((int)in_EAX + 0x3e) = sVar3;
  }
  else if (iVar8 == -1) {
    *(undefined2 *)((int)in_EAX + 0x3e) = 0;
    return (int)*(short *)((int)in_EAX + 0x3e);
  }
  return (int)*(short *)((int)in_EAX + 0x3e);
}
#endif
