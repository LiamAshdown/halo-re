// game_engine_update_local_player_look  (Ghidra: FUN_00472160; renamed, no established name)
// address 0x472160, size 1346 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (REWRITTEN from objdump 0x472160..0x4726a1)
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
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern Globals *global_globals;                              // 0x00746fa0
extern data_array *object_data;                            // 0x008603b0
extern tag_instance *tag_instances;                            // 0x0087bc14

// camera_basis_out now lives in types/game.h (folded there by the phase-4 review).

extern void chimera__spectate_fp_camera_position(camera_basis_out *out, int16_t local_player_index); // 0x472020, ESI, AX
extern void value_step_toward_target(float *value, float target, float max_step); // 0x470d40, ECX, stack
extern real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b); // 0x4cd4f0, ECX, EDX
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                void *marker, uint32_t flags); // 0x4f6080

extern double atan2(double y, double x); // x87 FPATAN
extern double cos(double x);
extern double sin(double x);
extern double sqrt(double x);            // x87 FSQRT
extern double fabs(double x);

static real look_wrap_angle(real a)
{
    if (!(a < 3.1415927f)) {
        a -= 6.2831855f;
    }
    if (a <= -3.1415927f) {
        a += 6.2831855f;
    }
    return a;
}

// REWRITTEN from objdump. yaw += yaw_delta. In a seat whose definition (unit tag +0x2e8, stride 0x11c) has a
//   yaw range (+0xf0 / +0xf4), the seat marker (name at +0x24) gives the base heading atan2(forward.j,
//   forward.i); a yaw outside [base + min, base + max] snaps to the nearer edge. The yaw is then wrapped into
//   [0, 2pi]. With a unit camera block (camera + 8: +0x40 auto-level pitch, +0x44 / +0x48 pitch range), a
//   non-zero range replaces the default +-1.4922565 limits. When seated and the unit's up.k is above 0.2, the
//   limits and target are shifted by pi/2 minus the angle between the unit's up and the horizontal yaw
//   direction, then clamped back to +-1.4922565. A non-zero target, or autolevelling, steps the pitch toward
//   the target by |pitch - target| * 2/pi * |velocity| * (0.08, or the globals' autolevelling scale when the
//   target is 0). The stored pitch limits move toward the new ones by at most 0.0122718 per tick, and
//   pitch += pitch_delta is clamped between them. The draft never read the camera block (autolevelling and
//   seat pitch limits were dead) and wrote the 0x6c-byte seat marker into a 60-byte buffer.
// blam-cc: AX -> local_player_index, stack -> yaw_delta, pitch_delta
void game_engine_update_local_player_look(int16_t local_player_index, real yaw_delta, real pitch_delta)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    GlobalsPlayerControl *player_control = (GlobalsPlayerControl *)global_globals->player_control.pointer;
    real pitch_min = -1.4922565f;
    real pitch_max = 1.4922565f;
    camera_basis_out camera;
    uint8_t *unit_camera;
    uint8_t *unit;

    chimera__spectate_fp_camera_position(&camera, local_player_index);
    look->yaw = yaw_delta + look->yaw;

    if (camera.seat_index != -1) {
        uint8_t *unit_object = (uint8_t *)((object_header *)object_data->data)[camera.unit & 0xffff].data;
        uint8_t *unit_tag = (uint8_t *)tag_instances[*(datum_index *)unit_object & 0xffff].data;
        uint8_t *seat = *(uint8_t **)(unit_tag + 0x2e8) + (int32_t)camera.seat_index * 0x11c;
        real yaw_min = *(real *)(seat + 0xf0);
        real yaw_max = *(real *)(seat + 0xf4);

        if (yaw_min != 0.0f || yaw_max != 0.0f) {
            object_marker marker;
            real base, a, b, span, forward_delta, back_delta;

            object_get_node_local_transform(camera.unit, (char *)(seat + 0x24), &marker, 1);
            base = (real)atan2((double)*(real *)((uint8_t *)&marker + 0x40), (double)*(real *)((uint8_t *)&marker + 0x3c));
            a = base + yaw_min;
            b = base + yaw_max;
            span = look_wrap_angle(b - a);
            forward_delta = look_wrap_angle(b - look->yaw);
            back_delta = look_wrap_angle(look->yaw - a);
            if (!(span >= 0.0f)) {
                span += 6.2831855f;
            }
            if (!((forward_delta >= 0.0f && forward_delta < span) || (back_delta >= 0.0f && back_delta < span))) {
                if ((real)fabs((double)forward_delta) <= (real)fabs((double)back_delta)) {
                    look->yaw = b;
                } else {
                    look->yaw = a;
                }
            }
        }
    }

    if (!(look->yaw >= 0.0f)) {
        real yaw = look->yaw;
        do {
            yaw += 6.2831855f;
        } while (!(yaw >= 0.0f));
        look->yaw = yaw;
    }
    if (look->yaw > 6.2831855f) {
        real yaw = look->yaw;
        do {
            yaw -= 6.2831855f;
        } while (yaw > 6.2831855f);
        look->yaw = yaw;
    }

    unit_camera = camera.marker_offset;
    if (unit_camera != 0) {
        real target_pitch = *(real *)(unit_camera + 0x40);

        unit = (uint8_t *)((object_header *)object_data->data)[camera.unit & 0xffff].data;
        if (*(real *)(unit_camera + 0x48) != 0.0f || *(real *)(unit_camera + 0x44) != 0.0f) {
            pitch_min = *(real *)(unit_camera + 0x44);
            pitch_max = *(real *)(unit_camera + 0x48);
            if (camera.seat_index != -1 && ((unit_object *)unit)->base.up.k > 0.2f) {
                real_vector3d heading;
                real adjust;

                heading.i = (real)cos((double)look->yaw) * 1.0f; // cos(0.0) of the double at 0x672c08
                heading.j = (real)sin((double)look->yaw) * 1.0f;
                heading.k = 0.0f;                               // sin(0.0)
                adjust = 1.5707964f - vector3d_angle_between_4cd4f0((real_vector3d *)(unit + 0x80), &heading);
                pitch_min = pitch_min - adjust;
                pitch_max = pitch_max - adjust;
                target_pitch = target_pitch - adjust;
            }
            if (!(pitch_min >= -1.4922565f)) {
                pitch_min = -1.4922565f;
            } else if (!(pitch_min <= 1.4922565f)) {
                pitch_min = 1.4922565f;
            }
            if (!(pitch_max >= -1.4922565f)) {
                pitch_max = -1.4922565f;
            } else if (!(pitch_max <= 1.4922565f)) {
                pitch_max = 1.4922565f;
            }
        }
        if (target_pitch != 0.0f || look->autolevelling_active) {
            real error = (real)(fabs((double)(look->pitch - target_pitch)) * 0.6366197466850281);
            real_vector3d *velocity = (real_vector3d *)(unit + 0x68);
            real speed = (real)sqrt((double)(velocity->i * velocity->i + velocity->j * velocity->j +
                velocity->k * velocity->k));
            real step;

            if (target_pitch != 0.0f) {
                step = speed * error * 0.08f;
            } else {
                step = speed * player_control->look_autolevelling_scale * error;
            }
            value_step_toward_target(&look->pitch, target_pitch, step);
        }
    }

    {
        real d = pitch_min - look->pitch_minimum;
        if (!(d >= -0.012271847f)) {
            d = -0.012271847f;
        } else if (!(d <= 0.012271847f)) {
            d = 0.012271847f;
        }
        look->pitch_minimum = d + look->pitch_minimum;
        d = pitch_max - look->pitch_maximum;
        if (!(d >= -0.012271847f)) {
            d = -0.012271847f;
        } else if (!(d <= 0.012271847f)) {
            d = 0.012271847f;
        }
        look->pitch_maximum = d + look->pitch_maximum;
    }

    pitch_delta = pitch_delta + look->pitch;
    look->pitch = pitch_delta;
    if (!(pitch_delta >= look->pitch_minimum)) {
        look->pitch = look->pitch_minimum;
    } else if (!(pitch_delta <= look->pitch_maximum)) {
        look->pitch = look->pitch_maximum;
    }
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
