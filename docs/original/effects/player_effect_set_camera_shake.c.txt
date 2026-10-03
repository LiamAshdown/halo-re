// player_effect_set_camera_shake  (Ghidra: FUN_00457d50, still unnamed there; named directly by
//   out/phase4/effects_types_notes.md: "player_effect_set_camera_shake 0x457d50 (18 floats into
//   +0x84)")
// address 0x457d50, size 201 bytes
// name confidence: 0.55   rewrite confidence: 0.9 (VERIFIED against objdump 0x457d50..0x457e18)
// evidence: types/effects.h player_effect.shake (player_camera_shake, +0x84), shake_ticks
//   (+0xe2), flags (+0xe8, _player_effect_camera_shake_bit) and player_camera_shake (intensity
//   +0x28, unknown_20 +0x20 "also multiplied by the caller scale and 30").
// register convention: intensity falloff and duration scale are the two recognized stack
//   parameters (param_1, param_2); a ScreenFlash-shaped... rather, a camera-shake-shaped bounds
//   descriptor in EAX (in_EAX); player_effect self in EBX (unaff_EBX).
//   // blam-cc: stack -> (intensity_falloff, duration_scale), in_EAX -> descriptor,
//   //   unaff_EBX -> self

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void player_effect_set_camera_shake(player_effect *self, player_camera_shake *descriptor,
    float intensity_falloff, float duration_scale) // blam-cc: unaff_EBX, in_EAX, stack, stack
{
    float duration = duration_scale * 30.0f;
    float blended = (1.0f - descriptor->intensity) * intensity_falloff + descriptor->intensity;

    if (duration * descriptor->duration <= (float)self->shake_ticks &&
        blended <= self->shake.intensity &&
        (blended < self->shake.intensity || duration * descriptor->duration <= (float)self->shake_ticks)) {
        return;
    }

    self->shake = *descriptor;
    self->shake.intensity = blended;
    self->shake.duration = duration * self->shake.duration;
    self->shake_ticks = (int16_t)self->shake.duration;
    self->flags |= _player_effect_camera_shake_bit;
    self->shake.wobble_period = duration * self->shake.wobble_period;
}

#if 0
Original Ghidra decompilation (0x457d50):

void FUN_00457d50(float param_1,float param_2)

{
  float *pfVar1;
  float fVar2;
  undefined2 uVar3;
  float *in_EAX;
  int iVar4;
  int unaff_EBX;
  float *pfVar5;

  param_2 = param_2 * 30.0;
  fVar2 = (1.0 - in_EAX[10]) * param_1 + in_EAX[10];
  if (((param_2 * *in_EAX <= (float)(int)*(short *)(unaff_EBX + 0xe2)) &&
      (fVar2 <= *(float *)(unaff_EBX + 0xac))) &&
     ((fVar2 < *(float *)(unaff_EBX + 0xac) ||
      (param_2 * *in_EAX <= (float)(int)*(short *)(unaff_EBX + 0xe2))))) {
    return;
  }
  pfVar1 = (float *)(unaff_EBX + 0x84);
  pfVar5 = pfVar1;
  for (iVar4 = 0x12; iVar4 != 0; iVar4 = iVar4 + -1) {
    *pfVar5 = *in_EAX;
    in_EAX = in_EAX + 1;
    pfVar5 = pfVar5 + 1;
  }
  *(float *)(unaff_EBX + 0xac) = fVar2;
  *pfVar1 = param_2 * *pfVar1;
  uVar3 = __ftol();
  *(undefined2 *)(unaff_EBX + 0xe2) = uVar3;
  *(byte *)(unaff_EBX + 0xe8) = *(byte *)(unaff_EBX + 0xe8) | 4;
  *(float *)(unaff_EBX + 0xa4) = param_2 * *(float *)(unaff_EBX + 0xa4);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
