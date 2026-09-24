// widget_instance_get_cumulative_scale  (Ghidra: widget_instance_get_cumulative_scale, already
// named)
// address 0x499c20, size 27 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: matches the given name and types/interface.h's widget_instance note ("multiplies it
// [scale] by the same field of every ancestor reached through +0x30").
// register convention: widget in EAX (in_EAX), unresolved register read. Ghidra types the
// result float10 (x87 extended precision); returned here as a plain float, matching every other
// scale-typed field in types/interface.h.
// blam-cc: EAX -> widget

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

// blam-cc: EAX -> widget
// Returns widget->scale multiplied by every ancestor's own scale, walking up the parent chain.
float widget_instance_get_cumulative_scale(widget_instance *widget)
{
    float scale = widget->scale;
    widget_instance *ancestor;

    for (ancestor = widget->parent; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
        scale = scale * ancestor->scale;
    }
    return scale;
}

#if 0
Original Ghidra decompilation (0x499c20):

float10 widget_instance_get_cumulative_scale(void)

{
  int iVar1;
  int in_EAX;
  float10 fVar2;

  fVar2 = (float10)*(float *)(in_EAX + 0x24);
  for (iVar1 = *(int *)(in_EAX + 0x30); iVar1 != 0; iVar1 = *(int *)(iVar1 + 0x30)) {
    fVar2 = fVar2 * (float10)*(float *)(iVar1 + 0x24);
  }
  return fVar2;
}
#endif
