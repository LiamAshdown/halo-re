// player_effect_build_camera_shake_matrix  (Ghidra: FUN_00457390, still unnamed there; named
//   directly by types/effects.h: "player_effect_build_camera_shake_matrix 0x457390 does the same
//   [as player_effect_build_screen_flash] for the shake")
// address 0x457390, size 1295 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (REWRITTEN from objdump 0x457390..0x45789e) (LOW-rigor best-effort pass, matching
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
#include "game.h"
#include "units.h"

extern player_effect_globals *player_effect_globals_pointer; // 0x006f1884
extern game_time_globals *game_time; // 0x006f1d6c
extern real_matrix4x3 *k_render_identity_matrix_ptr;           // 0x0069673c, UNSURE identity
                                    // (see src/game's own "render_ptr_9673c" for this address)
extern random_seed effect_random_seed;                         // 0x00719cd4
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b,
    real_matrix4x3 *out); // 0x00696664, math module

extern real_vector3d *global_up3d_pointer; // 0x00696720
extern double cos(double x);
extern double sin(double x);
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *a, real_vector3d *b); // 0x4052c0
extern void matrix4x3_from_axis_angle(real_matrix4x3 *out, real_vector3d *axis, real sin_angle,
                                       real cos_angle); // 0x4cb880
extern void matrix4x3_from_euler_angles(real_matrix4x3 *out, real yaw, real pitch, real roll); // 0x4cba10
extern real transition_function_evaluate(int16_t type, real phase); // 0x4ccac0, CX type, stack phase
extern real periodic_function_evaluate(int16_t type, double phase); // 0x4cc9b0, AX type, stack phase (a double)
extern void player_effect_random_shake_offset(real_matrix4x3 *out, real magnitude, real angle); // 0x457280,
                                    // this module

static real shake_random_signed(void)
{
    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    return (real)(int32_t)(effect_random_seed >> 16) * 1.5259022e-05f * 2.0f - 1.0f;
}

