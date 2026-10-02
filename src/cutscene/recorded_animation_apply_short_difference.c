// recorded_animation_apply_short_difference  (Ghidra: FUN_0044a150; renamed per types notes)
// address 0x44a150, size 61 bytes
// name confidence: 0.8 (symbols/agent_phase4_cutscene.txt)   rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "0x44a110 and 0x44a150 wrap only the yaw ...
// EAX is the pair, EDX the delta." types/cutscene.h recorded_animation_angles (int16 yaw,
// pitch) and recorded_animation_short_difference (int16 yaw, pitch); the compressed-difference
// payload for the 16 bit delta format (bysw vector_short_difference_data).
// register convention: EAX = angles (in_EAX), EDX = delta (in_EDX). blam-cc: EAX -> angles,
// EDX -> delta.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

// blam-cc: EAX -> angles, EDX -> delta
// Identical to recorded_animation_apply_char_difference, but the delta is a full 16 bit
// yaw/pitch pair (the "large delta" compressed format) instead of a signed byte pair.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void recorded_animation_apply_short_difference(recorded_animation_angles *angles,
    recorded_animation_short_difference *delta)
{
    int16_t yaw;

    angles->yaw = angles->yaw + delta->yaw;
    yaw = angles->yaw;
    if (1000 < yaw) {
        angles->yaw = yaw - 1000;
        angles->pitch = angles->pitch + delta->pitch;
        return;
    }
    if (yaw < -1000) {
        angles->yaw = yaw + 1000;
    }
    angles->pitch = angles->pitch + delta->pitch;
}

#if 0
Original Ghidra decompilation (0x44a150):

void FUN_0044a150(void)

{
  short sVar1;
  short *in_EAX;
  short *in_EDX;

  *in_EAX = *in_EAX + *in_EDX;
  sVar1 = *in_EAX;
  if (1000 < sVar1) {
    *in_EAX = sVar1 + -1000;
    in_EAX[1] = in_EAX[1] + in_EDX[1];
    return;
  }
  if (sVar1 < -1000) {
    *in_EAX = sVar1 + 1000;
  }
  in_EAX[1] = in_EAX[1] + in_EDX[1];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
