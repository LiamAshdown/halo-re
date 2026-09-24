// game_engine_update_local_player_look  (Ghidra: FUN_00472160; renamed, no established name)
// address 0x472160, size 1346 bytes
// name confidence: 0.3   rewrite confidence: 0.15
// evidence: out/phase4/game_functions.md ("Updates a local player's look yaw/pitch each tick,
// clamping against any vehicle seat facing constraints and limiting the per-tick turn rate");
// modules.json ("Keep game, and keep it with 0x472020. ... resolves a player's unit index, seat
// index and the pointer to the seat's aiming/camera data"); types/game.h local_player_control
// (yaw +0x0c, pitch +0x10, unknown_26 +0x26, pitch_minimum +0x38, pitch_maximum +0x3c);
// chimera__spectate_fp_camera_position.c (this batch) for the {unit, seat_index, marker_offset,
// position} out-struct this function also fills (called here with the same struct, zero visible
// arguments, exactly like that function's own established convention).
// register convention: local-player index in AX (Ghidra's `in_AX`); yaw and pitch deltas are
// this function's own recognized stack parameters.
//   // blam-cc: AX -> local_player_index, stack -> yaw_delta, pitch_delta
// UNSURE: this is a low-confidence, mostly-literal transcription of a large function whose
// vehicle-seat aiming-constraint math (the seat pointer's +0x24/+0x88/+0xf0/+0xf4 fields, and the
// separate "local_7c" constraint-block fields +0x40/+0x44/+0x48/+0x68/+0x6c/+0x70) is not
// attested in any header this module owns. The two `(a < b) == (a == b)` / `(a < b) != (a == b)`
// FPU idioms are reduced per the established rule from game_engine_tick.c and
// angle_delta_wrapped.c (`a > b` and `a <= b` respectively).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "game.h"

extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern Globals *global_globals;                              // 0x00746fa0
extern data_array *object_headers;                            // 0x008603b0
extern tag_instance *tag_instances;                            // 0x0087bc14

// camera_basis_out now lives in types/game.h (folded there by the phase-4 review).

extern void chimera__spectate_fp_camera_position(camera_basis_out *out, int16_t local_player_index); // this batch, 0x472020
extern void value_step_toward_target(float *value, float target, float max_step); // this batch, 0x470d40
extern real vector3d_angle_between_4cd4f0(void); // 0x4cd4f0, not in this batch; UNSURE args (register-passed)
extern void object_get_node_local_transform(datum_index object_index, int32_t node_index, void *out_transform, int32_t unknown); // 0x4f6080, not in this batch; UNSURE args

extern double atan2(double y, double x); // x87 FPATAN
extern double cos(double x);
extern double sin(double x);
extern double sqrt(double x);            // x87 FSQRT