// REWRITTEN from objdump. Scripted shake (globals +0x120 bit 0): identity, then while ticks (+0x11c) remain,
//   t = intensity * (bit 1 ? ticks / duration : 1 - ticks / duration) and ticks count down; once they run out
//   with bit 1 set the shake ends (bit 0 cleared). The rotation comes from euler angles with random +-1 draws
//   times the translation amplitudes (+0x10c..+0x114) and t, and the position from three more draws times
//   +0x104 / +0x100 / +0x108. Per player (stride 0xec): while +0xe0 ticks remain (or bit 1 of +0xe8 forces full),
//   t = transition(+0x54, 1 - (+0x50 - ticks) / +0x50) * +0x68, the matrix rotates about cross(+0x00, up) by
//   t * +0x58 and is placed at t * +0x0c + t * +0x5c * (+0x00). Otherwise it's identity. While +0xe2 ticks remain
//   (or bit 2 forces full), a second matrix gets t2 = transition(+0x88, 1 - (+0x84 - ticks) / +0x84) * +0xac,
//   w = (periodic(+0xa0, (+0x84 - ticks) / +0xa4) * +0xa8 + 1 - +0xa8) * t2, and two random shake offsets:
//   (w * +0x8c + +0xd4, w * +0x90 + +0xd8), then (w * +0x8c, w * +0x90), both clamped at 0. The +0xdc timer
//   clears +0xcc..+0xd8 once it passes 0, and out = out * second. The draft cleared the wrong scripted bit, used
//   transition/periodic type 0 with the wrong phases and fields, and overwrote the placed position.
void player_effect_build_camera_shake_matrix(real_matrix4x3 *out, int16_t local_player_index)
    // blam-cc: stack -> out, CX -> local_player_index
{
    player_effect_globals *globals = player_effect_globals_pointer;
    uint8_t *g = (uint8_t *)globals;
    int16_t dt;

    if (local_player_index == -1) {
        return;
    }
    if ((*(uint8_t *)&((struct player_effect_globals *)g)->scripted_shake_flags & 1) != 0) {
        real t = ((struct player_effect_globals *)g)->scripted_shake_intensity;
        int16_t ticks = ((struct player_effect_globals *)g)->scripted_shake_ticks;

        *out = *k_render_identity_matrix_ptr;
        if (ticks > 0) {
            real fraction = (real)(int32_t)ticks / (real)(int32_t)((struct player_effect_globals *)g)->scripted_shake_duration;

            if ((*(uint8_t *)&((struct player_effect_globals *)g)->scripted_shake_flags & 2) == 0) {
                fraction = 1.0f - fraction;
            }
            t = fraction * t;
            ((struct player_effect_globals *)g)->scripted_shake_ticks = (int16_t)(ticks - game_time->ticks_this_frame);
        } else if ((((struct player_effect_globals *)g)->scripted_shake_flags & 2) != 0) {
            ((struct player_effect_globals *)g)->scripted_shake_flags &= ~(uint32_t)1;
        }
        if ((*(uint8_t *)&((struct player_effect_globals *)g)->scripted_shake_flags & 1) == 0) {
            return;
        }
        if (!(t >= 0.0f)) {
            t = 0.0f;
        } else if (!(t <= 1.0f)) {
            t = 1.0f;
        }
        {
            real a1 = shake_random_signed();
            real a2 = shake_random_signed();
            real a3 = shake_random_signed();

            matrix4x3_from_euler_angles(out, a3 * *(real *)(g + 0x10c) * t, a2 * *(real *)(g + 0x110) * t,
                a1 * *(real *)(g + 0x114) * t);
        }
        {
            real b4 = shake_random_signed();
            real b5 = shake_random_signed();
            real b6 = shake_random_signed();

            out->position.x = b6 * *(real *)(g + 0x104) * t;
            out->position.y = b5 * *(real *)(g + 0x100) * t;
            out->position.z = b4 * *(real *)(g + 0x108) * t;
        }
        return;
    }

    {
        uint8_t *self = g + (int32_t)local_player_index * 0xec;
        int16_t ticks = *(int16_t *)(self + 0xe0);
        real t;

        if (ticks <= 0 && (self[0xe8] & 2) == 0) {
            *out = *k_render_identity_matrix_ptr;
        } else {
            real_vector3d axis;
            real angle;
            real k;
            real_matrix4x3 rotation;

            if ((self[0xe8] & 2) != 0) {
                t = 1.0f;
            } else {
                real duration = *(real *)(self + 0x50);

                t = transition_function_evaluate(*(int16_t *)(self + 0x54),
                    1.0f - (duration - (real)(int32_t)ticks) / duration) * *(real *)(self + 0x68);
            }
            self[0xe8] &= 0xfd;
            vector3d_cross_product(&axis, (real_vector3d *)self, global_up3d_pointer);
            angle = t * *(real *)(self + 0x58);
            matrix4x3_from_axis_angle(&rotation, &axis, (real)sin((double)angle), (real)cos((double)angle));
            k = t * *(real *)(self + 0x5c);
            rotation.position.x = t * *(real *)(self + 0x0c) + k * *(real *)(self + 0x00);
            rotation.position.y = t * *(real *)(self + 0x10) + k * *(real *)(self + 0x04);
            rotation.position.z = t * *(real *)(self + 0x14) + k * *(real *)(self + 0x08);
            *(int16_t *)(self + 0xe0) = (int16_t)(*(int16_t *)(self + 0xe0) - game_time->ticks_this_frame);
            *out = rotation;
        }

        ticks = *(int16_t *)(self + 0xe2);
        if (ticks > 0 || (self[0xe8] & 4) != 0) {
            real_matrix4x3 second = *k_render_identity_matrix_ptr;
            real t2;
            real w;
            real a;
            real b;

            if ((self[0xe8] & 4) != 0) {
                t2 = 1.0f;
            } else {
                real duration = *(real *)(self + 0x84);

                t2 = transition_function_evaluate((int16_t)*(uint16_t *)(self + 0x88),
                    1.0f - (duration - (real)(int32_t)ticks) / duration) * *(real *)(self + 0xac);
            }
            w = periodic_function_evaluate(*(int16_t *)(self + 0xa0),
                (double)((*(real *)(self + 0x84) - (real)(int32_t)*(int16_t *)(self + 0xe2)) / *(real *)(self + 0xa4)));
            w = (w * *(real *)(self + 0xa8) + (1.0f - *(real *)(self + 0xa8))) * t2;
            a = w * *(real *)(self + 0x8c);
            if (!(a > 0.0f)) {
                a = 0.0f;
            }
            b = w * *(real *)(self + 0x90);
            if (!(b > 0.0f)) {
                b = 0.0f;
            }
            self[0xe8] &= 0xfb;
            player_effect_random_shake_offset(&second, a + *(real *)(self + 0xd4), b + *(real *)(self + 0xd8));
            dt = game_time->ticks_this_frame;
            *(int16_t *)(self + 0xdc) = (int16_t)(*(int16_t *)(self + 0xdc) + dt);
            if (*(int16_t *)(self + 0xdc) > 0) {
                *(int16_t *)(self + 0xdc) = 0;
                *(real *)(self + 0xcc) = 0.0f;
                *(real *)(self + 0xd0) = 0.0f;
                *(real *)(self + 0xd4) = 0.0f;
                *(real *)(self + 0xd8) = 0.0f;
            }
            player_effect_random_shake_offset(&second, a, b);
            *(int16_t *)(self + 0xe2) = (int16_t)(*(int16_t *)(self + 0xe2) - dt);
            matrix4x3_multiply_procedure(out, &second, out);
        }
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
