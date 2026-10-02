// widget_instance_find_at_point  (Ghidra: widget_instance_find_at_point, already named)
// address 0x499ad0, size 331 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: matches the given name; recursive hit test over the widget tree. A widget is
// eligible to be tested at all when it (or its first child) can receive events (has its own
// event handlers, is a list type, or its tag's force_handle_mouse flag (bit 15 of
// UIWidgetDefinitionFlags) is set) and is not hidden; when eligible, tests the cursor against its
// tag bounds offset by the accumulated local_x/local_y passed down from the caller, then
// recurses into children before falling back to itself (unless it is itself a list type, which
// never matches as a fallback -- only a specific child can).
// register convention: cdecl, all four recognized stack parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14

extern void widget_list_adjust_rect_for_scroll_arrows(widget_instance *widget, Rectangle2D *rect); // 0x499990

// Recursively searches a widget instance and its descendants for the topmost widget whose
// bounding rectangle contains the given screen point (offset_x/offset_y is the accumulated
// local_x/local_y of every ancestor above `widget`), honoring visibility and list-type widget
// rules.
widget_instance *widget_instance_find_at_point(widget_instance *widget, int32_t cursor_x,
                                                int32_t cursor_y, int32_t offset_xy)
{
    UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;
    widget_instance *first_child = widget->first_child;
    uint8_t eligible =
        (widget->hidden == 0 &&
         (tag->event_handlers.count > 0 || widget->widget_type == 2 || widget->widget_type == 3)) ||
        (first_child == (widget_instance *)0 || first_child->widget_type == 2 ||
         first_child->widget_type == 3) ||
        ((int8_t)((uint32_t)tag->flags >> 8) < 0);
    widget_instance *result = (widget_instance *)0;

    if (!eligible || widget->hidden == 1) {
        return (widget_instance *)0;
    }

    {
        int16_t off_x = (int16_t)offset_xy + widget->local_x;
        int16_t off_y = (int16_t)(offset_xy >> 16) + widget->local_y;
        Rectangle2D rect = tag->bounds;

        widget_list_adjust_rect_for_scroll_arrows(widget, &rect);

        if ((int16_t)(rect.left + off_x) <= cursor_x && cursor_x <= (int16_t)(rect.right + off_x) &&
            (int16_t)(rect.top + off_y) <= cursor_y && cursor_y <= (int16_t)(rect.bottom + off_y)) {
            widget_instance *child = first_child;
            int32_t child_offset = ((int32_t)off_y << 16) | (uint16_t)off_x;

            while (child != (widget_instance *)0 && result == (widget_instance *)0) {
                result = widget_instance_find_at_point(child, cursor_x, cursor_y, child_offset);
                child = child->next_sibling;
            }
            if (widget->widget_type == 3 || widget->widget_type == 2) {
                widget = (widget_instance *)0;
            }
            if (result == (widget_instance *)0) {
                result = widget;
            }
            return result;
        }
        return (widget_instance *)0;
    }
}

#if 0
Original Ghidra decompilation (0x499ad0):

uint * widget_instance_find_at_point(uint *param_1,int param_2,int param_3,undefined4 param_4)

{
  char *pcVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  uint uVar5;
  short sVar6;
  uint *puVar7;
  short local_8;
  short sStack_6;
  short local_4;
  short sStack_2;

  puVar7 = param_1;
  iVar2 = *(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar3 = *(undefined4 *)(iVar2 + 0x24);
  uVar4 = *(undefined4 *)(iVar2 + 0x28);
  pcVar1 = (char *)((int)param_1 + 0x12);
  uVar5 = param_1[0xd];
  param_1 = (uint *)0x0;
  if (((((*pcVar1 == '\0') &&
        (((0 < *(int *)(iVar2 + 0x54) || (*(short *)((int)puVar7 + 0xe) == 2)) ||
         (*(short *)((int)puVar7 + 0xe) == 3)))) ||
       (((puVar7[0xc] == 0 || (sVar6 = *(short *)(puVar7[0xc] + 0xe), sVar6 == 2)) || (sVar6 == 3)))
       ) || ((char)((uint)*(undefined4 *)(iVar2 + 0x2c) >> 8) < '\0')) && (*pcVar1 != '\x01')) {
    sVar6 = (short)param_4 + *(short *)((int)puVar7 + 10);
    param_4._2_2_ = param_4._2_2_ + (short)puVar7[3];
    param_4 = CONCAT22(param_4._2_2_,sVar6);
    FUN_00499990();
    sStack_6 = (short)((uint)uVar3 >> 0x10);
    sStack_2 = (short)((uint)uVar4 >> 0x10);
    local_8 = (short)uVar3;
    local_4 = (short)uVar4;
    if ((((short)(sStack_6 + sVar6) <= param_2) && (param_2 <= (short)(sStack_2 + sVar6))) &&
       (((short)(local_8 + param_4._2_2_) <= param_3 &&
        (param_3 <= (short)(local_4 + param_4._2_2_))))) {
      while ((uVar5 != 0 && (param_1 == (uint *)0x0))) {
        param_1 = (uint *)widget_instance_find_at_point(uVar5,param_2,param_3,param_4);
        uVar5 = *(uint *)(uVar5 + 0x2c);
      }
      if ((*(short *)((int)puVar7 + 0xe) == 3) || (*(short *)((int)puVar7 + 0xe) == 2)) {
        puVar7 = (uint *)0x0;
      }
      if (param_1 == (uint *)0x0) {
        param_1 = puVar7;
      }
      return param_1;
    }
    return (uint *)0x0;
  }
  return (uint *)0x0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
