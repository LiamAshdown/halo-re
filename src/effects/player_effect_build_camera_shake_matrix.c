// player_effect_build_camera_shake_matrix  (Ghidra: FUN_00457390, still unnamed there; named
//   directly by types/effects.h: "player_effect_build_camera_shake_matrix 0x457390 does the same
//   [as player_effect_build_screen_flash] for the shake")
// address 0x457390, size 1295 bytes
// name confidence: 0.5   rewrite confidence: 0.2 (LOW-rigor best-effort pass, matching
//   src/effects/player_effect_build_screen_flash.c's own precedent for this sibling function:
//   several offsets below have no established field name and are kept raw)
// evidence: types/effects.h player_effect_globals (scripted_shake_flags +0x120, ..._ticks
//   +0x11c, ..._duration +0x11e, ..._intensity +0x118, ..._rotation[3] +0x100,
//   ..._translation[3] +0x10c) and player_effect.flags (_player_effect_camera_shake_bit);
//   global 0x0069673c (UNSURE identity, see src/game's own "render_ptr_9673c" for the same
//   address) copied 13 dwords into `out`, matching a real_matrix4x3; src/math's
//   matrix4x3_from_euler_angles / matrix4x3_from_axis_angle / matrix4x3_multiply and this
//   module's transition_function_evaluate / periodic_function_evaluate.
// register convention: output matrix pointer as the recognized stack parameter (param_1); local
//   player index in CX (in_CX).
//   // blam-cc: stack -> out, in_CX -> local_player_index
// UNSURE (heavily): the shake_ticks-shaped field this function tests at player_effect+0xe0 is
//   documented elsewhere (types/effects.h) as impulse_ticks, not shake_ticks (+0xe2) -- kept as
//   the raw offset exactly as decompiled rather than "corrected" to a field name that would
//   contradict the disassembly. `vector3d_cross_product`'s two operands and
//   `matrix4x3_from_axis_angle`'s axis argument are fully elided by Ghidra; reconstructed as the
//   player's own impulse_direction/impulse_rotation vectors (the only two nearby vectors in
//   scope), which is a guess, not evidence. The two matrix4x3_from_euler_angles argument-to-field
//   pairings (scripted_shake_translation feeding a *rotation* builder, scripted_shake_rotation
//   feeding the matrix's *position*) are preserved exactly as the raw offsets decompile, even
//   though the names in types/effects.h suggest they should be swapped -- see that header's own
//   "scripted_shake_rotation/translation" comments for the discrepancy this exposes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern player_effect_globals *player_effect_globals_pointer; // 0x006f1884
extern int32_t *game_time;                             // 0x006f1d6c
extern real_matrix4x3 *k_render_identity_matrix_ptr;           // 0x0069673c, UNSURE identity
                                    // (see src/game's own "render_ptr_9673c" for this address)
extern random_seed effect_random_seed;                         // 0x00719cd4
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b,
    real_matrix4x3 *out); // 0x00696664, math module

extern double cos(double x);
extern double sin(double x);
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *a, real_vector3d *b); // 0x4052c0
extern void matrix4x3_from_axis_angle(real_matrix4x3 *out, real_vector3d *axis, real sin_angle,
                                       real cos_angle); // 0x4cb880
extern void matrix4x3_from_euler_angles(real_matrix4x3 *out, real yaw, real pitch, real roll); // 0x4cba10
extern real transition_function_evaluate(int16_t type, real phase); // 0x4ccac0, math module;
                                    // UNSURE: type argument dropped by Ghidra, kept as 0
extern real periodic_function_evaluate(int16_t type, real phase); // 0x4cc9b0, math module;
                                    // UNSURE: type argument dropped by Ghidra, kept as 0
extern void player_effect_random_shake_offset(real_matrix4x3 *out, real magnitude, real angle); // 0x457280,
                                    // this module

