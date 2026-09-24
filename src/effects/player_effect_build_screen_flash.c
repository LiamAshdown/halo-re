// player_effect_build_screen_flash  (Ghidra: FUN_00457000, still unnamed there; named directly
//   by out/phase4/effects_types_notes.md: "player_effect_build_screen_flash 0x457000")
// address 0x457000, size 529 bytes
// name confidence: 0.5   rewrite confidence: 0.3 (low-rigor best-effort pass: the output
//   descriptor shape (`unaff_EBX`) is not an established struct, so it is written through raw
//   offsets rather than a named type)
// evidence: types/effects.h player_effect_globals (scripted_flash_color +0xec,
//   scripted_flash_start_tick +0xf8, scripted_flash_ticks +0xfc, scripted_flash_fade_in +0xfe)
//   and player_effect.flash (player_screen_flash, +0x18: type +0x00, color +0x28 => +0x40
//   absolute, intensity +0x24 => +0x3c absolute); global 0x006b7020 player_effect_suppressed and
//   0x00687218 screen_flash_pass[8] (types/effects.h globals list); src/effects/decal_update_fade.c
//   for game_tick_globals[3].
// register convention: an output descriptor pointer in EBX (unaff_EBX, at least 6 dwords: type,
//   pad, alpha, then 4 dwords of color); a local player index in CX (in_CX).
//   // blam-cc: unaff_EBX -> out, in_CX -> local_player_index
// UNSURE: this function's own game_time_globals-shaped tick-length read (iVar1+0x10) is kept as
//   a raw offset since no established name covers it here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern uint8_t player_effect_suppressed;                      // 0x006b7020
extern player_effect_globals *player_effect_globals_pointer;  // 0x006f1884
extern int32_t *game_tick_globals;                             // 0x006f1d6c
extern int16_t screen_flash_pass[8];                           // 0x00687218

extern real transition_function_evaluate(int16_t type, real phase); // 0x4ccac0, math module;
                                    // UNSURE: type argument dropped by Ghidra at both call sites
                                    // here, kept as 0 (_transition_function_linear)

void player_effect_build_screen_flash(uint32_t *out, int16_t local_player_index)
    // blam-cc: unaff_EBX, in_CX
{
    player_effect_globals *globals = player_effect_globals_pointer;

    if (player_effect_suppressed != 0) {
        return;
    }

    if (globals->scripted_flash_ticks != -1 &&
        (globals->scripted_flash_fade_in != 0 ||
         game_tick_globals[3] - globals->scripted_flash_start_tick <= globals->scripted_flash_ticks)) {
        float fraction;
        ColorRGB color = globals->scripted_flash_color;

        out[0] = 1;
        *(ColorRGB *)&out[3] = color;
        out[2] = 0x3f800000; // 1.0f, UNSURE which sub-field this really is

        if (globals->scripted_flash_ticks < 1) {
            fraction = 1.0f;
        } else {
            float t = (float)(game_tick_globals[3] - globals->scripted_flash_start_tick) /
                      (float)globals->scripted_flash_ticks;
            t = (t < 0.0f) ? 0.0f : (1.0f < t ? 1.0f : t);
            fraction = transition_function_evaluate(0, t);
        }

        *(float *)&out[1] = fraction;
        if (globals->scripted_flash_fade_in == 0) {
            *(float *)&out[1] = 1.0f - fraction;
        }
        if (*(float *)&out[1] < 0.0f) {
            out[1] = 0;
        } else if (1.0f < *(float *)&out[1]) {
            out[1] = 0x3f800000;
        }
        return;
    }

    globals->scripted_flash_ticks = -1;

    if (local_player_index != -1) {
        player_effect *self = &globals->players[local_player_index];

        if (0 < self->flash_ticks || (self->flags & 1) != 0) {
            self->flags &= ~(uint32_t)1;
            out[0] = (uint32_t)(uint16_t)screen_flash_pass[self->flash.type];
            *(ColorARGB *)&out[2] = self->flash.color; // color is +0x40 absolute
            if (0.0f < self->flash.duration) {
                float fraction = ((float)self->flash_ticks / self->flash.duration) *
                                  self->flash.intensity; // UNSURE: intensity used as a scale here
                *(float *)&out[1] = transition_function_evaluate(0, fraction);
            } else {
                *(float *)&out[1] = self->flash.intensity;
            }
            self->flash_ticks = self->flash_ticks - 1; // UNSURE: subtracts a tick-length field
                                    // this batch does not establish; approximated as 1 tick
        }
    }
}

