// ui_get_saved_pulse_color  (Ghidra: FUN_0049c620, renamed)
// renamed from FUN_0049c620 in the naming pass
// address 0x49c620, size 93 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// evidence: functions.md: "Builds a 4-component colour value (a base colour plus saved RGB) used
// as the source colour for a widget's pulsing or animated tint effect." types/interface.h: "global
// 0x006927b8: float ui_saved_color[3]  read by 0x49c5c0 and 0x49c620." widget_instance_render_
// text_box.c already fixed the register convention (EAX -> out) and, from its own objdump reading,
// established this returns `out` in EAX even though Ghidra shows no explicit return here either.
// reconciled: 0x006851fc is a pointer to the opaque-white ColorARGB (0x00655138); one name global_white_argb: fixed by widget_instance_render_text_box.c: EAX -> out.
// blam-cc: EAX -> out
// Phase-4 s2 review (objdump 0x49c620..0x49c67c): 0x006851fc holds a pointer to a ColorARGB;
// the routine copies all four floats out of it and then overwrites red, green and blue with
// ui_saved_color, so only its alpha survives. The earlier rewrite read 0x006851fc as the float
// itself. ui_list_widget_rebuild_rows.c now uses the same (ColorARGB *out) signature.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const ColorARGB *global_white_argb; // 0x006851fc, a pointer: only its alpha survives
extern float ui_saved_color[3]; // 0x006927b8

// blam-cc: EAX -> out
ColorARGB *ui_get_saved_pulse_color(ColorARGB *out)
{
    out->alpha = global_white_argb->alpha; // mov edx,[0x006851fc]; mov ecx,[edx]
    out->red = ui_saved_color[0];
    out->green = ui_saved_color[1];
    out->blue = ui_saved_color[2];
    return out;
}

#if 0
Original Ghidra decompilation (0x49c620):

void FUN_0049c620(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 *in_EAX;

  uVar3 = DAT_006927c0;
  uVar2 = DAT_006927bc;
  uVar1 = DAT_006927b8;
  *in_EAX = *(undefined4 *)PTR_DAT_006851fc;
  in_EAX[1] = uVar1;
  in_EAX[2] = uVar2;
  in_EAX[3] = uVar3;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
