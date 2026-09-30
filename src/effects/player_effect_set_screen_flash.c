// player_effect_set_screen_flash  (Ghidra: FUN_004578a0, still unnamed there; named directly by
//   out/phase4/effects_types_notes.md: "player_effect_set_screen_flash 0x4578a0 (14 dwords into
//   +0x18)")
// address 0x4578a0, size 264 bytes
// name confidence: 0.55   rewrite confidence: 0.9 (VERIFIED against objdump 0x4578a0..0x4579a7; weight/maximum FIXED)
// evidence: types/effects.h player_effect.flash (player_screen_flash, +0x18) and flash_ticks
//   (+0xde); global 0x00687218 screen_flash_pass[8] (types/effects.h globals list). The source
//   descriptor is modeled as another player_screen_flash because the 14-dword copy exactly
//   matches that struct's own size and every sub-field this function reads back out of the
//   descriptor (type at +0x00, an unnamed priority-like field at +0x02, duration at +0x10, an
//   unnamed weight at +0x20, intensity at +0x24) lines up with the struct's own layout.
// register convention: player_effect self as the recognized stack parameter (param_1); a
//   ScreenFlash-shaped bounds descriptor in EBX (unaff_EBX); intensity falloff and duration
//   scale as the two remaining recognized stack parameters (param_2, param_3).
//   // blam-cc: stack -> self, unaff_EBX -> descriptor, stack -> (intensity_falloff, duration_scale)
// UNSURE: field names beyond type/duration/intensity are not established elsewhere, so the
//   "priority" and "weight" framing is inferred from how the values are compared/blended, not
//   from any named tag field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern int16_t screen_flash_pass[8]; // 0x00687218

// VERIFIED against disassembly 0x4578a0..0x4579a7 (2026-09-30): the update condition, the 14-dword copy, the ticks __ftol
//   (scaled duration), the blend/clamp and the flag byte match; NaN inputs now take the same path as the x87 compares.
//   STILL-UNSURE: the difftest's 65/200 mismatch (intensity 0 vs 0.6884) was not explained by this comparison.
void player_effect_set_screen_flash(player_effect *self, player_screen_flash *descriptor,
    float intensity_falloff, float duration_scale) // blam-cc: stack, unaff_EBX, stack, stack
{
    if ((self->flash.priority <= descriptor->priority ||
         (float)self->flash_ticks <= duration_scale * 30.0f * descriptor->duration) &&
        screen_flash_pass[descriptor->type] != 0) {
        float blended;

        self->flash = *descriptor;
        self->flash.duration = duration_scale * 30.0f * descriptor->duration;
        self->flash_ticks = (int16_t)self->flash.duration;

        // FIXED (objdump 0x457916..0x4579a3): the weight is descriptor +0x24 (intensity) and the clamp maximum is
        //   +0x20; the result lands in the copied flash's +0x24. The draft swapped the two fields.
        {
            float weight = descriptor->intensity;
            float maximum = *(float *)&descriptor->maximum_intensity;

            blended = (1.0f - weight) * intensity_falloff + weight;
            if (blended < 0.0f) {                 // 0x457929: fcomp 0; jp -> (>= 0 or unordered) continues
                self->flash.intensity = 0.0f;
            } else if (blended > maximum) {       // 0x457962: fcomp max; jne when <= (NaN stores blended)
                self->flash.intensity = maximum;
            } else {
                self->flash.intensity = blended;
            }
            self->flags |= _player_effect_screen_flash_bit;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4578a0):

void FUN_004578a0(int param_1,float param_2,float param_3)

{
  float fVar1;
  undefined2 uVar2;
  int iVar3;
  short *unaff_EBX;
  short *psVar4;
  undefined4 *puVar5;

  if (((*(short *)(param_1 + 0x1a) <= unaff_EBX[1]) ||
      ((float)(int)*(short *)(param_1 + 0xde) <= param_3 * 30.0 * *(float *)(unaff_EBX + 8))) &&
     (*(short *)(&DAT_00687218 + *unaff_EBX * 2) != 0)) {
    psVar4 = unaff_EBX;
    puVar5 = (undefined4 *)(param_1 + 0x18);
    for (iVar3 = 0xe; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *(undefined4 *)psVar4;
      psVar4 = psVar4 + 2;
      puVar5 = puVar5 + 1;
    }
    *(float *)(param_1 + 0x28) = param_3 * 30.0 * *(float *)(param_1 + 0x28);
    uVar2 = __ftol();
    *(undefined2 *)(param_1 + 0xde) = uVar2;
    if ((1.0 - *(float *)(unaff_EBX + 0x12)) * param_2 + *(float *)(unaff_EBX + 0x12) < 0.0) {
      *(undefined4 *)(param_1 + 0x3c) = 0;
      *(byte *)(param_1 + 0xe8) = *(byte *)(param_1 + 0xe8) | 1;
      return;
    }
    if (*(float *)(unaff_EBX + 0x10) <
        (1.0 - *(float *)(unaff_EBX + 0x12)) * param_2 + *(float *)(unaff_EBX + 0x12)) {
      *(undefined4 *)(param_1 + 0x3c) = *(undefined4 *)(unaff_EBX + 0x10);
      *(byte *)(param_1 + 0xe8) = *(byte *)(param_1 + 0xe8) | 1;
      return;
    }
    fVar1 = *(float *)(unaff_EBX + 0x12);
    *(byte *)(param_1 + 0xe8) = *(byte *)(param_1 + 0xe8) | 1;
    *(float *)(param_1 + 0x3c) = (1.0 - fVar1) * param_2 + fVar1;
  }
  return;
}
#endif
