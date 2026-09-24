// effect_set_placement  (Ghidra: FUN_00451600; named per out/phase4/effects_types_notes.md, which
// refers to this address directly throughout the tint_source evidence section: "effect_set_placement
// 0x451600 copies three dwords from a caller vector into +0x30/+0x34/+0x38 ... clears only +0x34
// and +0x38 on the null path, leaving +0x30 alone")
// address 0x451600, size 83 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/effects.h effect (a_scale 0x44, b_scale 0x48, color 0x18, tint_source 0x30).
// register convention: effect* in EAX (in_EAX), colour pointer in ECX (in_ECX, NULL selects the
// default), tint_source pointer in EDX (in_EDX, NULL clears proc/unknown_08 only); a_scale and
// b_scale are Ghidra's own recognised stack parameters.
//   // blam-cc: EAX -> self, ECX -> color, EDX -> tint_source, stack -> (a_scale, b_scale)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern const ColorRGB *const *default_effect_color_pointer; // 0x00686b04, UNSURE, see
    // effect_new_at_texture_coordinate.c

// Sets an effect's per-instance placement inputs: the A/B scale values, its tint colour
// (defaulting to *default_effect_color_pointer when `color` is NULL), and its optional
// per-position tint source callback (cleared, except for its data pointer, when `tint_source` is
// NULL).
void effect_set_placement(effect *self, const ColorRGB *color, const effect_tint_source *tint_source,
    real a_scale, real b_scale)
{
    self->a_scale = a_scale;
    self->b_scale = b_scale;

    if (color == 0) {
        color = *default_effect_color_pointer;
    }
    self->color = *color;

    if (tint_source != 0) {
        self->tint_source = *tint_source;
    } else {
        self->tint_source.proc = 0;
        self->tint_source.unknown_08 = 0;
    }
}

#if 0
Original Ghidra decompilation (0x451600):

void FUN_00451600(undefined4 param_1,undefined4 param_2)

{
  int in_EAX;
  undefined4 *in_ECX;
  undefined4 *in_EDX;

  *(undefined4 *)(in_EAX + 0x44) = param_1;
  *(undefined4 *)(in_EAX + 0x48) = param_2;
  if (in_ECX == (undefined4 *)0x0) {
    in_ECX = (undefined4 *)PTR_DAT_00686b04;
  }
  *(undefined4 *)(in_EAX + 0x18) = *in_ECX;
  *(undefined4 *)(in_EAX + 0x1c) = in_ECX[1];
  *(undefined4 *)(in_EAX + 0x20) = in_ECX[2];
  if (in_EDX != (undefined4 *)0x0) {
    *(undefined4 *)(in_EAX + 0x30) = *in_EDX;
    *(undefined4 *)(in_EAX + 0x34) = in_EDX[1];
    *(undefined4 *)(in_EAX + 0x38) = in_EDX[2];
    return;
  }
  *(undefined4 *)(in_EAX + 0x34) = 0;
  *(undefined4 *)(in_EAX + 0x38) = 0;
  return;
}
#endif
