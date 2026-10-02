// biped_update_facing  (Ghidra / symbols/functions.txt: unit_update_facing, renamed)
// address 0x55b7c0, size 1745 bytes
// name confidence: 0.7   rewrite confidence: 0.5
// evidence for the rename: every tag field this function reads is a *Biped* field and none of
//   them exists on a bare Unit -- tag+0x2f0 Biped.moving_turning_speed, tag+0x2f4
//   Biped.biped_flags (bit 0x04 "flying", bit 0x40 "can_climb_any_surface", bit 0x01
//   "turns_without_animating"), tag+0x324/0x328/0x32c Biped.bank_angle / bank_apply_time /
//   bank_decay_time, tag+0x330 Biped.pitch_ratio, tag+0x344/0x348
//   Biped.angular_velocity_maximum / angular_acceleration_maximum, tag+0x4c8
//   Biped.cosine_stationary_turning_threshold. It also reads biped_data.movement_state
//   (0x4d2) and read-modify-writes biped_data.unknown_510 (the bank angle 0x560800 takes the
//   cos and sin of). Its single caller is biped_update (0x5590a0), immediately before
//   biped_integrate_movement_with_collision (0x55cfd0), and it shares that caller's
//   stack byte. Same reasoning as the 0x5590a0 unit_update -> biped_update rename already
//   recorded in out/phase4/units_types_notes.md.
// tag+0x17c is Unit.unit_flags (Object tag data is 0x17c bytes, so the Unit block starts
//   there); bit 0x100000 is "special_cinematic_unit" in the UnitFlags bitfield in types/tags.h.
// register convention: object_index in EAX, the caller's animation-state byte on the stack.
//   // blam-cc: in_EAX -> object_index, param_1 -> out_animation_state
// VERIFIED (2026-09-30): the EAX/ECX/stack bindings of every vector3d_cross_product call here were
//   checked against the disassembly (0x55b985, 0x55bb42, 0x55bb52, 0x55bb97, 0x55bd84, 0x55be17, 0x55be3e).
// vector3d_rotate_toward_bounded's register operands (orphan pass 4 review, 0x55bad1..0x55baee):
//   ECX = &target (local_1c, the projected desired direction) and ESI = 0 (no transform).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern real_vector3d *global_up3d_pointer; // 0x00696720, indirect pointer to math.h global_up3d

extern double cos(double x); // x87 FCOS
extern double sin(double x); // x87 FSIN

extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, vector in ECX
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX

// vector3d_cross_product (0x4052c0) computes  *out = stack_operand x ecx_operand,  with out
// in EAX, ecx_operand in ECX and stack_operand pushed -- read out of the callee own
// decompilation (in_EAX / in_ECX / param_1) and matching
// src/objects/object_set_position_and_orientation.c. Ghidra binds only the stack operand at
// the call sites below, so the declaration is left unprototyped.
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX, ECX, stack

// vector3d_rotate_about_axis (0x4cd820) rotates the vector in EAX about the axis in ECX in
// place, by the (sin_angle, cos_angle) pair pushed on the stack -- the callee own
// decompilation is a Rodrigues formula over in_EAX / in_ECX / param_1 / param_2, and
// src/math/vector3d_rotate_toward.c reads it the same way. Ghidra binds only the two stack
// arguments at the call sites below, so the declaration is left unprototyped.
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820
    // FIXED: this was declared without a prototype, so the float sin/cos arguments were promoted to double and the
    //   callee read garbage angles on every biped turn.
  // real signature (vector3d_rotate_about_axis.c): void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); Ghidra recovered 0 of 4 args at this call site
extern void vector3d_rotate_toward_bounded(real_vector3d *current, real_vector3d *velocity, float *bounds,
                          float max_velocity, float max_acceleration, real_vector3d *target,
                          real_matrix4x3 *transform); // 0x564ae0, src/math; blam-cc: stack (current, velocity,
                          // bounds, max_velocity, max_acceleration), ECX target, ESI transform
