// player_effect_set_camera_impulse  (Ghidra: FUN_004579b0, still unnamed there; named directly
//   by out/phase4/effects_types_notes.md: "player_effect_set_camera_impulse 0x4579b0")
// address 0x4579b0, size 925 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (REWRITTEN from objdump 0x4579b0..0x457d4c)
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
//   pointer, `global_up3d_pointer` at 0x00696720). `vector2d_angle_between` and both
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
#include "units.h"
#include "game.h"

extern int32_t *player_control_globals_ptr; // 0x006b145c, UNSURE: foreign module (player look/aim
                                    // globals); stride 0x40, +0x1c pitch, +0x20 yaw
extern random_seed effect_random_seed; // 0x00719cd4
extern double fabs(double x); // ABS is a single x87 FABS instruction
extern const real *global_up3d_pointer; // 0x00696720, UNSURE: foreign module (render globals),
                                    // 3 floats read as a basis for game_engine_update_local_player_look's expression

extern double cos(double x);
extern double sin(double x);
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *a, real_vector3d *b); // 0x4052c0
extern real vector2d_angle_between(real_vector2d *a, real_vector2d *b); // 0x4cd480
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle,
                                        real cos_angle); // 0x4cd820
extern void game_engine_update_local_player_look(int16_t local_player_index, real yaw_delta, real pitch_delta); // 0x472160, AX, stack
extern void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch, real_vector3d *out_forward); // 0x473d70, EAX, ECX, ESI
extern player_globals *local_player_globals; // 0x0087a478

// Replaces a local player's active camera impulse with a new one built from `descriptor` and
// `direction` when the new one out-prioritizes (or has run longer than) the current one, then
// always feeds a second, independent computation into game_engine_update_local_player_look.
// REWRITTEN from objdump. Raw offsets into player_effect: +0x00 impulse direction, +0x0c impulse rotation, +0x50 the
//   13-float impulse copied from the descriptor (+0x50 duration in ticks, +0x60/+0x64 magnitude min/max, +0x68
//   intensity), +0xe0 ticks, +0xe8 flags.
//   A new impulse replaces the running one when it outlasts it, is stronger, or is as strong and longer. Its
//   direction is the angle between the (x, y, 0) damage direction and the player's (pitch, yaw) look vector, both
//   normalized copies (the caller's vector is left alone). The rotation axis is up x direction, rotated about the
//   direction by a random angle and scaled by a random magnitude in [min, max].
//   In all cases the look is then nudged: game_engine_update_local_player_look(player, (left . direction) * s,
//   (forward . direction) * s), with s = descriptor[8] * blend(descriptor[9]) and forward/left from
//   player_compute_view_forward_vector. The draft normalized the caller's direction in place, used the wrong
//   magnitude slots and cross/rotate operands, and called both tail helpers without arguments.
void player_effect_set_camera_impulse(player_effect *self, int16_t local_player_index,
    real *descriptor, real *direction, real intensity_falloff, real duration_scale)
    // blam-cc: EBX -> self, stack -> local_player_index, descriptor, direction, intensity_falloff, duration_scale
{
    uint8_t *fx = (uint8_t *)self;
    real *impulse = (real *)(fx + 0x50);
    real_vector3d *impulse_direction = (real_vector3d *)(fx + 0x00);
    real_vector3d *impulse_rotation = (real_vector3d *)(fx + 0x0c);
    real_vector3d *up = (real_vector3d *)global_up3d_pointer; // [0x696720]: the global up vector
    real duration_ticks = duration_scale * 30.0f;
    real ticks = (real)((struct player_effect *)fx)->impulse_ticks;
    real blended = (1.0f - descriptor[6]) * intensity_falloff + descriptor[6];
    real *look_globals = (real *)((uint8_t *)player_control_globals_ptr + local_player_index * 0x40);

    if (impulse[0] > ticks || blended > ((struct player_effect *)fx)->impulse.intensity ||
        (!(blended < ((struct player_effect *)fx)->impulse.intensity) && duration_ticks * descriptor[0] > ticks)) {
        real_vector3d flat_direction;
        real_vector3d look;
        real a = look_globals[7]; // +0x1c
        real b = look_globals[8]; // +0x20

        flat_direction.i = direction[0];
        flat_direction.j = direction[1];
        flat_direction.k = 0.0f;
        vector3d_normalize_with_length(&flat_direction);
        look.i = (real)cos((double)a) * (real)cos((double)b);
        look.j = (real)sin((double)a) * (real)cos((double)b);
        look.k = 0.0f;
        vector3d_normalize_with_length(&look);

        if ((real)fabs((double)(flat_direction.i * flat_direction.i + flat_direction.j * flat_direction.j +
                                flat_direction.k * flat_direction.k - 1.0f)) < 9.9999997e-05 &&
            (real)fabs((double)(look.i * look.i + look.j * look.j + look.k * look.k - 1.0f)) < 9.9999997e-05) {
            real angle = vector2d_angle_between((real_vector2d *)&flat_direction, (real_vector2d *)&look);
            real magnitude;
            real random_angle;
            int i;

            for (i = 0; i < 13; i++) {
                impulse[i] = descriptor[i];
            }
            impulse[0] = duration_ticks * impulse[0];
            ((struct player_effect *)fx)->impulse.intensity = blended;
            ((struct player_effect *)fx)->impulse_ticks = (int16_t)(int32_t)impulse[0];

            impulse_direction->k = 0.0f;
            impulse_direction->i = (real)cos((double)angle);
            impulse_direction->j = (real)sin((double)angle);

            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            magnitude = (real)(int32_t)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
                (((struct player_effect *)fx)->impulse.magnitude_maximum - ((struct player_effect *)fx)->impulse.magnitude_minimum) + ((struct player_effect *)fx)->impulse.magnitude_minimum;
            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            random_angle = (real)(int32_t)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * 6.2831855f;

            vector3d_cross_product(impulse_rotation, up, impulse_direction);
            vector3d_normalize_with_length(impulse_rotation);
            vector3d_rotate_about_axis(impulse_rotation, impulse_direction, (real)sin((double)random_angle),
                (real)cos((double)random_angle));
            impulse_rotation->i = magnitude * impulse_rotation->i;
            impulse_rotation->j = magnitude * impulse_rotation->j;
            impulse_rotation->k = magnitude * impulse_rotation->k;
            *(uint8_t *)&((struct player_effect *)fx)->flags |= 2;
        }
    }

    {
        real blended_b = (1.0f - descriptor[9]) * intensity_falloff + descriptor[9];
        datum_index player_handle = (local_player_index != -1 && local_player_index < 1) ?
            *(datum_index *)&local_player_globals->local_players[local_player_index] : (datum_index)k_datum_index_none;
        real_vector3d forward;
        real_vector3d left;
        real yaw_delta;
        real pitch_delta;

        player_compute_view_forward_vector(player_handle, &look_globals[7], &forward);
        left.i = forward.k * up->j - forward.j * up->k;
        left.j = forward.i * up->k - forward.k * up->i;
        left.k = forward.j * up->i - forward.i * up->j;
        yaw_delta = (left.j * direction[1] + left.k * direction[2] + left.i * direction[0]) * descriptor[8] * blended_b;
        pitch_delta = (forward.j * direction[1] + forward.i * direction[0] + forward.k * direction[2]) * descriptor[8] *
            blended_b;
        game_engine_update_local_player_look(local_player_index, yaw_delta, pitch_delta);
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
