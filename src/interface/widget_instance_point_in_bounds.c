// widget_instance_point_in_bounds  (Ghidra: widget_instance_point_in_bounds, already named)
// address 0x4999f0, size 161 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: matches the given name exactly; types/interface.h's widget_instance note documents
// local_x/local_y (0x0a/0x0c) as exactly the pair this function sums up the parent chain
// ("widget_instance_point_in_bounds @0x4999f0 sums +0x0a against the X bound and +0x0c against
// the Y bound while walking +0x30"). Ghidra's own decompile conflates EAX and ECX into one
// "in_EAX" variable across the ancestor-sum loop and the widget_list_adjust_rect_for_scroll_arrows call, which reads as if
// the call receives a NULL widget; the disassembly (0x4999f3..0x499a33) shows EAX (the original
// widget) is never touched by that loop (which walks ECX instead), so the call receives the real
// widget in EAX and a stack copy of its own tag bounds in ECX, exactly matching
// widget_list_adjust_rect_for_scroll_arrows's own signature.
// register convention: widget in EAX (in_EAX), unresolved register read.
// blam-cc: EAX -> widget

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t ui_cursor_x; // 0x00718f84, clamped into 0 .. 0x280
extern int32_t ui_cursor_y; // 0x00718f88, clamped into 0 .. 0x1e0

extern void widget_list_adjust_rect_for_scroll_arrows(widget_instance *widget, Rectangle2D *rect); // 0x499990

// blam-cc: EAX -> widget
// Tests whether the current UI cursor position falls within `widget`'s on-screen bounding
// rectangle (its tag's bounds, adjusted for spinner_list scroll arrows), offset by the sum of
// local_x/local_y over the widget and every one of its ancestors.
uint8_t widget_instance_point_in_bounds(widget_instance *widget)
{
    UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;
    Rectangle2D rect = tag->bounds;
    int16_t x_sum = 0;
    int16_t y_sum = 0;
    widget_instance *cursor = widget;

    do {
        x_sum = x_sum + cursor->local_x;
        y_sum = y_sum + cursor->local_y;
        cursor = cursor->parent;
    } while (cursor != (widget_instance *)0);

    widget_list_adjust_rect_for_scroll_arrows(widget, &rect);

    if ((int16_t)(rect.left + x_sum) <= ui_cursor_x && ui_cursor_x <= (int16_t)(rect.right + x_sum) &&
        (int16_t)(rect.top + y_sum) <= ui_cursor_y && ui_cursor_y <= (int16_t)(rect.bottom + y_sum)) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4999f0):

undefined4 widget_instance_point_in_bounds(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  uint *in_EAX;
  short sVar4;
  short sVar5;
  short local_8;
  short sStack_6;
  short local_4;
  short sStack_2;

  iVar1 = *(int *)((*in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar2 = *(undefined4 *)(iVar1 + 0x24);
  uVar3 = *(undefined4 *)(iVar1 + 0x28);
  sVar5 = 0;
  sVar4 = 0;
  do {
    sVar4 = sVar4 + *(short *)((int)in_EAX + 10);
    sVar5 = sVar5 + (short)in_EAX[3];
    in_EAX = (uint *)in_EAX[0xc];
  } while (in_EAX != (uint *)0x0);
  FUN_00499990();
  sStack_6 = (short)((uint)uVar2 >> 0x10);
  sStack_2 = (short)((uint)uVar3 >> 0x10);
  local_8 = (short)uVar2;
  local_4 = (short)uVar3;
  if (((((short)(sStack_6 + sVar4) <= DAT_00718f84) && (DAT_00718f84 <= (short)(sStack_2 + sVar4)))
      && ((short)(local_8 + sVar5) <= DAT_00718f88)) && (DAT_00718f88 <= (short)(local_4 + sVar5)))
  {
    return 1;
  }
  return 0;
}

Disassembly proving EAX (the widget) survives the ancestor-sum loop unchanged, which walks ECX
instead, and is passed to FUN_00499990 alongside a stack copy of the tag's own bounds:

  4999f3: mov    ecx,[eax]            ; widget->definition
  499a08: mov    edx,[ecx+0x24]       ; tag->bounds low dword (top,left)
  499a0b: mov    ecx,[ecx+0x28]       ; tag->bounds high dword (bottom,right)
  499a10: mov    [esp+0xc],ecx
  499a17: mov    [esp+0x8],edx
  499a1b: mov    ecx,eax              ; ECX = widget (EAX untouched from here on)
  499a20: add    si,[ecx+0xa]         ; x_sum += cursor->local_x
  499a24: add    di,[ecx+0xc]         ; y_sum += cursor->local_y
  499a28: mov    ecx,[ecx+0x30]       ; cursor = cursor->parent
  499a2d: jne    0x499a20
  499a2f: lea    ecx,[esp+0x8]        ; &local rect copy
  499a33: call   0x499990             ; FUN_00499990(eax=widget, ecx=&rect)
#endif