void player_effect_build_camera_shake_matrix(real_matrix4x3 *out, int16_t local_player_index)
    // blam-cc: stack -> out, in_CX -> local_player_index
{
    player_effect_globals *globals = player_effect_globals_pointer;

    if (local_player_index == -1) {
        return;
    }

    if ((globals->scripted_shake_flags & 1) != 0) {
        real t = globals->scripted_shake_intensity;

        *out = *k_render_identity_matrix_ptr;

        if (globals->scripted_shake_ticks < 1) {
            if ((globals->scripted_shake_flags & 2) != 0) {
                globals->scripted_shake_flags &= ~(uint32_t)2;
            }
        } else {
            if ((globals->scripted_shake_flags & 2) == 0) {
                t = 1.0f - (real)globals->scripted_shake_ticks / (real)globals->scripted_shake_duration;
            } else {
                t = (real)globals->scripted_shake_ticks / (real)globals->scripted_shake_duration;
            }
            t = t * globals->scripted_shake_intensity;
            globals->scripted_shake_ticks =
                globals->scripted_shake_ticks - *(int16_t *)((uint8_t *)game_time + 0x10);
        }

        if ((globals->scripted_shake_flags & 1) == 0) {
            return;
        }

        t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);

        // Three advances per block, and the ORDER matters: the original pairs the LAST draw
        // with the first argument and the FIRST draw with the last. The conversion is unsigned
        // -- `(float)(seed >> 0x10) * 1.5259022e-05` -- so the fraction is in [0, 1) and the
        // `(x + x) - 1` below maps it onto [-1, 1). An earlier draft of this file cast the
        // shifted word to int16_t first, which halves the range and makes it signed, and paired
        // the draws with the wrong axes.
        {
            real draw1, draw2, draw3;

            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            draw1 = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;
            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            draw2 = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;
            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            draw3 = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;

            matrix4x3_from_euler_angles(out,
                ((draw3 + draw3) - 1.0f) * globals->scripted_shake_translation[0] * t,
                ((draw2 + draw2) - 1.0f) * globals->scripted_shake_translation[1] * t,
                ((draw1 + draw1) - 1.0f) * globals->scripted_shake_translation[2] * t); // UNSURE,
                                    // see file header
        }
        {
            real draw1, draw2, draw3;

            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            draw1 = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;
            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            draw2 = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;
            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            draw3 = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;

            // The raw offsets are +0x104, +0x100, +0x108 in that order, i.e. rotation[1],
            // rotation[0], rotation[2] -- NOT [0], [1], [2].
            out->position.x = ((draw3 + draw3) - 1.0f) * globals->scripted_shake_rotation[1] * t;
            out->position.y = ((draw2 + draw2) - 1.0f) * globals->scripted_shake_rotation[0] * t;
            out->position.z = ((draw1 + draw1) - 1.0f) * globals->scripted_shake_rotation[2] * t;
                                    // UNSURE, see file header
        }
        return;
    }

    {
        uint8_t *self = (uint8_t *)&globals->players[local_player_index];
        real_matrix4x3 *base;
        real t;
        uint8_t inactive = *(int16_t *)(self + 0xe0) < 1; // UNSURE, see file header

        if (inactive) {
            if ((*(uint32_t *)(self + 0xe8) & 2) == 0) {
                *out = *k_render_identity_matrix_ptr;
                *(uint16_t *)(self + 0xe2) = *(uint16_t *)(self + 0xe2) -
                    *(int16_t *)((uint8_t *)game_time + 0x10);
                goto write_scale_and_check_impulse;
            }
            t = 1.0f;
        } else {
            if ((*(uint32_t *)(self + 0xe8) & 2) != 0) {
                t = 1.0f;
            } else {
                real duration = *(real *)(self + 0x9c); // player_camera_shake.duration, 0x84+0x18
                t = transition_function_evaluate(0, duration);
            }
        }

        {
            real_vector3d axis;

            *(uint8_t *)(self + 0xe8) = *(uint8_t *)(self + 0xe8) & ~(uint8_t)2; // UNSURE bit clear
            vector3d_cross_product(&axis, (real_vector3d *)self, (real_vector3d *)(self + 0x0c)); // UNSURE,
                                    // see file header
            {
                real angle = t * *(real *)(self + 0xa0); // player_camera_shake.unknown_20, 0x84+0x1c
                real cos_angle = (real)cos((double)angle);
                real sin_angle = (real)sin((double)angle);
                real_matrix4x3 rotation;

                matrix4x3_from_axis_angle(&rotation, &axis, sin_angle, cos_angle);
                base = &rotation;

                out->position.x = t * *(real *)(self + 0xa4) + axis.i * angle * *(real *)(self + 0x88);
                out->position.y = t * *(real *)(self + 0xa8) + axis.j * angle * *(real *)(self + 0x8c);
                *(uint16_t *)(self + 0xe0) = *(uint16_t *)(self + 0xe0) -
                    *(int16_t *)((uint8_t *)game_time + 0x10);
                out->position.z = t * *(real *)(self + 0xac) + axis.k * angle * *(real *)(self + 0x90); // UNSURE
                *out = *base;
            }
        }
    }

