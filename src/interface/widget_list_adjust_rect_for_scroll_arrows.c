// widget_list_adjust_rect_for_scroll_arrows  (Ghidra: FUN_00499990, unnamed;
// out/phase2/results/interface_01.json names it widget_list_adjust_rect_for_scroll_arrows,
// conf=0.45)
// address 0x499990, size 95 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: phase-2 evidence plus offsets recomputed field-by-field from types/tags.h's
// UIWidgetDefinition layout (pack(1), summed byte by byte): +0x3e0 is child_widgets (a
// TagReflexive, .count at +0), +0x160/+0x170 are list_header_bitmap.tag_id / list_footer_bitmap
// .tag_id (each TagDependency's trailing TagID), and +0x176/+0x182 are header_bounds.left /
// footer_bounds.right (each Rectangle2D's {top,left,bottom,right} layout). The implicit ECX
// argument is a Rectangle2D whose ->left and ->right fields (offsets 2 and 6) are the only ones
// touched, matching the summary's "shrink/reposition the rect" reading.
// register convention: widget in EAX (in_EAX), rect in ECX (in_ECX), both unresolved register
// reads.
// blam-cc: EAX -> widget, ECX -> rect

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

// blam-cc: EAX -> widget, ECX -> rect
// For a spinner_list widget with fewer than two children and both scroll-arrow bitmaps set,
// shrinks the given layout rectangle to leave room for them: nudges the left edge in by the
// header bitmap's own left margin minus 10, and grows the right edge out to clear the footer
// bitmap's right margin plus 2.
void widget_list_adjust_rect_for_scroll_arrows(widget_instance *widget, Rectangle2D *rect)
{
    UIWidgetDefinition *tag;

    if (widget->widget_type != 2 /* spinner_list */) {
        return;
    }
    tag = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;
    if (tag->child_widgets.count >= 2) {
        return;
    }
    if (*(uint32_t *)&tag->list_footer_bitmap.tag_id == 0xffffffffu ||
        *(uint32_t *)&tag->list_header_bitmap.tag_id == 0xffffffffu) {
        return;
    }
    rect->left = rect->left + tag->header_bounds.left - 10;
    if (rect->right <= tag->footer_bounds.right) {
        rect->right = tag->footer_bounds.right + 2;
    }
}

#if 0
Original Ghidra decompilation (0x499990):

void FUN_00499990(void)

{
  int iVar1;
  uint *in_EAX;
  int in_ECX;

  if ((((*(short *)((int)in_EAX + 0xe) == 2) &&
       (iVar1 = *(int *)((*in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
       *(int *)(iVar1 + 0x3e0) < 2)) && (*(int *)(iVar1 + 0x170) != -1)) &&
     (*(int *)(iVar1 + 0x160) != -1)) {
    *(short *)(in_ECX + 2) = *(short *)(in_ECX + 2) + *(short *)(iVar1 + 0x176) + -10;
    if (*(short *)(in_ECX + 6) <= *(short *)(iVar1 + 0x182)) {
      *(short *)(in_ECX + 6) = *(short *)(iVar1 + 0x182) + 2;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