// blam-cc: AX -> local_player_index, stack -> yaw_delta, pitch_delta
// See the header UNSURE note.
void game_engine_update_local_player_look(int16_t local_player_index, real yaw_delta, real pitch_delta)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    // CORRECTED (phase 4 review): Globals + 0x114 is player_control.pointer, not
    // player_information (that is + 0x174); + 0x54 in it is look_autolevelling_scale.
    GlobalsPlayerControl *player_control =
        (GlobalsPlayerControl *)global_globals->player_control.pointer;
    real pitch_min = -1.4922565f;
    real pitch_max = 1.4922565f;
    camera_basis_out camera; // UNSURE: unit/seat_index/marker_offset/position, see header
    int32_t *seat_constraint = 0; // "local_7c" in Ghidra

    chimera__spectate_fp_camera_position(&camera, local_player_index);
    look->yaw = yaw_delta + look->yaw;

    // UNSURE: reading camera.unit/seat_index/marker_offset back out requires knowing
    // camera_basis_out's exact field order; modeled via raw offsets matching that struct's
    // documented layout (unit@0, seat_index@4, marker_offset@8).
    {
        int16_t *raw = (int16_t *)&camera;
        datum_index unit = *(datum_index *)&camera;
        int16_t seat_index = raw[2];
        int32_t *marker = *(int32_t **)((uint8_t *)&camera + 8);

        if (seat_index != -1 && marker != 0 && (marker[0x3c] != 0 || marker[0x3d] != 0)) {
            // marker+0xf0/+0xf4 as float offsets 0x3c/0x3d dwords
            real *markerf = (real *)marker;
            object *u = (object *)(*(void **)((uint8_t *)object_headers->data +
                (uint32_t)(uint16_t)unit * object_headers->size + 8));
            void *transform_out_scratch[15]; // Ghidra's local_6c[60 bytes]
            real tf_j, tf_i; // Ghidra's local_2c/local_30

            object_get_node_local_transform(unit, (int32_t)((uint8_t *)marker - (uint8_t *)0 + 0x24),
                transform_out_scratch, 1); // UNSURE: node index argument approximate
            tf_j = ((real *)transform_out_scratch)[0]; // UNSURE: local_2c/local_30 mapping
            tf_i = ((real *)transform_out_scratch)[1];

            {
                real base_angle = (real)atan2((double)tf_j, (double)tf_i);
                real a = base_angle + markerf[0x3c];
                real b = base_angle + markerf[0x3d];
                real delta = b - a;
                real forward_delta, back_delta, span;

                if (delta >= 3.1415927f) { delta -= 6.2831855f; }
                if (delta <= -3.1415927f) { delta += 6.2831855f; } // reduced idiom, see header

                forward_delta = b - look->yaw;
                if (forward_delta >= 3.1415927f) { forward_delta -= 6.2831855f; }
                if (forward_delta <= -3.1415927f) { forward_delta += 6.2831855f; }

                back_delta = look->yaw - a;
                if (back_delta >= 3.1415927f) { back_delta -= 6.2831855f; }
                if (back_delta <= -3.1415927f) { back_delta += 6.2831855f; }

                if (delta < 0.0f) { delta += 6.2831855f; }
                span = delta;

                if ((forward_delta < 0.0f || span <= forward_delta) &&
                    (back_delta < 0.0f || span <= back_delta)) {
                    if ((forward_delta < 0 ? -forward_delta : forward_delta) <=
                        (back_delta < 0 ? -back_delta : back_delta)) {
                        look->yaw = b;
                    } else {
                        look->yaw = a;
                    }
                }
            }
        }
    }

    if (look->yaw < 0.0f) {
        do { look->yaw += 6.2831855f; } while (look->yaw < 0.0f);
    }
    if (look->yaw > 6.2831855f) {
        do { look->yaw -= 6.2831855f; } while (look->yaw > 6.2831855f);
    }

    if (seat_constraint != 0) {
        // UNSURE: seat_constraint ("local_7c") is never actually resolved to non-NULL above in
        // this transcription (see the marker/camera UNSURE block); this branch is preserved for
        // fidelity to Ghidra's control flow but is effectively dead pending a full disassembly
        // pass of this function.
        object *u = (object *)(*(void **)((uint8_t *)object_headers->data +
            (uint32_t)(uint16_t)0 * object_headers->size + 8));
        real target_pitch = *(real *)((uint8_t *)seat_constraint + 0x40);

        if (*(real *)((uint8_t *)seat_constraint + 0x48) != 0.0f ||
            *(real *)((uint8_t *)seat_constraint + 0x44) != 0.0f) {
            pitch_min = *(real *)((uint8_t *)seat_constraint + 0x44);
            pitch_max = *(real *)((uint8_t *)seat_constraint + 0x48);

            if (*(real *)((uint8_t *)u + 0x88) > 0.2f) {
                real adjust = 1.5707964f - vector3d_angle_between_4cd4f0();

                pitch_min -= adjust;
                pitch_max -= adjust;
                target_pitch -= adjust;
            }
            if (pitch_min < -1.4922565f) { pitch_min = -1.4922565f; }
            else if (pitch_min > 1.4922565f) { pitch_min = 1.4922565f; }
            if (pitch_max < -1.4922565f) { pitch_max = -1.4922565f; }
            else if (pitch_max > 1.4922565f) { pitch_max = 1.4922565f; }
        }

        if (target_pitch != 0.0f || look->autolevelling_active != 0) {
            real angle_frac = (look->pitch - target_pitch < 0 ? -(look->pitch - target_pitch)
                : (look->pitch - target_pitch)) * 0.63661975f;
            real fx = *(real *)((uint8_t *)u + 0x68);
            real fy = *(real *)((uint8_t *)u + 0x6c);
            real fz = *(real *)((uint8_t *)u + 0x70);
            real step;

            if (target_pitch == 0.0f) {
                real len = (real)sqrt((double)(fx * fx + fy * fy + fz * fz));

                step = len * player_control->look_autolevelling_scale * angle_frac;
            } else {
                real len = (real)sqrt((double)(fy * fy + fz * fz + fx * fx));

                step = len * angle_frac * 0.08f;
            }
            value_step_toward_target(&look->pitch, target_pitch, step);
        }
    }

    pitch_min = pitch_min - look->pitch_minimum;
    if (pitch_min < -0.012271847f) { pitch_min = -0.012271847f; }
    else if (pitch_min > 0.012271847f) { pitch_min = 0.012271847f; }
    look->pitch_minimum = pitch_min + look->pitch_minimum;

    pitch_max = pitch_max - look->pitch_maximum;
    if (pitch_max < -0.012271847f) { pitch_max = -0.012271847f; }
    else if (pitch_max > 0.012271847f) { pitch_max = 0.012271847f; }
    look->pitch_maximum = pitch_max + look->pitch_maximum;

    pitch_delta = pitch_delta + look->pitch;
    look->pitch = pitch_delta;
    if (look->pitch_minimum <= pitch_delta) {
        if (pitch_delta <= look->pitch_maximum) {
            look->pitch = pitch_delta;
            return;
        }
        look->pitch = look->pitch_maximum;
        return;
    }
    look->pitch = look->pitch_minimum;
}