write_scale_and_check_impulse:
    if (0 < *(int16_t *)((uint8_t *)&globals->players[local_player_index] + 0xe2) ||
        (*(uint32_t *)((uint8_t *)&globals->players[local_player_index] + 0xe8) & 4) != 0) {
        uint8_t *self = (uint8_t *)&globals->players[local_player_index];
        real_matrix4x3 second = *k_render_identity_matrix_ptr;
        real t;

        if ((*(uint32_t *)(self + 0xe8) & 4) == 0) {
            t = transition_function_evaluate(0, *(real *)(self + 0xd8)); // player_camera_shake.intensity
        } else {
            t = 1.0f;
        }

        {
            int16_t ticks = *(int16_t *)(self + 0xe2);
            real wobble = periodic_function_evaluate(0,
                (*(real *)(self + 0x9c) - (real)ticks) / *(real *)(self + 0xac));
            real weighted = ((1.0f - t) + wobble * t) * t; // UNSURE, see file header
            real translate_magnitude = weighted * *(real *)(self + 0xa0);
            real rotate_magnitude = weighted * *(real *)(self + 0xa4);

            translate_magnitude = (translate_magnitude < 0.0f) ? 0.0f : translate_magnitude;
            rotate_magnitude = (rotate_magnitude < 0.0f) ? 0.0f : rotate_magnitude;

            *(uint32_t *)(self + 0xe8) = *(uint32_t *)(self + 0xe8) & ~(uint32_t)4;
            player_effect_random_shake_offset(&second,
                translate_magnitude + *(real *)(self + 0xd4), rotate_magnitude + *(real *)(self + 0xd8));

            *(int16_t *)(self + 0xdc) = *(int16_t *)(self + 0xdc) +
                *(int16_t *)((uint8_t *)game_time + 0x10);
            if (*(int16_t *)(self + 0xdc) > 0) {
                *(int16_t *)(self + 0xdc) = 0;
                *(real *)(self + 0xcc) = 0.0f;
                *(real *)(self + 0xd0) = 0.0f;
                *(real *)(self + 0xd4) = 0.0f;
                *(real *)(self + 0xd8) = 0.0f;
            }
        }

        player_effect_random_shake_offset(&second,
            *(real *)(self + 0xd4), *(real *)(self + 0xd8));
        *(int16_t *)(self + 0xe2) = *(int16_t *)(self + 0xe2) -
            *(int16_t *)((uint8_t *)game_time + 0x10);
        matrix4x3_multiply_procedure(out, &second, out);
    }
}

#if 0
Original Ghidra decompilation (0x457390):

void FUN_00457390(undefined4 *param_1)

