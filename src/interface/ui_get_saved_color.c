// ui_get_saved_color  (Ghidra: FUN_0049c5c0, renamed)
// renamed from FUN_0049c5c0 in the naming pass
// address 0x49c5c0, size 82 bytes
// name confidence: 0.35   rewrite confidence: 0.8
// evidence: functions.md: "Copies a previously saved three-component colour value from globals
// into the caller's output buffer." types/interface.h: "global 0x006927b8: float
// ui_saved_color[3]  read by 0x49c5c0 and 0x49c620." widget_instance_render_list_head.c (already
// rewritten, outside this range) already fixed the register convention (EAX -> out) and, from its
// own objdump reading of the call site, established that this function returns `out` in EAX even
// though Ghidra's own decompile of this address shows no explicit return.
// register convention: fixed by widget_instance_render_list_head.c: EAX -> out.
// blam-cc: EAX -> out
// UNSURE: that same caller file types `out` as `ColorARGB *` even though only 3 floats are ever
// written here; declared as `ColorRGB *` here instead (types/tags.h's exact 3-float type), which
// is compatible at the call site but is a real cross-file type mismatch worth reconciling.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern float ui_saved_color[3]; // 0x006927b8

// blam-cc: EAX -> out
ColorRGB *ui_get_saved_color(ColorRGB *out)
{
    out->red = ui_saved_color[0];
    out->green = ui_saved_color[1];
    out->blue = ui_saved_color[2];
    return out;
}

#if 0
Original Ghidra decompilation (0x49c5c0):

void FUN_0049c5c0(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 *in_EAX;

  uVar2 = DAT_006927c0;
  uVar1 = DAT_006927bc;
  *in_EAX = DAT_006927b8;
  in_EAX[1] = uVar1;
  in_EAX[2] = uVar2;
  return;
}
#endif
