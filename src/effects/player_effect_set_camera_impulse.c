// player_effect_set_camera_impulse  (Ghidra: FUN_004579b0, still unnamed there; named directly
//   by out/phase4/effects_types_notes.md: "player_effect_set_camera_impulse 0x4579b0")
// address 0x4579b0, size 925 bytes
// name confidence: 0.5   rewrite confidence: 0.15 (VERY LOW -- see UNSURE)
// evidence: types/effects.h player_effect.impulse (player_camera_impulse, +0x50), impulse_ticks
//   (+0xe0), flags (_player_effect_camera_impulse_bit); player_camera_impulse (duration +0x00,
//   magnitude_minimum/_maximum +0x10/+0x14, intensity +0x18); this module's
//   player_effect_set_camera_shake.c for the identical "replace only if higher priority"
//   blend-and-compare idiom this function opens with; src/math/vector2d_angle_between.c and
//   vector3d_rotate_about_axis's established signatures.
// register convention: player_effect self in EBX (unaff_EBX, never assigned anywhere in this
//   function's own decompile -- reconstructed exactly as every sibling player_effect_set_*
//   function in this module receives it); local player index, tag descriptor pointer, direction
//   vector pointer, intensity falloff and duration scale are Ghidra's own recognized stack
//   parameters, in that order.
//   // blam-cc: unaff_EBX -> self, stack -> (local_player_index, descriptor, direction,
//   //   intensity_falloff, duration_scale)
// UNSURE (heavily): game_engine_update_local_player_look and player_compute_view_forward_vector (both outside this batch) are called with
//   almost every argument elided by Ghidra; player_compute_view_forward_vector's call is kept as a bare, argument-less
//   placeholder (its established use elsewhere -- src/game/game_engine_compute_local_player_
//   look_vector.c -- takes a unit handle and an out yaw/pitch pointer, neither of which is
//   recoverable here) and game_engine_update_local_player_look's two arguments are reconstructed from the only visible
//   arithmetic (a signed triple-product-shaped expression against a foreign "camera basis"
//   pointer, `k_camera_axis_table` at 0x00696720). `vector2d_angle_between` and both
//   `vector3d_normalize_with_length` calls are likewise argument-elided; reconstructed from the
//   two 2D unit vectors the surrounding magnitude checks operate on. The player_camera_impulse
//   fields at +0x1c/+0x20/+0x24/+0x28/+0x2c (written from the 13-dword descriptor copy but not
//   otherwise read here) are accessed as raw floats since only duration/intensity are named.
//   src/effects/player_effect_mark_damage_direction.c's own call to this function passes only
//   5 arguments and no separate `self`; its EBX must therefore already hold the same
//   player_effect pointer that function computed for its own use, which is how this rewrite's
//   signature was fixed to match this function's real (5 stack argument) decompile.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern int32_t *game_control_globals; // 0x006b145c, UNSURE: foreign module (player look/aim
                                    // globals); stride 0x40, +0x1c pitch, +0x20 yaw
extern random_seed effect_random_seed; // 0x00719cd4
extern double fabs(double x); // ABS is a single x87 FABS instruction
extern const real *k_camera_axis_table; // 0x00696720, UNSURE: foreign module (render globals),
                                    // 3 floats read as a basis for game_engine_update_local_player_look's expression

extern double cos(double x);
extern double sin(double x);
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *a, real_vector3d *b); // 0x4052c0
extern real vector2d_angle_between(real_vector2d *a, real_vector2d *b); // 0x4cd480
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle,
                                        real cos_angle); // 0x4cd820
extern void game_engine_update_local_player_look(real angle_a, real angle_b); // 0x472160, UNSURE signature, see file header
extern void player_compute_view_forward_vector(void); // 0x473d70, UNSURE signature (elsewhere: unit, out yaw/pitch),
                                    // see file header