extern void unit_update_up_vector(Biped *biped_tag, object *obj); // 0x560800

// Turns a biped's body toward its desired facing once per tick.
//
// A walking biped (or any biped whose health is frozen) takes the first path: the desired
// facing is projected into the turning plane -- the XY plane normally, or the plane
// perpendicular to object.up for a can_climb_any_surface biped -- and the sign of its cross
// product with the current forward picks the turn direction, with a tie broken by the current
// turn animation when the two are nearly opposite. If the biped is already in the "moving"
// movement state (or turns without animating) the forward vector is rotated by
// moving_turning_speed per tick and snapped exactly onto the target on the tick it would
// overshoot; if it is standing still instead, and the facing error exceeds
// cosine_stationary_turning_threshold, the caller's animation-state byte is set to the
// turn-in-place state (2 = left, 3 = right) so the caller starts that animation.
//
// A live flying biped takes the second path: it holds its heading when it is essentially at
// rest and already pointed the right way, otherwise it pitches the target by pitch_ratio times
// the vertical throttle, eases its bank angle toward the roll implied by the turn, and hands
// the result to the bounded angular servo. Either path finishes by re-levelling the up vector.
void biped_update_facing(uint32_t object_index, int8_t *out_animation_state) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;

    real_vector3d target;   // local_1c / local_18 / local_14
    real_vector3d scratch;  // local_10 / local_c / local_8
    float turn_cross;
    float facing_dot;
    float threshold;
    uint8_t turn_right;

    if ((tag->biped_flags & 0x00000004) == 0 ||                     // "flying"
        (obj->vitality_flags & _object_health_frozen_bit) != 0) {
        int8_t base_state = unit->base_animation_state;
        uint32_t climbs;

        if (base_state == _unit_base_animation_state_asleep) {
            return;
        }

        climbs = tag->biped_flags & 0x00000040;                     // "can_climb_any_surface"
        if (climbs == 0) {
            // Flat turn: drop the desired facing into the XY plane and renormalize it.
            target.i = unit->desired_facing_vector.i;
            target.j = unit->desired_facing_vector.j;
            target.k = 0.0f;
            if (vector2d_normalize_with_length((real_vector2d *)&target) == 0.0f) {
                target = obj->forward;
            }
            turn_cross = target.i * obj->forward.j - target.j * obj->forward.i;
            facing_dot = target.j * obj->forward.j;
        } else {
            // Surface-climbing turn: project the desired facing into the plane perpendicular
            // to object.up.  scratch = up x desired, target = scratch x up.
            vector3d_cross_product(&scratch, &unit->desired_facing_vector, &obj->up);
            vector3d_cross_product(&target, &obj->up, &scratch);
            if (vector3d_normalize_with_length(&target) == 0.0f) {
                target = obj->forward;
            }
            vector3d_cross_product(&scratch, &obj->forward, &target);
            turn_cross = scratch.i * obj->up.i + scratch.k * obj->up.k + scratch.j * obj->up.j;
            facing_dot = target.k * obj->forward.k + target.j * obj->forward.j;
        }
        facing_dot = target.i * obj->forward.i + facing_dot;

        turn_right = (0.0f < turn_cross);
        if (facing_dot < -0.9f) {
            // Nearly a full reversal: the cross product's sign is meaningless, so keep turning
            // the way the current turn animation is already going.
            if (unit->animation_state == _unit_animation_state_unknown_03) {
                turn_right = 1;
            } else if (unit->animation_state == _unit_animation_state_unknown_02) {
                turn_right = 0;
            }
        }

        if (biped->movement_state == 1 ||
            (tag->biped_flags & 0x00000001) != 0) {                 // "turns_without_animating"
            float turn_sin;
            float turn_cos;
            double angle;

            if ((unit->control_flags & _unit_control_flag_look_dont_turn) != 0) {
                return;
            }

            angle = (double)tag->moving_turning_speed * 0.033333335;  // per tick, 30 ticks/sec
            turn_cos = (float)cos(angle);
            turn_sin = (float)sin(angle);
            if (turn_right) {
                turn_sin = -turn_sin;
            }

            if (climbs == 0) {
                float forward_i = obj->forward.i;
                obj->forward.i = turn_cos * forward_i - turn_sin * obj->forward.j;
                obj->forward.j = turn_cos * obj->forward.j + turn_sin * forward_i;
                turn_cross = target.i * obj->forward.j - target.j * obj->forward.i;
            } else {
                vector3d_rotate_about_axis(&obj->forward, &obj->up, turn_sin, turn_cos);
                vector3d_cross_product(&scratch, &obj->forward, &target);
                turn_cross = scratch.i * obj->up.i + scratch.k * obj->up.k + scratch.j * obj->up.j;
            }

            // Still short of the target: the rotation above is the whole tick's work.
            if (turn_right) {
                if (0.0f <= turn_cross) {
                    return;
                }
            } else if (turn_cross <= 0.0f) {
                return;
            }

            // This tick would overshoot, so land exactly on the target instead.
            if ((tag->biped_flags & 0x00000040) == 0) {
                obj->forward.i = target.i;
                obj->forward.j = target.j;
                obj->forward.k = 0.0f;
                obj->up = *global_up3d_pointer;
            } else {
                vector3d_cross_product(&scratch, &target, &obj->up);
                if (0.0f < vector3d_normalize_with_length(&scratch)) {
                    vector3d_cross_product(&obj->forward, &obj->up, &scratch);
                    vector3d_normalize_with_length(&obj->forward);
                    return;
                }
                // Degenerate (target parallel to up): the original only renormalizes the
                // forward vector and leaves it otherwise untouched.
            }
            vector3d_normalize_with_length(&obj->forward);
            return;
        }

        if (biped->movement_state == 0 &&
            base_state != _unit_base_animation_state_flaming &&
            (unit->flags & _unit_flag_unknown_4000) == 0 &&
            (unit->control_flags & _unit_control_flag_look_dont_turn) == 0) {
            threshold = ((unit->control_flags & _unit_control_flag_exact_facing) != 0)
                            ? 0.99f
                            : tag->cosine_stationary_turning_threshold;
            if (facing_dot < threshold &&
                (((Unit *)tag)->unit_flags & 0x00100000) == 0) {     // "special_cinematic_unit"
                *out_animation_state = (int8_t)(turn_right + 2);     // 2 = turn left, 3 = turn right
            }
        }
        return;
    }

    // --- flying biped ------------------------------------------------------------------
    {
        float pitch;
        float bank_target;
        float bank_blend;
        float bank_time;
        float bounds[4];
        float servo_acceleration;

        if (obj->velocity.k * obj->velocity.k + obj->velocity.j * obj->velocity.j +
                obj->velocity.i * obj->velocity.i < 0.00027777778f &&
            obj->angular_velocity.k * obj->angular_velocity.k +
                obj->angular_velocity.j * obj->angular_velocity.j +
                obj->angular_velocity.i * obj->angular_velocity.i < 1.3538552e-06f &&
            unit->throttle.k * unit->throttle.k + unit->throttle.j * unit->throttle.j +
                unit->throttle.i * unit->throttle.i < 0.010000001f) {
            threshold = ((unit->control_flags & _unit_control_flag_exact_facing) != 0)
                            ? 0.99f
                            : tag->cosine_stationary_turning_threshold;
            if (threshold < unit->desired_facing_vector.i * obj->forward.i +
                                unit->desired_facing_vector.j * obj->forward.j +
                                unit->desired_facing_vector.k * obj->forward.k) {
                // At rest and already aimed: hold the current heading.
                target = obj->forward;
                goto apply_turn;
            }
        }

        pitch = tag->pitch_ratio * unit->throttle.k;
        target = unit->desired_facing_vector;
        if (pitch != 0.0f) {
            target.k = target.k + pitch;
            if (vector3d_normalize_with_length(&target) == 0.0f) {
                target = unit->desired_facing_vector;
            }
        }

    apply_turn:
        // Bank into the turn: the roll target is how far the desired facing sits off the
        // current forward, scaled by the forward throttle and trimmed by the lateral one.
        // VERIFIED against disassembly 0x55b977..0x55b985 (2026-09-30): EAX=scratch (esp+0x2c), ECX=&obj->up (edi+0x80), stack=&obj->forward (edi+0x74);
        // scratch is dotted with the desired facing (esi) below
        vector3d_cross_product(&scratch, &obj->up, &obj->forward);
        bank_target = (scratch.i * unit->desired_facing_vector.i +
                       scratch.k * unit->desired_facing_vector.k +
                       scratch.j * unit->desired_facing_vector.j) *
                          3.3333333f * unit->throttle.i -
                      unit->throttle.j;
        if (1.5f < bank_target) {
            bank_target = 1.5f;
        }
        bank_target = bank_target * tag->bank_angle;

        // Ghidra renders the x87 compare as "(p < 0) == (p == 0)", which is true exactly when
        // the product is strictly positive, i.e. the bank is already leaning the target way.
        if (bank_target * biped->bank_angle > 0.0f) {
            bank_blend = biped->bank_angle / bank_target;
            if (1.0f < bank_blend) {
                bank_blend = 1.0f;
            }
            bank_blend = 1.0f - bank_blend;
        } else {
            bank_blend = 1.0f;
        }
        bank_time = bank_blend * tag->bank_apply_time + (1.0f - bank_blend) * tag->bank_decay_time;
        if (0.0f < bank_time) {
            bank_target = (bank_target - biped->bank_angle) / (bank_time * 30.0f) +
                          biped->bank_angle;
        }
        biped->bank_angle = bank_target;

        bounds[0] = -3.1415927f;   // yaw range
        bounds[1] = 3.1415927f;
        bounds[2] = -1.5707964f;   // pitch range
        bounds[3] = 1.5707964f;

        servo_acceleration = tag->angular_acceleration_maximum * 0.0011111111f; // per tick^2, 1/900
        if (servo_acceleration != 0.0f) {
            vector3d_rotate_toward_bounded(&obj->forward, &obj->angular_velocity, bounds,
                         tag->angular_velocity_maximum * 0.033333335f, servo_acceleration, &target, 0);
        } else {
            obj->forward = target;
        }
        unit_update_up_vector(tag, obj);
    }
}

