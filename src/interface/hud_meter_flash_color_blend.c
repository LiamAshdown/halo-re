// hud_meter_flash_color_blend  (Ghidra: FUN_004ab980, renamed)
// address 0x4ab980, size 573 bytes
// name confidence: 0.45 (chosen)   rewrite confidence: 0.75
// evidence: rewritten in the phase-4 review from objdump -d 0x4ab980..0x4abbbc. ESI points at a
// hud_flash_parameters run inside a HUD interface tag element (default color, flashing color,
// period, delay, count, flags, length: the GrenadeHUDInterfaceOverlay +0x24 layout of
// types/tags.h), EDI is the game time (ticks) the flashing started, 0 for a steady state.
// The time since the start is converted to seconds (times 1/30, the float at 0x00672acc),
// wrapped by the flash period with fmod (0x628cca, the CRT _CIfmod), and once more by
// delay + length inside the flash window of number_of_flashes * (delay + length).
// Inside the flash length the weight s = sqrt(clamp(1 - (cos(2 pi u) + 1) / 2, 0, 1)), u the
// fraction of the length elapsed, blends the two colors; vector3d_lerp (EAX out, ECX a, EDX b,
// stack t, out = t a + (1 - t) b) covers alpha, red and green and the blue channel is inlined.
// The earlier rewrite had the color reads, the time base, the fmod and the branch targets wrong.
// UNSURE: 2 pi is the float 6.283 at 0x00672da0, kept as the literal.
// register convention: flash parameters in ESI, start time in EDI.
//   // blam-cc: flash -> ESI, start_time -> EDI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_math.h"

extern double cos(double x);           // FCOS
extern double sqrt(double x);          // FSQRT
extern double fmod(double x, double y); // 0x628cca, _CIfmod

extern game_time_globals *game_time; // 0x006f1d6c

extern void color_argb_int_to_real(ColorARGB *out, uint32_t packed); // 0x43f5a0, blam-cc: EAX out, ECX packed
extern uint32_t color_pack_argb_from_real(ColorARGB *color); // 0x497900


// out = s * a + (1 - s) * b over all four channels.
static uint32_t hud_flash_blend(ColorARGB *a, ColorARGB *b, float s)
{
    ColorARGB out;
    vector3d_lerp((real_vector3d *)&out, (real_vector3d *)a, (real_vector3d *)b, s);
    out.blue = (1.0f - s) * b->blue + a->blue * s;
    return color_pack_argb_from_real(&out);
}

// blam-cc: flash -> ESI, start_time -> EDI
// Returns the packed 0xAARRGGBB color of a HUD element with flashing parameters at this moment.
uint32_t hud_meter_flash_color_blend(const hud_flash_parameters *flash, int32_t start_time)
{
    ColorARGB default_color;
    ColorARGB flashing_color;
    float cycle_time;
    float flash_time;

    if (flash->flash_period == 0.0f || flash->flash_length == 0.0f) {
        color_argb_int_to_real(&default_color, *(uint32_t *)&flash->default_color);
        return color_pack_argb_from_real(&default_color); // 0x4abbb1: the callee leaves EAX = out
    }

    cycle_time = (float)fmod((float)(game_time->game_time - start_time) * (1.0f / 30.0f),
                             flash->flash_period);
    color_argb_int_to_real(&default_color, *(uint32_t *)&flash->default_color);
    color_argb_int_to_real(&flashing_color, *(uint32_t *)&flash->flashing_color);

    if ((float)flash->number_of_flashes * (flash->flash_delay + flash->flash_length) <= cycle_time) {
        return color_pack_argb_from_real(&default_color);
    }
    flash_time = (float)fmod(cycle_time, flash->flash_delay + flash->flash_length);

    if (start_time == 0) {
        return color_pack_argb_from_real((flash->flash_flags & 1) ? &default_color : &flashing_color);
    }
    if (flash_time < flash->flash_length) {
        double wave = 1.0 - (cos(flash_time / flash->flash_length * 6.283f) + 1.0) * 0.5;
        float s;
        if (wave < 0.0) {
            wave = 0.0;
        } else if (wave > 1.0) {
            wave = 1.0;
        }
        s = (float)sqrt(wave);
        if (flash->flash_flags & 1) {
            return hud_flash_blend(&default_color, &flashing_color, s);
        }
        return hud_flash_blend(&flashing_color, &default_color, s);
    }
    return color_pack_argb_from_real((flash->flash_flags & 1) ? &flashing_color : &default_color);
}