{
  float fVar1;
  float fVar2;
  short sVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  uint uVar7;
  uint uVar8;
  undefined4 *puVar9;
  short in_CX;
  int iVar10;
  int iVar11;
  int extraout_EDX;
  undefined4 extraout_EDX_00;
  float *pfVar12;
  undefined4 *puVar13;
  float10 fVar14;
  float10 fVar15;
  undefined4 local_54 [10];
  float local_2c;
  float local_28;
  float local_24;
  float local_c;
  float local_8;

  iVar11 = DAT_006f1884;
  if (in_CX == -1) {
    return;
  }
  if ((*(byte *)(DAT_006f1884 + 0x120) & 1) != 0) {
    local_8 = *(float *)(DAT_006f1884 + 0x118);
    puVar9 = (undefined4 *)PTR_DAT_0069673c;
    puVar13 = param_1;
    for (iVar10 = 0xd; iVar10 != 0; iVar10 = iVar10 + -1) {
      *puVar13 = *puVar9;
      puVar9 = puVar9 + 1;
      puVar13 = puVar13 + 1;
    }
    sVar3 = *(short *)(iVar11 + 0x11c);
    if (sVar3 < 1) {
      if ((*(uint *)(iVar11 + 0x120) & 2) != 0) {
        *(uint *)(iVar11 + 0x120) = *(uint *)(iVar11 + 0x120) & 0xfffffffe;
      }
    }
    else {
      if ((*(byte *)(iVar11 + 0x120) & 2) == 0) {
        fVar1 = 1.0 - (float)(int)sVar3 / (float)(int)*(short *)(iVar11 + 0x11e);
      }
      else {
        fVar1 = (float)(int)sVar3 / (float)(int)*(short *)(iVar11 + 0x11e);
      }
      local_8 = fVar1 * local_8;
      *(short *)(iVar11 + 0x11c) = sVar3 - *(short *)(DAT_006f1d6c + 0x10);
    }
    if ((*(byte *)(iVar11 + 0x120) & 1) == 0) {
      return;
    }
    if (0.0 <= local_8) {
      if (1.0 < local_8) {
        local_8 = 1.0;
      }
    }
    else {
      local_8 = 0.0;
    }
    uVar7 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    uVar8 = uVar7 * 0x19660d + 0x3c6ef35f;
    fVar1 = (float)(uVar7 >> 0x10) * 1.5259022e-05;
    DAT_00719cd4 = uVar8 * 0x19660d + 0x3c6ef35f;
    local_c = (float)(DAT_00719cd4 >> 0x10);
    fVar2 = (float)(uVar8 >> 0x10) * 1.5259022e-05;
    matrix4x3_from_euler_angles
              ((((float)(int)local_c * 1.5259022e-05 + (float)(int)local_c * 1.5259022e-05) - 1.0) *
               *(float *)(iVar11 + 0x10c) * local_8,
               ((fVar2 + fVar2) - 1.0) * *(float *)(iVar11 + 0x110) * local_8,
               ((fVar1 + fVar1) - 1.0) * *(float *)(iVar11 + 0x114) * local_8);
    uVar7 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    uVar8 = uVar7 * 0x19660d + 0x3c6ef35f;
    fVar4 = (float)(uVar7 >> 0x10) * 1.5259022e-05;
    DAT_00719cd4 = uVar8 * 0x19660d + 0x3c6ef35f;
    fVar5 = (float)(uVar8 >> 0x10) * 1.5259022e-05;
    fVar1 = *(float *)(extraout_EDX + 0x108);
    fVar2 = *(float *)(extraout_EDX + 0x100);
    fVar6 = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
    param_1[10] = ((fVar6 + fVar6) - 1.0) * *(float *)(extraout_EDX + 0x104) * local_8;
    param_1[0xb] = ((fVar5 + fVar5) - 1.0) * *(float *)(extraout_EDX + 0x100) * local_8;
    param_1[0xc] = ((fVar4 + fVar4) - 1.0) * fVar1 * local_8;
    return;
  }
  pfVar12 = (float *)(in_CX * 0xec + DAT_006f1884);
  if (*(short *)(in_CX * 0xec + 0xe0 + DAT_006f1884) < 1) {
    puVar9 = (undefined4 *)PTR_DAT_0069673c;
    if (((uint)pfVar12[0x3a] & 2) == 0) goto LAB_004576ea;
LAB_00457618:
    local_8 = 1.0;
  }
  else {
    if (((uint)pfVar12[0x3a] & 2) != 0) goto LAB_00457618;
    local_c = pfVar12[0x1a];
    fVar14 = (float10)transition_function_evaluate();
    local_8 = (float)(fVar14 * (float10)local_c);
  }
  *(byte *)(pfVar12 + 0x3a) = *(byte *)(pfVar12 + 0x3a) & 0xfd;
  vector3d_cross_product();
  fVar14 = (float10)fcos((float10)local_8 * (float10)pfVar12[0x16]);
  fVar15 = (float10)fsin((float10)local_8 * (float10)pfVar12[0x16]);
  puVar9 = (undefined4 *)matrix4x3_from_axis_angle((float)fVar15,(float)fVar14);
  fVar1 = local_8 * pfVar12[0x17];
  local_2c = local_8 * pfVar12[3] + fVar1 * *pfVar12;
  local_28 = local_8 * pfVar12[4] + fVar1 * pfVar12[1];
  *(short *)(pfVar12 + 0x38) = *(short *)(pfVar12 + 0x38) - *(short *)(DAT_006f1d6c + 0x10);
  local_24 = local_8 * pfVar12[5] + fVar1 * pfVar12[2];
LAB_004576ea:
  puVar13 = param_1;
  for (iVar11 = 0xd; iVar11 != 0; iVar11 = iVar11 + -1) {
    *puVar13 = *puVar9;
    puVar9 = puVar9 + 1;
    puVar13 = puVar13 + 1;
  }
  if ((0 < *(short *)((int)pfVar12 + 0xe2)) || (((uint)pfVar12[0x3a] & 4) != 0)) {
    puVar9 = (undefined4 *)PTR_DAT_0069673c;
    puVar13 = local_54;
    for (iVar11 = 0xd; iVar11 != 0; iVar11 = iVar11 + -1) {
      *puVar13 = *puVar9;
      puVar9 = puVar9 + 1;
      puVar13 = puVar13 + 1;
    }
    if (((uint)pfVar12[0x3a] & 4) == 0) {
      local_c = pfVar12[0x2b];
      fVar14 = (float10)transition_function_evaluate();
      local_8 = (float)(fVar14 * (float10)local_c);
    }
    else {
      local_c = 1.0;
    }
    local_8 = (float)(int)*(short *)((int)pfVar12 + 0xe2);
    fVar14 = (float10)periodic_function_evaluate
                                ((double)((pfVar12[0x21] - (float)(int)local_8) / pfVar12[0x29]));
    fVar15 = (((float10)1.0 - (float10)pfVar12[0x2a]) + fVar14 * (float10)pfVar12[0x2a]) *
             (float10)local_c;
    fVar14 = fVar15 * (float10)pfVar12[0x23];
    if (fVar14 <= (float10)0.0) {
      local_c = 0.0;
    }
    else {
      local_c = (float)fVar14;
    }
    fVar15 = fVar15 * (float10)pfVar12[0x24];
    if (fVar15 <= (float10)0.0) {
      local_8 = 0.0;
    }
    else {
      local_8 = (float)fVar15;
    }
    *(byte *)(pfVar12 + 0x3a) = *(byte *)(pfVar12 + 0x3a) & 0xfb;
    FUN_00457280(local_c + pfVar12[0x35],local_8 + pfVar12[0x36]);
    iVar11 = DAT_006f1d6c;
    *(short *)(pfVar12 + 0x37) = *(short *)(pfVar12 + 0x37) + *(short *)(DAT_006f1d6c + 0x10);
    if (0 < *(short *)(pfVar12 + 0x37)) {
      *(undefined2 *)(pfVar12 + 0x37) = 0;
      pfVar12[0x33] = 0.0;
      pfVar12[0x34] = 0.0;
      pfVar12[0x35] = 0.0;
      pfVar12[0x36] = 0.0;
    }
    FUN_00457280(local_c,local_8);
    *(short *)((int)pfVar12 + 0xe2) = *(short *)((int)pfVar12 + 0xe2) - *(short *)(iVar11 + 0x10);
    (*(code *)PTR_matrix4x3_multiply_00696664)(param_1,extraout_EDX_00,param_1);
  }
  return;
}
#endif
