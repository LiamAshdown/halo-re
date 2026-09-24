// widget_cursor_side_of_midpoint  (Ghidra: FUN_004a1ff0, renamed)
// renamed from FUN_004a1ff0 in the naming pass
// address 0x4a1ff0, size 126 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: functions.md: "Determines which side of a widget's bounding-rectangle midpoint the
// current cursor position falls on, returning +1 or -1 accordingly." types/interface.h names
// 0x00718f84 ui_cursor_x. The summed field (offset 0x0a) is local_x, matching
// widget_instance_point_in_bounds's own use of the same field for the X axis per
// types/interface.h's widget_instance note.
// register convention: widget in EAX (in_EAX, unresolved register read).
// blam-cc: EAX -> widget
// UNSURE: widget_list_adjust_rect_for_scroll_arrows is called with no visible arguments and its result (if any) is not
// consumed by this function's visible computation; preserved as a no-argument call for fidelity.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern int32_t ui_cursor_x; // 0x00718f84
extern tag_instance *tag_instances; // 0x0087bc14

extern void widget_list_adjust_rect_for_scroll_arrows(void); // 0x499990, UNSURE args/purpose

// blam-cc: EAX -> widget
int32_t widget_cursor_side_of_midpoint(widget_instance *widget)
{
    UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;
    int16_t cumulative_x = 0;
    widget_instance *ancestor;
    int16_t left;
    int16_t right;
    int32_t midpoint;

    for (ancestor = widget; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
        cumulative_x = cumulative_x + ancestor->local_x;
    }

    left = tag->bounds.left;
    right = tag->bounds.right;
    widget_list_adjust_rect_for_scroll_arrows();

    midpoint = ((int16_t)(right + cumulative_x) + (int16_t)(left + cumulative_x)) / 2;
    return (midpoint < ui_cursor_x) * 2 - 1;
}

#if 0
Original Ghidra decompilation (0x4a1ff0):

int FUN_004a1ff0(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  uint *in_EAX;
  short sVar4;
  short sStack_6;
  short sStack_2;

  iVar1 = *(int *)((*in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar4 = 0;
  do {
    sVar4 = sVar4 + *(short *)((int)in_EAX + 10);
    in_EAX = (uint *)in_EAX[0xc];
  } while (in_EAX != (uint *)0x0);
  uVar2 = *(undefined4 *)(iVar1 + 0x24);
  uVar3 = *(undefined4 *)(iVar1 + 0x28);
  FUN_00499990();
  sStack_6 = (short)((uint)uVar2 >> 0x10);
  sStack_2 = (short)((uint)uVar3 >> 0x10);
  return (uint)(((int)(short)(sStack_2 + sVar4) + (int)(short)(sStack_6 + sVar4)) / 2 < DAT_00718f84
               ) * 2 + -1;
}
#endif