#if 0
Original Ghidra decompilation (0x55b7c0):

void unit_update_facing(char *param_1)

{
  float fVar1;
  float fVar2;
  char cVar3;
  uint *puVar4;
  int iVar5;
  undefined *puVar6;
  uint in_EAX;
  uint uVar7;
  float *pfVar8;
  bool bVar9;
  float10 fVar10;
  float10 fVar11;
  float local_28;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  undefined4 local_4;

  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar5 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((*(uint *)(iVar5 + 0x2f4) & 4) == 0) || ((*(byte *)((int)puVar4 + 0x106) & 4) != 0)) {
    cVar3 = *(char *)((int)puVar4 + 0x2a7);
    if (cVar3 != '\0') {
      uVar7 = *(uint *)(iVar5 + 0x2f4) & 0x40;
      if (uVar7 == 0) {
        local_1c = (float)puVar4[0x89];
        local_18 = (float)puVar4[0x8a];
        local_14 = 0.0;
        fVar10 = (float10)vector2d_normalize_with_length();
        if ((float10)0.0 == fVar10) {
          local_1c = (float)puVar4[0x1d];
          local_18 = (float)puVar4[0x1e];
          local_14 = (float)puVar4[0x1f];
        }
        fVar1 = local_1c * (float)puVar4[0x1e] - local_18 * (float)puVar4[0x1d];
        fVar2 = local_18 * (float)puVar4[0x1e];
      }
      else {
        vector3d_cross_product(puVar4 + 0x20);
        vector3d_cross_product(&local_10);
        fVar10 = (float10)vector3d_normalize_with_length();
        if ((float10)0.0 == fVar10) {
          local_1c = (float)puVar4[0x1d];
          local_18 = (float)puVar4[0x1e];
          local_14 = (float)puVar4[0x1f];
        }
        vector3d_cross_product(&local_1c);
        fVar1 = local_10 * (float)puVar4[0x20] +
                local_8 * (float)puVar4[0x22] + local_c * (float)puVar4[0x21];
        fVar2 = local_14 * (float)puVar4[0x1f] + local_18 * (float)puVar4[0x1e];
      }
      pfVar8 = (float *)(puVar4 + 0x1d);
      fVar2 = local_1c * *pfVar8 + fVar2;
      bVar9 = 0.0 < fVar1;
      if (fVar2 < -0.9) {
        if (*(char *)((int)puVar4 + 0x2a3) == '\x03') {
          bVar9 = true;
        }
        else if (*(char *)((int)puVar4 + 0x2a3) == '\x02') {
          bVar9 = false;
        }
      }
      if ((*(char *)((int)puVar4 + 0x4d2) == '\x01') || ((*(byte *)(iVar5 + 0x2f4) & 1) != 0)) {
        if ((puVar4[0x82] & 0x100) == 0) {
          fVar10 = (float10)*(float *)(iVar5 + 0x2f0) * (float10)0.033333335;
          fVar11 = (float10)fcos(fVar10);
          fVar1 = (float)fVar11;
          fVar10 = (float10)fsin(fVar10);
          local_28 = (float)fVar10;
          if (bVar9) {
            local_28 = -local_28;
          }
          if (uVar7 == 0) {
            fVar2 = *pfVar8;
            *pfVar8 = fVar1 * *pfVar8 - local_28 * (float)puVar4[0x1e];
            puVar4[0x1e] = (uint)(fVar1 * (float)puVar4[0x1e] + local_28 * fVar2);
            fVar1 = local_1c * (float)puVar4[0x1e] - local_18 * *pfVar8;
          }
          else {
            vector3d_rotate_about_axis(local_28,fVar1);
            vector3d_cross_product(&local_1c);
            fVar1 = local_10 * (float)puVar4[0x20] +
                    local_8 * (float)puVar4[0x22] + local_c * (float)puVar4[0x21];
          }
          puVar6 = PTR_DAT_00696720;
          if (bVar9) {
            if (0.0 <= fVar1) {
              return;
            }
          }
          else if (fVar1 <= 0.0) {
            return;
          }
          if ((*(byte *)(iVar5 + 0x2f4) & 0x40) == 0) {
            *pfVar8 = local_1c;
            puVar4[0x1e] = (uint)local_18;
            puVar4[0x1f] = 0;
            puVar4[0x20] = *(uint *)puVar6;
            puVar4[0x21] = *(uint *)(puVar6 + 4);
            puVar4[0x22] = *(uint *)(puVar6 + 8);
          }
          else {
            vector3d_cross_product(puVar4 + 0x20);
            fVar10 = (float10)vector3d_normalize_with_length();
            if ((float10)0.0 < fVar10) {
              vector3d_cross_product(&local_10);
              vector3d_normalize_with_length();
              return;
            }
          }
          vector3d_normalize_with_length();
        }
      }
      else if ((((*(char *)((int)puVar4 + 0x4d2) == '\0') && (cVar3 != '\x05')) &&
               ((puVar4[0x81] & 0x4000) == 0)) && ((puVar4[0x82] & 0x100) == 0)) {
        if ((puVar4[0x82] & 0x20) == 0) {
          fVar1 = *(float *)(iVar5 + 0x4c8);
        }
        else {
          fVar1 = 0.99;
        }
        if ((fVar2 < fVar1) && ((*(uint *)(iVar5 + 0x17c) & 0x100000) == 0)) {
          *param_1 = bVar9 + '\x02';
          return;
        }
      }
    }
    return;
  }
  if ((((float)puVar4[0x1c] * (float)puVar4[0x1c] +
        (float)puVar4[0x1b] * (float)puVar4[0x1b] + (float)puVar4[0x1a] * (float)puVar4[0x1a] <
        0.00027777778) &&
      ((float)puVar4[0x25] * (float)puVar4[0x25] +
       (float)puVar4[0x24] * (float)puVar4[0x24] + (float)puVar4[0x23] * (float)puVar4[0x23] <
       1.3538552e-06)) &&
     ((float)puVar4[0xa0] * (float)puVar4[0xa0] +
      (float)puVar4[0x9f] * (float)puVar4[0x9f] + (float)puVar4[0x9e] * (float)puVar4[0x9e] <
      0.010000001)) {
    if ((puVar4[0x82] & 0x20) == 0) {
      fVar1 = *(float *)(iVar5 + 0x4c8);
    }
    else {
      fVar1 = 0.99;
    }
    if (fVar1 < (float)puVar4[0x89] * (float)puVar4[0x1d] +
                (float)puVar4[0x8a] * (float)puVar4[0x1e] +
                (float)puVar4[0x8b] * (float)puVar4[0x1f]) {
      local_1c = (float)puVar4[0x1d];
      local_18 = (float)puVar4[0x1e];
      local_14 = (float)puVar4[0x1f];
      goto LAB_0055b977;
    }
  }
  fVar1 = *(float *)(iVar5 + 0x330) * (float)puVar4[0xa0];
  local_1c = (float)puVar4[0x89];
  local_18 = (float)puVar4[0x8a];
  local_14 = (float)puVar4[0x8b];
  if (fVar1 != 0.0) {
    local_14 = local_14 + fVar1;
    fVar10 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar10) {
      local_1c = (float)puVar4[0x89];
      local_18 = (float)puVar4[0x8a];
      local_14 = (float)puVar4[0x8b];
    }
  }