// Replaces a local player's active camera impulse with a new one built from `descriptor` and
// `direction` when the new one out-prioritizes (or has run longer than) the current one, then
// always feeds a second, independent computation into game_engine_update_local_player_look.
void player_effect_set_camera_impulse(player_effect *self, int16_t local_player_index,
    real *descriptor, real *direction, real intensity_falloff, real duration_scale)
    // blam-cc: unaff_EBX, stack, stack, stack, stack, stack
{
    real blended = (1.0f - descriptor[6]) * intensity_falloff + descriptor[6];
    real *impulse = (real *)&self->impulse; // player_camera_impulse, +0x50

    if ((real)self->impulse_ticks < impulse[0] || self->impulse.intensity < blended ||
        (self->impulse.intensity <= blended &&
         (real)self->impulse_ticks < duration_scale * 30.0f * descriptor[0])) {
        real dir_x = direction[0], dir_y = direction[1];
        real pitch = *(real *)((uint8_t *)game_control_globals + local_player_index * 0x40 + 0x1c);
        real yaw = *(real *)((uint8_t *)game_control_globals + local_player_index * 0x40 + 0x20);
        real cos_yaw = (real)cos((double)yaw);
        real look_z = 0.0f;
        real look_x = (real)cos((double)pitch) * cos_yaw;
        real look_y = (real)sin((double)pitch) * cos_yaw;

        vector3d_normalize_with_length((real_vector3d *)direction);
        vector3d_normalize_with_length((real_vector3d *)&look_x); // UNSURE: 2D vector normalized
                                    // through the 3D routine with z forced to 0, matching the raw
                                    // code's own length checks below

        if ((real)fabs((double)(dir_x * dir_x + dir_y * dir_y + 0.0f - 1.0f)) < 0.0001f &&
            (real)fabs((double)(look_x * look_x + look_y * look_y + 0.0f - 1.0f)) < 0.0001f) {
            real angle;
            int i;

            {
                real_vector2d a, b;
                a.i = dir_x; a.j = dir_y;
                b.i = look_x; b.j = look_y;
                angle = vector2d_angle_between(&a, &b);
            }

            for (i = 0; i < 13; i++) {
                impulse[i] = descriptor[i];
            }
            impulse[0] = duration_scale * 30.0f * impulse[0];
            self->impulse.intensity = blended;
            self->impulse_ticks = (int16_t)impulse[0];

            self->impulse_direction.k = 0.0f;
            self->impulse_direction.i = (real)cos((double)angle);
            self->impulse_direction.j = (real)sin((double)angle);

            {
                real random_magnitude, random_angle;
                real_vector3d axis;

                effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                // Unsigned: the raw code is (float)(seed >> 0x10) * 1.5259022e-05, so the
                // fraction is in [0, 1). An earlier draft cast the shifted word to int16_t.
                random_magnitude = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
                    (impulse[6] - impulse[5]) + impulse[5]; // magnitude_maximum(0x18)-minimum(0x14)
                effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                random_angle = (real)(effect_random_seed >> k_random_value_shift) *
                    1.5259022e-05f * 6.2831855f;

                vector3d_cross_product(&axis, &self->impulse_direction, (real_vector3d *)&look_x); // UNSURE
                vector3d_normalize_with_length(&axis);
                vector3d_rotate_about_axis(&self->impulse_rotation, &axis,
                    (real)sin((double)random_angle), (real)cos((double)random_angle));

                self->impulse_rotation.i = random_magnitude * self->impulse_rotation.i;
                self->impulse_rotation.j = random_magnitude * self->impulse_rotation.j;
                self->impulse_rotation.k = random_magnitude * self->impulse_rotation.k;
            }

            self->flags |= _player_effect_camera_impulse_bit;
        }
    }

    {
        real blended_b = (1.0f - descriptor[9]) * intensity_falloff + descriptor[9];

        player_compute_view_forward_vector(); // UNSURE, see file header
        game_engine_update_local_player_look(blended_b, blended); // UNSURE argument order/roles, see file header
    }
}

#if 0
Original Ghidra decompilation (0x4579b0):