#if 0
Original Ghidra decompilation (0x4ab980):

void FUN_004ab980(void)

{
  float *pfVar1;
  int iVar2;
  int unaff_ESI;
  int unaff_EDI;
  float10 fVar3;
  float fVar4;
  float local_30;
  undefined4 uStack_2c;
  undefined4 uStack_28;
  float fStack_24;
  float fStack_20;
  undefined4 uStack_1c;
  undefined4 uStack_18;
  float fStack_14;
  float fStack_10;
  undefined4 uStack_c;
  undefined4 uStack_8;
  float fStack_4;

  if ((*(float *)(unaff_ESI + 8) == 0.0) || (*(float *)(unaff_ESI + 0x14) == 0.0)) {
    pfVar1 = (float *)color_argb_int_to_real();
    color_pack_argb_from_real(pfVar1);
    return;
  }
  iVar2 = *(int *)(DAT_006f1d6c + 0xc) - unaff_EDI;
  fVar3 = (float10)FUN_00628cca();
  fVar4 = (float)fVar3;
  color_argb_int_to_real(fVar4,iVar2);
  color_argb_int_to_real();
  if (fVar4 < (float)(int)*(short *)(unaff_ESI + 0x10) *
              (*(float *)(unaff_ESI + 0xc) + *(float *)(unaff_ESI + 0x14))) {
    fVar3 = (float10)FUN_00628cca();
    if (unaff_EDI == 0) {
      if ((*(byte *)(unaff_ESI + 0x12) & 1) != 0) goto LAB_004aba2b;
    }
    else {
      if ((float)fVar3 < *(float *)(unaff_ESI + 0x14)) {
        fVar3 = (float10)fcos(((float10)(float)fVar3 / (float10)*(float *)(unaff_ESI + 0x14)) *
                              (float10)6.283);
        fVar3 = (float10)1.0 - (fVar3 + (float10)1.0) * (float10)0.5;
        if ((float10)0.0 <= fVar3) {
          if ((float10)1.0 < fVar3) {
            fVar3 = (float10)1.0;
          }
        }
        else {
          fVar3 = (float10)0.0;
        }
        fVar4 = (float)SQRT(fVar3);
        if ((*(byte *)(unaff_ESI + 0x12) & 1) != 0) {
          pfVar1 = (float *)vector3d_lerp(fVar4);
          fStack_24 = fStack_14 * fVar4 + (1.0 - fVar4) * fStack_4;
          color_pack_argb_from_real(pfVar1);
          return;
        }
        pfVar1 = (float *)vector3d_lerp(fVar4);
        fStack_24 = fStack_4 * fVar4 + (1.0 - fVar4) * fStack_14;
        color_pack_argb_from_real(pfVar1);
        return;
      }
      if ((*(byte *)(unaff_ESI + 0x12) & 1) == 0) {
        uStack_28 = uStack_18;
        local_30 = fStack_20;
        uStack_2c = uStack_1c;
        fStack_24 = fStack_14;
        color_pack_argb_from_real(&local_30);
        return;
      }
    }
    local_30 = fStack_10;
    fStack_24 = fStack_4;
    uStack_2c = uStack_c;
    uStack_28 = uStack_8;
    color_pack_argb_from_real(&local_30);
    return;
  }
LAB_004aba2b:
  uStack_2c = uStack_1c;
  local_30 = fStack_20;
  uStack_28 = uStack_18;
  fStack_24 = fStack_14;
  color_pack_argb_from_real(&local_30);
  return;
}
#endif