LAB_0055b977:
  pfVar8 = (float *)(puVar4 + 0x1d);
  vector3d_cross_product(pfVar8);
  fVar1 = (local_10 * (float)puVar4[0x89] +
          local_8 * (float)puVar4[0x8b] + local_c * (float)puVar4[0x8a]) * 3.3333333 *
          (float)puVar4[0x9e] - (float)puVar4[0x9f];
  if (1.5 < fVar1) {
    fVar1 = 1.5;
  }
  fVar1 = fVar1 * *(float *)(iVar5 + 0x324);
  if (fVar1 * (float)puVar4[0x144] < 0.0 == (fVar1 * (float)puVar4[0x144] == 0.0)) {
    fVar2 = (float)puVar4[0x144] / fVar1;
    if (1.0 < fVar2) {
      fVar2 = 1.0;
    }
    fVar2 = 1.0 - fVar2;
  }
  else {
    fVar2 = 1.0;
  }
  fVar2 = fVar2 * *(float *)(iVar5 + 0x328) + (1.0 - fVar2) * *(float *)(iVar5 + 0x32c);
  if (0.0 < fVar2) {
    fVar1 = (fVar1 - (float)puVar4[0x144]) / (fVar2 * 30.0) + (float)puVar4[0x144];
  }
  puVar4[0x144] = (uint)fVar1;
  local_10 = -3.1415927;
  local_c = 3.1415927;
  local_8 = -1.5707964;
  local_4 = 0x3fc90fdb;
  fVar1 = *(float *)(iVar5 + 0x348) * 0.0011111111;
  if (fVar1 != 0.0) {
    FUN_00564ae0(pfVar8,puVar4 + 0x23,&local_10,*(float *)(iVar5 + 0x344) * 0.033333335,fVar1);
    FUN_00560800();
    return;
  }
  *pfVar8 = local_1c;
  puVar4[0x1e] = (uint)local_18;
  puVar4[0x1f] = (uint)local_14;
  FUN_00560800();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