#if 0
Original Ghidra decompilation (0x472160), from tools/pack.py 0x472160:

void FUN_00472160(float param_1,float param_2)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  float fVar6;
  short in_AX;
  int iVar7;
  float10 fVar8;
  float local_a4;
  float local_a0;
  float local_9c;
  float local_98;
  float local_94;
  uint local_84;
  short local_80;
  int local_7c;
  undefined1 local_6c [60];
  float local_30;
  float local_2c;

  iVar1 = in_AX * 0x40 + 0x10 + DAT_006b145c;
  iVar5 = *(int *)(DAT_00746fa0 + 0x114);
  local_9c = -1.4922565;
  local_98 = 1.4922565;
  chimera__spectate_fp_camera_position();
  *(float *)(iVar1 + 0xc) = param_1 + *(float *)(iVar1 + 0xc);
  if ((local_80 != -1) &&
     ((iVar7 = local_80 * 0x11c +
               *(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                             (local_84 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                                DAT_0087bc14) + 0x2e8), *(float *)(iVar7 + 0xf0) != 0.0 ||
      (*(float *)(iVar7 + 0xf4) != 0.0)))) {
    object_get_node_local_transform(local_84,iVar7 + 0x24,local_6c,1);
    fVar8 = (float10)fpatan((float10)local_2c,(float10)local_30);
    fVar2 = (float)(fVar8 + (float10)*(float *)(iVar7 + 0xf0));
    fVar8 = fVar8 + (float10)*(float *)(iVar7 + 0xf4);
    fVar3 = (float)fVar8;
    fVar8 = fVar8 - (float10)fVar2;
    if ((float10)3.1415927 <= fVar8) {
      fVar8 = fVar8 - (float10)6.2831855;
    }
    if (fVar8 < (float10)-3.1415927 != (fVar8 == (float10)-3.1415927)) {
      fVar8 = fVar8 + (float10)6.2831855;
    }
    local_a4 = fVar3 - *(float *)(iVar1 + 0xc);
    if (3.1415927 <= local_a4) {
      local_a4 = local_a4 - 6.2831855;
    }
    if (local_a4 < -3.1415927 != (local_a4 == -3.1415927)) {
      local_a4 = local_a4 + 6.2831855;
    }
    local_a0 = *(float *)(iVar1 + 0xc) - fVar2;
    if (3.1415927 <= local_a0) {
      local_a0 = local_a0 - 6.2831855;
    }
    if (local_a0 < -3.1415927 != (local_a0 == -3.1415927)) {
      local_a0 = local_a0 + 6.2831855;
    }
    if (fVar8 < (float10)0.0) {
      fVar8 = fVar8 + (float10)6.2831855;
    }
    local_94 = (float)fVar8;
    if (((local_a4 < 0.0) || (local_94 <= local_a4)) && ((local_a0 < 0.0 || (local_94 <= local_a0)))
       ) {
      if (ABS(local_a4) <= ABS(local_a0)) {
        *(float *)(iVar1 + 0xc) = fVar3;
      }
      else {
        *(float *)(iVar1 + 0xc) = fVar2;
      }
    }
  }
  if (*(float *)(iVar1 + 0xc) < 0.0) {
    fVar2 = *(float *)(iVar1 + 0xc);
    do {
      fVar2 = fVar2 + 6.2831855;
    } while (fVar2 < 0.0);
    *(float *)(iVar1 + 0xc) = fVar2;
  }
  if (6.2831855 < *(float *)(iVar1 + 0xc)) {
    fVar2 = *(float *)(iVar1 + 0xc);
    do {
      fVar2 = fVar2 - 6.2831855;
    } while (6.2831855 < fVar2);
    *(float *)(iVar1 + 0xc) = fVar2;
  }
  if (local_7c != 0) {
    iVar7 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_84 & 0xffff) * 0xc);
    local_a4 = *(float *)(local_7c + 0x40);
    if ((*(float *)(local_7c + 0x48) != 0.0) || (*(float *)(local_7c + 0x44) != 0.0)) {
      local_9c = *(float *)(local_7c + 0x44);
      local_98 = *(float *)(local_7c + 0x48);
      if ((local_80 != -1) && (0.2 < *(float *)(iVar7 + 0x88))) {
        fcos((float10)0.0);
        fcos((float10)*(float *)(iVar1 + 0xc));
        fsin((float10)*(float *)(iVar1 + 0xc));
        fsin((float10)0.0);
        fVar8 = (float10)vector3d_angle_between_4cd4f0();
        fVar8 = (float10)1.5707964 - fVar8;
        local_9c = (float)((float10)local_9c - fVar8);
        local_98 = (float)((float10)local_98 - fVar8);
        local_a4 = (float)((float10)local_a4 - fVar8);
      }
      if (-1.4922565 <= local_9c) {
        if (1.4922565 < local_9c) {
          local_9c = 1.4922565;
        }
      }
      else {
        local_9c = -1.4922565;
      }
      if (-1.4922565 <= local_98) {
        if (1.4922565 < local_98) {
          local_98 = 1.4922565;
        }
      }
      else {
        local_98 = -1.4922565;
      }
    }
    if ((local_a4 != 0.0) || (*(char *)(iVar1 + 0x26) != '\0')) {
      fVar6 = ABS(*(float *)(iVar1 + 0x10) - local_a4) * 0.63661975;
      fVar2 = *(float *)(iVar7 + 0x70);
      fVar3 = *(float *)(iVar7 + 0x6c);
      fVar4 = *(float *)(iVar7 + 0x68);
      if (local_a4 == 0.0) {
        fVar6 = SQRT(fVar4 * fVar4 + fVar2 * fVar2 + fVar3 * fVar3) * *(float *)(iVar5 + 0x54) *
                fVar6;
      }
      else {
        fVar6 = SQRT(fVar2 * fVar2 + fVar3 * fVar3 + fVar4 * fVar4) * fVar6 * 0.08;
      }
      FUN_00470d40(local_a4,fVar6);
    }
  }
  local_9c = local_9c - *(float *)(iVar1 + 0x38);
  if (-0.012271847 <= local_9c) {
    if (0.012271847 < local_9c) {
      local_9c = 0.012271847;
    }
  }
  else {
    local_9c = -0.012271847;
  }
  *(float *)(iVar1 + 0x38) = local_9c + *(float *)(iVar1 + 0x38);
  local_98 = local_98 - *(float *)(iVar1 + 0x3c);
  if (-0.012271847 <= local_98) {
    if (0.012271847 < local_98) {
      local_98 = 0.012271847;
    }
  }
  else {
    local_98 = -0.012271847;
  }
  *(float *)(iVar1 + 0x3c) = local_98 + *(float *)(iVar1 + 0x3c);
  param_2 = param_2 + *(float *)(iVar1 + 0x10);
  *(float *)(iVar1 + 0x10) = param_2;
  if (*(float *)(iVar1 + 0x38) <= param_2) {
    if (param_2 <= *(float *)(iVar1 + 0x3c)) {
      *(float *)(iVar1 + 0x10) = param_2;
      return;
    }
    *(undefined4 *)(iVar1 + 0x10) = *(undefined4 *)(iVar1 + 0x3c);
    return;
  }
  *(undefined4 *)(iVar1 + 0x10) = *(undefined4 *)(iVar1 + 0x38);
  return;
}
#endif