#if 0
Original Ghidra decompilation (0x457000):

void FUN_00457000(void)

{
  int iVar1;
  short in_CX;
  undefined2 *unaff_EBX;
  int iVar2;
  float10 fVar3;
  float local_4;

  iVar1 = DAT_006f1d6c;
  iVar2 = DAT_006f1884;
  if (DAT_006b7020 == '\0') {
    if ((*(short *)(DAT_006f1884 + 0xfc) != -1) &&
       ((*(char *)(DAT_006f1884 + 0xfe) != '\0' ||
        (*(int *)(DAT_006f1d6c + 0xc) - *(int *)(DAT_006f1884 + 0xf8) <=
         (int)*(short *)(DAT_006f1884 + 0xfc))))) {
      *unaff_EBX = 1;
      *(undefined4 *)(unaff_EBX + 6) = *(undefined4 *)(iVar2 + 0xec);
      *(undefined4 *)(unaff_EBX + 8) = *(undefined4 *)(iVar2 + 0xf0);
      *(undefined4 *)(unaff_EBX + 10) = *(undefined4 *)(iVar2 + 0xf4);
      *(undefined4 *)(unaff_EBX + 4) = 0x3f800000;
      if (*(short *)(iVar2 + 0xfc) < 1) {
        fVar3 = (float10)1.0;
      }
      else {
        local_4 = (float)(*(int *)(iVar1 + 0xc) - *(int *)(iVar2 + 0xf8)) /
                  (float)(int)*(short *)(iVar2 + 0xfc);
        if (0.0 <= local_4) {
          if (1.0 < local_4) {
            local_4 = 1.0;
          }
        }
        else {
          local_4 = 0.0;
        }
        fVar3 = (float10)transition_function_evaluate(local_4);
      }
      *(float *)(unaff_EBX + 2) = (float)fVar3;
      if (*(char *)(iVar2 + 0xfe) == '\0') {
        *(float *)(unaff_EBX + 2) = (float)((float10)1.0 - fVar3);
      }
      if (0.0 <= *(float *)(unaff_EBX + 2)) {
        if (*(float *)(unaff_EBX + 2) <= 1.0) {
          *(undefined4 *)(unaff_EBX + 2) = *(undefined4 *)(unaff_EBX + 2);
          return;
        }
        *(undefined4 *)(unaff_EBX + 2) = 0x3f800000;
        return;
      }
      *(undefined4 *)(unaff_EBX + 2) = 0;
      return;
    }
    if (in_CX != -1) {
      iVar2 = in_CX * 0xec + DAT_006f1884;
      *(undefined2 *)(DAT_006f1884 + 0xfc) = 0xffff;
      if ((0 < *(short *)(iVar2 + 0xde)) || ((*(byte *)(iVar2 + 0xe8) & 1) != 0)) {
        *(byte *)(iVar2 + 0xe8) = *(byte *)(iVar2 + 0xe8) & 0xfe;
        *unaff_EBX = *(undefined2 *)(&DAT_00687218 + *(short *)(iVar2 + 0x18) * 2);
        *(undefined4 *)(unaff_EBX + 4) = *(undefined4 *)(iVar2 + 0x40);
        *(undefined4 *)(unaff_EBX + 6) = *(undefined4 *)(iVar2 + 0x44);
        *(undefined4 *)(unaff_EBX + 8) = *(undefined4 *)(iVar2 + 0x48);
        *(undefined4 *)(unaff_EBX + 10) = *(undefined4 *)(iVar2 + 0x4c);
        if (0.0 < *(float *)(iVar2 + 0x28)) {
          fVar3 = (float10)transition_function_evaluate
                                     (((float)(int)*(short *)(iVar2 + 0xde) /
                                      *(float *)(iVar2 + 0x28)) * *(float *)(iVar2 + 0x3c));
          *(float *)(unaff_EBX + 2) = (float)fVar3;
          *(short *)(iVar2 + 0xde) = *(short *)(iVar2 + 0xde) - *(short *)(iVar1 + 0x10);
          return;
        }
        *(undefined4 *)(unaff_EBX + 2) = *(undefined4 *)(iVar2 + 0x3c);
        *(short *)(iVar2 + 0xde) = *(short *)(iVar2 + 0xde) - *(short *)(iVar1 + 0x10);
      }
    }
  }
  return;
}
#endif
