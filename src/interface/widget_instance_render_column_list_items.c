// widget_instance_render_column_list_items  (Ghidra: FUN_0049bac0, renamed)
// renamed from FUN_0049bac0 in the naming pass
// address 0x49bac0, size 153 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: sole caller is widget_instance_render @0x49a8c0 (outside this session's range,
// already rewritten), which invokes this only for widget_type == 3 (column_list) and then skips
// its own generic child-render loop when tag->flags_2 bit 0 is set -- exactly the flag this
// function itself gates its whole body on. Renders every child of the list widget through
// widget_instance_render, flagging whichever child index equals the widget's own selection_index,
// and always finishes by clearing scroll_blink (the "selection changed" counter per
// types/interface.h's widget_instance::scroll_blink comment).
// register convention: widget in EDI (unaff_EDI, unresolved register read, callee-saved self
// pointer carried over from widget_instance_render's own EDI), tag/dest/offset_xy/flags the four
// recognized stack parameters. // blam-cc: EDI -> widget, stack -> tag, dest, offset_xy, flags
// UNSURE: widget_instance_render.c's own extern declaration of this function (and its two call
// sites) omit the widget argument entirely and pass only (tag, dest, offset_xy, flags) --
// consistent with Ghidra showing zero visible args at those call sites, which is what exposed the
// implicit EDI input here. That file is outside this session's range and was not edited; a
// follow-up pass should add the widget/EDI argument to its declaration and both call sites.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void widget_instance_render(widget_instance *widget, Rectangle2D *dest, int32_t offset_xy,
                                    uint32_t flag1, int32_t flag2); // 0x49a8c0

// blam-cc: EDI -> widget, stack -> tag, dest, offset_xy, flags
// Propagates the widget's cumulative scale to its extended_description widget and renders it,
// then -- only when the tag marks the list's items as code-generated -- renders every child of
// `widget` in order, flagging the one at selection_index as selected. Always clears scroll_blink
// on the way out.
void widget_instance_render_column_list_items(widget_instance *widget, UIWidgetDefinition *tag, Rectangle2D *dest,
                   int32_t offset_xy, uint32_t flags)
{
    widget_instance *child;
    int16_t index;

    if (widget->extended_description != (widget_instance *)0) {
        float scale = widget->scale;
        widget_instance *ancestor;

        for (ancestor = widget->parent; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
            scale = scale * ancestor->scale;
        }
        widget->extended_description->scale = scale;
        widget_instance_render(widget->extended_description, dest, offset_xy, 0, 1);
    }

    if ((tag->flags_2 & 1) == 0) { // UNSURE: bit 0 of flags_2, list_items_generated_in_code
        widget->scroll_blink = 0;
        return;
    }

    child = widget->first_child;
    if (child == (widget_instance *)0) {
        widget->scroll_blink = 0;
        return;
    }
    index = 0;
    do {
        if ((uint16_t)widget->item_count <= (uint16_t)index) {
            break;
        }
        widget_instance_render(child, dest, offset_xy, flags, index == widget->selection_index);
        child = child->next_sibling;
        index = index + 1;
    } while (child != (widget_instance *)0);
    widget->scroll_blink = 0;
}

#if 0
Original Ghidra decompilation (0x49bac0):

void FUN_0049bac0(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  float fVar1;
  int iVar2;
  int iVar3;
  int unaff_EDI;

  if (*(int *)(unaff_EDI + 0x4c) != 0) {
    fVar1 = *(float *)(unaff_EDI + 0x24);
    for (iVar3 = *(int *)(unaff_EDI + 0x30); iVar3 != 0; iVar3 = *(int *)(iVar3 + 0x30)) {
      fVar1 = fVar1 * *(float *)(iVar3 + 0x24);
    }
    *(float *)(*(int *)(unaff_EDI + 0x4c) + 0x24) = fVar1;
    widget_instance_render(*(undefined4 *)(unaff_EDI + 0x4c),param_2,param_3,0,1);
  }
  if ((*(byte *)(param_1 + 0x150) & 1) == 0) {
    *(undefined2 *)(unaff_EDI + 0x42) = 0;
    return;
  }
  iVar3 = *(int *)(unaff_EDI + 0x34);
  iVar2 = 0;
  if (iVar3 == 0) {
    *(undefined2 *)(unaff_EDI + 0x42) = 0;
    return;
  }
  do {
    if ((int)(uint)*(ushort *)(unaff_EDI + 0x48) <= iVar2) break;
    widget_instance_render(iVar3,param_2,param_3,param_4,iVar2 == *(short *)(unaff_EDI + 0x40));
    iVar3 = *(int *)(iVar3 + 0x2c);
    iVar2 = iVar2 + 1;
  } while (iVar3 != 0);
  *(undefined2 *)(unaff_EDI + 0x42) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