void FUN_004579b0(short param_1,float *param_2,float *param_3,float param_4,float param_5)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  undefined *puVar6;
  undefined2 uVar7;
  uint uVar8;
  int iVar9;
  float *unaff_EBX;
  float *pfVar10;
  float *pfVar11;
  float10 fVar12;
  float10 fVar13;
  unkbyte10 extraout_ST0;
  float local_c;
  float local_8;
  float local_4;

  puVar6 = PTR_DAT_00696720;
  fVar5 = (1.0 - param_2[6]) * param_4 + param_2[6];
  if ((((float)(int)*(short *)(unaff_EBX + 0x38) < unaff_EBX[0x14]) || (unaff_EBX[0x1a] < fVar5)) ||
     ((unaff_EBX[0x1a] <= fVar5 &&
      ((float)(int)*(short *)(unaff_EBX + 0x38) < param_5 * 30.0 * *param_2)))) {
    fVar2 = *param_3;
    fVar3 = param_3[1];
    vector3d_normalize_with_length();
    fVar4 = *(float *)(param_1 * 0x40 + 0x1c + DAT_006b145c);
    fVar12 = (float10)fcos((float10)*(float *)(param_1 * 0x40 + DAT_006b145c + 0x20));
    local_4 = 0.0;
    fVar13 = (float10)fcos((float10)fVar4);
    local_c = (float)(fVar13 * fVar12);
    fVar13 = (float10)fsin((float10)fVar4);
    local_8 = (float)(fVar13 * fVar12);
    vector3d_normalize_with_length();
    if ((ABS((fVar2 * fVar2 + fVar3 * fVar3 + 0.0) - 1.0) < 0.0001) &&
       (ABS((local_c * local_c + local_8 * local_8 + 0.0) - 1.0) < 0.0001)) {
      vector2d_angle_between();
      pfVar1 = unaff_EBX + 0x14;
      pfVar10 = param_2;
      pfVar11 = pfVar1;
      for (iVar9 = 0xd; iVar9 != 0; iVar9 = iVar9 + -1) {
        *pfVar11 = *pfVar10;
        pfVar10 = pfVar10 + 1;
        pfVar11 = pfVar11 + 1;
      }
      *pfVar1 = param_5 * 30.0 * *pfVar1;
      unaff_EBX[0x1a] = fVar5;
      uVar7 = __ftol();
      *(undefined2 *)(unaff_EBX + 0x38) = uVar7;
      unaff_EBX[2] = 0.0;
      fVar12 = (float10)fcos(extraout_ST0);
      *unaff_EBX = (float)fVar12;
      fVar12 = (float10)fsin(extraout_ST0);
      unaff_EBX[1] = (float)fVar12;
      uVar8 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      DAT_00719cd4 = uVar8 * 0x19660d + 0x3c6ef35f;
      fVar5 = (float)(uVar8 >> 0x10) * 1.5259022e-05 * (unaff_EBX[0x19] - unaff_EBX[0x18]) +
              unaff_EBX[0x18];
      fVar2 = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * 6.2831855;
      vector3d_cross_product();
      vector3d_normalize_with_length();
      fVar12 = (float10)fcos((float10)fVar2);
      fVar13 = (float10)fsin((float10)fVar2);
      vector3d_rotate_about_axis((float)fVar13,(float)fVar12);
      unaff_EBX[3] = fVar5 * unaff_EBX[3];
      unaff_EBX[4] = fVar5 * unaff_EBX[4];
      unaff_EBX[5] = fVar5 * unaff_EBX[5];
      *(byte *)(unaff_EBX + 0x3a) = *(byte *)(unaff_EBX + 0x3a) | 2;
    }
  }
  fVar5 = (1.0 - param_2[9]) * param_4 + param_2[9];
  FUN_00473d70();
  FUN_00472160(((local_4 * *(float *)(puVar6 + 4) - local_8 * *(float *)(puVar6 + 8)) * *param_3 +
               (local_8 * *(float *)puVar6 - local_c * *(float *)(puVar6 + 4)) * param_3[2] +
               (local_c * *(float *)(puVar6 + 8) - local_4 * *(float *)puVar6) * param_3[1]) *
               param_2[8] * fVar5,
               (local_4 * param_3[2] + local_c * *param_3 + local_8 * param_3[1]) * param_2[8] *
               fVar5);
  return;
}
#endif
