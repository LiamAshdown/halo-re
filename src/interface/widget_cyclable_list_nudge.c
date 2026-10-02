// widget_cyclable_list_nudge  (Ghidra: FUN_004a2070, renamed)
// renamed from FUN_004a2070 in the naming pass
// address 0x4a2070, size 116 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: functions.md: "Handles left/right navigation input for a cyclable list widget,
// adjusting its selected index with wraparound and triggering a refresh." Uses widget_cursor_side_of_midpoint's
// cursor-side result (this session) to decide left (-1) vs right (+1) and wraps the index at the
// widget's item_count.
// register convention: cdecl, the one recognized stack parameter (widget).
// UNSURE: offsets 0x42/0x54 are set to small animation-direction-shaped constants (0xfffc/-1 for
// left, 4/1 for right) whose exact meaning was not resolved; preserved as raw writes rather than
// named fields.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t widget_cursor_side_of_midpoint(widget_instance *widget); // 0x4a1ff0
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90

uint32_t widget_cyclable_list_nudge(widget_instance *widget)
{
    int32_t side = widget_cursor_side_of_midpoint(widget);
    int32_t new_index;

    if (side < 1) {
        new_index = widget->selection_index - 1;
        if (new_index < 0) {
            new_index = (uint16_t)widget->item_count - 1;
        }
        if (new_index == widget->selection_index) {
            goto play_and_return;
        }
        ((struct widget_instance *)widget)->scroll_blink = (int16_t)0xfffc;
        ((struct widget_instance *)widget)->selection_direction = -1;
    } else {
        if (side != 1) {
            return 1;
        }
        new_index = widget->selection_index + 1;
        if (new_index >= (uint16_t)widget->item_count) {
            new_index = 0;
        }
        if (new_index == widget->selection_index) {
            goto play_and_return;
        }
        ((struct widget_instance *)widget)->scroll_blink = 4;
        ((struct widget_instance *)widget)->selection_direction = 1;
    }
    widget->selection_index = (int16_t)new_index;
    widget_play_sound_effect(0); // UNSURE: effect id read from an unresolved register here

play_and_return:
    widget_play_sound_effect(0); // UNSURE: effect id read from an unresolved register here
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a2070):

undefined4 FUN_004a2070(int param_1)

{
  int iVar1;

  iVar1 = FUN_004a1ff0();
  if (iVar1 < 1) {
    iVar1 = *(short *)(param_1 + 0x40) + -1;
    if (iVar1 < 0) {
      iVar1 = *(ushort *)(param_1 + 0x48) - 1;
    }
    if (iVar1 == *(short *)(param_1 + 0x40)) goto LAB_004a20d6;
    *(undefined2 *)(param_1 + 0x42) = 0xfffc;
    *(undefined2 *)(param_1 + 0x54) = 0xffff;
  }
  else {
    if (iVar1 != 1) {
      return 1;
    }
    iVar1 = *(short *)(param_1 + 0x40) + 1;
    if ((int)(uint)*(ushort *)(param_1 + 0x48) <= iVar1) {
      iVar1 = 0;
    }
    if (iVar1 == *(short *)(param_1 + 0x40)) goto LAB_004a20d6;
    *(undefined2 *)(param_1 + 0x42) = 4;
    *(undefined2 *)(param_1 + 0x54) = 1;
  }
  *(short *)(param_1 + 0x40) = (short)iVar1;
  widget_play_sound_effect();
LAB_004a20d6:
  widget_play_sound_effect();
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
