// actor_movement_apply_steering  (Ghidra: FUN_004180c0; turns the movement goal into this tick's step and facing)
// address 0x4180c0, size 2239 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x4180c0..0x418a02 (the draft "called" the 0x418a04 case labels as functions and
//   returned, rotated the heading instead of the aim direction, and fed the frame projection the wrong
//   vectors). EAX: the cached strafe axis (0..3 reuses it), CL: keep the vertical component; stack (actor, avoid
//   check, its distance squared, order failed, turn rates p5..p9, desired offset, out aim, out axis, out step,
//   out "turning", out "arrived"). The aim direction comes from the cached axis, the strafe-axis chooser
//   (0x418a40) or the offset itself; close to the goal with an avoid check it is projected into the facing frame
//   (0x418c20, axis 4). A step along the chosen axis is taken only once the facing is within the turn cone (the
//   Actor tag's cosine +0xa0 far from the goal, 0.95 when a portal is 0.4 ahead), scaled down while braking
//   (0x4173a0). The aim is then turned towards the facing by at most the rate limits (held turn at +0x594).
// blam-cc: EAX -> cached_axis, ECX -> keep_z, stack -> the 15 parameters in order

// FIXED 2026-09-28: global_origin3d_pointer here is the global at its address comment, global_zero_point3d_pointer (the name belonged to another
// global at a different address, so the link bound it there).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"
#include "units.h"


extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern ScenarioStructureBSP *global_structure_bsp;
extern const real_vector3d *global_origin3d_pointer; // 0x00696714

extern double acos(double x); // 0x628140, CRT
extern double sqrt(double x);
extern double sin(double x);  // FSIN
extern double cos(double x);  // FCOS
extern double fabs(double x);

extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX out, ECX a, stack b
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820, EAX, ECX, stack
extern void actor_update_target_lead_position(datum_index actor_index); // 0x429570, EAX
extern float actor_compute_accuracy_scale(datum_index actor_index); // 0x429620, EAX
extern void actor_movement_get_stopping_distances(datum_index actor_index, float *out_accelerate_stop_distance,
                                                  float *out_stop_distance); // 0x4173a0, EAX, EBX, EDI
extern void actor_movement_choose_strafe_axis(const real_vector3d *direction, uint8_t use_3d,
                                              const real_vector3d *facing, const real_vector3d *reference,
                                              real_vector3d *out_axis, int16_t *out_index); // 0x418a40, EAX, BL, ESI, EDI, stack
extern void actor_movement_project_into_frame(uint8_t use_3d, const real_vector3d *frame_axis,
                                              const real_vector3d *v, real_vector3d *out); // 0x418c20, AL, ECX, stack
extern uint8_t path_find_trace_bsp_boundary(void *map, uint8_t ignore_permission, real_point3d *start, int32_t start_surface,
    real_point3d *end, int32_t target_surface, path_find_boundary_crossing *out_result); // 0x43d9b0

#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

// blam-cc: EAX -> cached_axis, ECX -> keep_z, stack -> the 15 parameters below
void actor_movement_apply_steering(
    int16_t cached_axis, uint8_t keep_z,
    datum_index actor_index, uint8_t want_avoid_check, float avoid_threshold, uint8_t order_failed,
    float steering_maximum, float oversteer_min, float oversteer_max, float avoidance_scale, float throttle_maximum,
    real_vector3d *desired_direction, real_vector3d *out_direction, int16_t *out_axis,
    real_vector3d *out_heading, uint8_t *out_flag_507, uint8_t *out_flag_506)
{
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    real_vector3d *facing = (real_vector3d *)(act + 0x174);
    float max_turn_cos = 0.8660254f; // S+0x18, later the accelerate-then-stop distance
    int16_t chosen_axis = -1;        // S+0x14
    real_vector3d aim;               // S+0x24
    real_vector3d heading;           // S+0x30
    real_vector3d desired;           // S+0x3c
    real_vector3d rotated;           // S+0x48
    float dot_facing;                // S+0x1c
    uint8_t take_step;               // S+0x13
    float stop_distance;             // the actor-index argument slot at 0x41865b
    float desired_length_squared;
    float turn_limit = throttle_maximum;

    if (act[0x42a]) {
        act[0x591] = 1;
    }
    if (cached_axis >= 0 && cached_axis <= 3) {
        chosen_axis = cached_axis;
        heading = *desired_direction;
        if (!keep_z) {
            heading.k = 0.0f;
        }
        if (vector3d_normalize_with_length(&heading) == 0.0f) {
            heading = *facing;
        }
        switch (cached_axis) { // 0x418a04
        case 0: aim = heading; break;
        case 1: aim.i = -heading.i; aim.j = -heading.j; aim.k = heading.k; break;
        case 2: aim.i = -heading.j; aim.j = heading.i; aim.k = heading.k; break;
        default: aim.i = heading.j; aim.j = -heading.i; aim.k = heading.k; break;
        }
        if (want_avoid_check) {
            actor_movement_project_into_frame(keep_z, &aim, &heading, &rotated);
            chosen_axis = 4;
        }
    } else {
        desired_length_squared = desired_direction->i * desired_direction->i +
                                 desired_direction->j * desired_direction->j +
                                 desired_direction->k * desired_direction->k;
        if (desired_length_squared > 0.64000005f) {
            max_turn_cos = ((Actor *)actor_tag)->cosine_begin_moving_angle;
        }
        if (want_avoid_check && desired_length_squared <= avoid_threshold) {
            uint8_t use_scratch = 0;
            real_vector3d fallback;

            desired = *desired_direction;
            if (act[0x505]) {
                aim = *(real_vector3d *)&((struct actor *)act)->forced_aim_direction.i;
                if (((struct actor *)act)->vehicle_driving_type > 0) {
                    use_scratch = 1;
                }
            } else {
                aim = *facing;
            }
            if (!keep_z) {
                desired.k = 0.0f;
                aim.k = 0.0f;
            }
            if (vector3d_normalize_with_length(&aim) == 0.0f) {
                aim = *facing;
            }
            fallback = aim;
            if (vector3d_normalize_with_length(&desired) == 0.0f) {
                desired = fallback;
            }
            if (use_scratch) {
                heading = *facing;
                if (!keep_z) {
                    heading.k = 0.0f;
                }
                if (vector3d_normalize_with_length(&heading) == 0.0f) {
                    heading = fallback;
                }
                actor_movement_project_into_frame(keep_z, &heading, &desired, &rotated);
            } else {
                actor_movement_project_into_frame(keep_z, &aim, &desired, &rotated);
            }
            chosen_axis = 4;
        } else if (act[0x505]) {
            actor_movement_choose_strafe_axis(desired_direction, keep_z, facing, (real_vector3d *)(act + 0x524), &aim,
                                              &chosen_axis);
        } else {
            aim = *desired_direction;
            if (!keep_z) {
                aim.k = 0.0f;
            }
            if (vector3d_normalize_with_length(&aim) == 0.0f) {
                aim = *facing;
            }
            chosen_axis = 0;
        }
    }

    dot_facing = aim.j * facing->j + aim.k * facing->k + aim.i * facing->i;
    if (order_failed || ((struct actor *)act)->control_animation_mode == 4) {
        take_step = 1;
    } else {
        if (!act[0x99]) { // not flying: a portal just ahead in the stepping direction means turn tighter first
            int32_t surface;

            actor_update_target_lead_position(actor_index);
            surface = ((struct actor *)act)->pathfinding_surface_index;
            if (surface != -1 && chosen_axis >= 0 && chosen_axis <= 3) {
                real_vector3d probe;

                switch (chosen_axis) { // 0x418a14
                case 0: probe = *facing; break;
                case 1: probe.i = -facing->i; probe.j = -facing->j; probe.k = facing->k; break;
                case 2: probe.i = facing->j; probe.j = -facing->i; probe.k = facing->k; break;
                default: probe.i = -facing->j; probe.j = facing->i; probe.k = facing->k; break;
                }
                if (vector2d_normalize_with_length((real_vector2d *)&probe) > 0.0f) {
                    real_point3d point;
                    path_find_boundary_crossing crossing;

                    point.x = probe.i * 0.4f + ((actor *)act)->body_position.x;
                    point.y = probe.j * 0.4f + ((actor *)act)->body_position.y;
                    point.z = ((actor *)act)->body_position.z;
                    if (path_find_trace_bsp_boundary(global_structure_bsp, act[0x376], (real_point3d *)(act + 0x12c),
                                                     surface, &point, -1, &crossing) &&
                        !(max_turn_cos > 0.95f)) {
                        max_turn_cos = 0.95f;
                    }
                }
            }
        }
        take_step = (uint8_t)(dot_facing > max_turn_cos);
    }

    {
        float accuracy = actor_compute_accuracy_scale(actor_index);

        desired_length_squared = desired_direction->i * desired_direction->i +
                                 desired_direction->j * desired_direction->j +
                                 desired_direction->k * desired_direction->k;
        *out_flag_506 = (uint8_t)(desired_length_squared <= accuracy * accuracy);
    }
    actor_movement_get_stopping_distances(actor_index, &max_turn_cos, &stop_distance);
    if (!act[0x46e] && stop_distance * stop_distance > desired_length_squared) {
        float distance = (float)sqrt(desired_length_squared);

        if (!(max_turn_cos + 0.05f < distance) || !(stop_distance > max_turn_cos)) {
            turn_limit = 0.0f;
        } else {
            float t = (distance - max_turn_cos) / (stop_distance - max_turn_cos);

            if (turn_limit > t) {
                turn_limit = t;
            }
        }
    }

    heading = *global_origin3d_pointer;
    if (take_step) {
        switch (chosen_axis) { // 0x418a24
        case 0: heading.i = 1.0f; break;
        case 1: heading.i = -1.0f; break;
        case 2: heading.j = -1.0f; break;
        case 3: heading.j = 1.0f; break;
        case 4: heading = rotated; break;
        default: break;
        }
        heading.i *= turn_limit;
        heading.j *= turn_limit;
        heading.k *= turn_limit;
        *out_flag_507 = 0;
    } else {
        act[0x591] = 1;
        *out_flag_507 = 1;
    }

    if (steering_maximum > 0.0f || oversteer_max > 0.0f) {
        float target_angle;
        float angle;
        float *held = (float *)(act + 0x594);

        if (dot_facing >= 1.0f) {
            target_angle = 0.0f;
        } else if (dot_facing <= -1.0f) {
            target_angle = 3.1415927f;
        } else {
            target_angle = (float)acos(dot_facing);
        }
        angle = target_angle;
        if (steering_maximum > 0.0f) {
            float limit = steering_maximum * avoidance_scale;
            float cap = steering_maximum;

            if (avoidance_scale > 1.0f) {
                cap = (avoidance_scale > 1.5f ? 1.5f : avoidance_scale) * steering_maximum;
            }
            if (target_angle * 3.0f <= limit) {
                limit = target_angle * 3.0f;
            }
            if (target_angle < limit) {
                angle = limit;
            } else if (target_angle > cap) {
                angle = cap;
            }
        }
        if (angle > *held) {
            if (act[0x591] && angle > oversteer_min) {
                *held = angle <= oversteer_max ? angle : oversteer_max;
            }
        } else if (*held > 0.0f) {
            if (angle < oversteer_min) {
                *held = 0.0f;
            } else {
                angle = *held;
            }
        }
        {
            float step = angle - target_angle;

            if (fabs(step) > 9.999999747378752e-05) {
                real_vector3d axis;

                // 0x418969: axis = facing x aim, then the aim itself is turned about it
                vector3d_cross_product(&axis, &aim, facing);
                if (vector3d_normalize_with_length(&axis) > 0.0f) {
                    vector3d_rotate_about_axis(&aim, &axis, (real)sin(step), (real)cos(step));
                }
            }
        }
    }

    *out_axis = chosen_axis;
    *out_direction = aim;
    *out_heading = heading;
}

#if 0
Original Ghidra decompilation (0x4180c0):

void FUN_004180c0(float param_1,char param_2,float param_3,char param_4,float param_5,float param_6,
                 float param_7,float param_8,float param_9,float *param_10,float *param_11,
                 undefined2 *param_12,float *param_13,undefined1 *param_14,undefined4 param_15)

{
  float *pfVar1;
  float fVar2;
  int iVar3;
  bool bVar4;
  float fVar5;
  char cVar6;
  short in_AX;
  char in_CL;
  int iVar7;
  float10 fVar8;
  float10 fVar9;
  undefined4 local_5c;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  undefined1 local_1c [28];

  iVar7 = ((uint)param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar3 = *(int *)((*(uint *)(iVar7 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_58 = 0.8660254;
  local_5c = 0xffffffff;
  if (*(char *)(iVar7 + 0x42a) != '\0') {
    *(undefined1 *)(iVar7 + 0x591) = 1;
  }
  if ((-1 < in_AX) && (in_AX < 4)) {
    local_40 = *param_10;
    local_3c = param_10[1];
    local_38 = param_10[2];
    if (in_CL == '\0') {
      local_38 = 0.0;
    }
    fVar8 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar8) {
      local_40 = *(float *)(iVar7 + 0x174);
      local_3c = *(float *)(iVar7 + 0x178);
      local_38 = *(float *)(iVar7 + 0x17c);
    }
                    /* WARNING: Could not recover jumptable at 0x0041818e. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    (*(code *)(&PTR_LAB_00418a04)[in_AX])();
    return;
  }
  fVar2 = param_10[2] * param_10[2] + param_10[1] * param_10[1] + *param_10 * *param_10;
  if (0.64000005 < fVar2) {
    local_58 = *(float *)(iVar3 + 0xa0);
  }
  if ((param_2 == '\0') || (param_3 <= fVar2)) {
    if (*(char *)(iVar7 + 0x505) == '\0') {
      local_4c = *param_10;
      local_48 = param_10[1];
      local_44 = param_10[2];
      if (in_CL == '\0') {
        local_44 = 0.0;
      }
      fVar8 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 == fVar8) {
        local_4c = *(float *)(iVar7 + 0x174);
        local_48 = *(float *)(iVar7 + 0x178);
        local_44 = *(float *)(iVar7 + 0x17c);
      }
      local_5c = 0;
    }
    else {
      FUN_00418a40(&local_4c,&local_5c);
    }
  }
  else {
    local_34 = *param_10;
    local_30 = param_10[1];
    local_2c = param_10[2];
    bVar4 = false;
    if (*(char *)(iVar7 + 0x505) == '\0') {
      local_4c = *(float *)(iVar7 + 0x174);
      local_48 = *(float *)(iVar7 + 0x178);
      local_44 = *(float *)(iVar7 + 0x17c);
    }
    else {
      local_4c = *(float *)(iVar7 + 0x524);
      local_48 = *(float *)(iVar7 + 0x528);
      local_44 = *(float *)(iVar7 + 0x52c);
      if (0 < *(short *)(iVar7 + 0x15e)) {
        bVar4 = true;
      }
    }
    if (in_CL == '\0') {
      local_2c = 0.0;
      local_44 = 0.0;
    }
    fVar8 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar8) {
      local_4c = *(float *)(iVar7 + 0x174);
      local_48 = *(float *)(iVar7 + 0x178);
      local_44 = *(float *)(iVar7 + 0x17c);
    }
    fVar5 = local_48;
    fVar2 = local_4c;
    fVar8 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar8) {
      local_2c = local_44;
      local_34 = fVar2;
      local_30 = fVar5;
    }
    if (bVar4) {
      local_40 = *(float *)(iVar7 + 0x174);
      local_3c = *(float *)(iVar7 + 0x178);
      local_38 = *(float *)(iVar7 + 0x17c);
      if (in_CL == '\0') {
        local_38 = 0.0;
      }
      fVar8 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 == fVar8) {
        local_38 = local_44;
        local_40 = fVar2;
        local_3c = fVar5;
      }
    }
    FUN_00418c20(&local_34,&local_28);
    local_5c = 4;
  }
  pfVar1 = (float *)(iVar7 + 0x174);
  local_54 = local_4c * *pfVar1 +
             local_44 * *(float *)(iVar7 + 0x17c) + local_48 * *(float *)(iVar7 + 0x178);
  if ((param_4 == '\0') && (*(short *)(iVar7 + 0x6dc) != 4)) {
    if (*(char *)(iVar7 + 0x99) == '\0') {
      actor_update_target_lead_position();
      iVar3 = *(int *)(iVar7 + 0x164);
      if (iVar3 != -1) {
        switch((undefined2)local_5c) {
        case 0:
          local_40 = *pfVar1;
          local_3c = *(float *)(iVar7 + 0x178);
          local_38 = *(float *)(iVar7 + 0x17c);
          break;
        case 1:
          local_38 = *(float *)(iVar7 + 0x17c);
          local_40 = -*pfVar1;
          local_3c = -*(float *)(iVar7 + 0x178);
          break;
        case 2:
          local_40 = *(float *)(iVar7 + 0x178);
          local_38 = *(float *)(iVar7 + 0x17c);
          local_3c = -*pfVar1;
          break;
        case 3:
          local_3c = *pfVar1;
          local_38 = *(float *)(iVar7 + 0x17c);
          local_40 = -*(float *)(iVar7 + 0x178);
          break;
        default:
          goto switchD_004184df_default;
        }
        fVar8 = (float10)vector2d_normalize_with_length();
        if ((float10)0.0 < fVar8) {
          local_2c = *(float *)(iVar7 + 0x134);
          local_34 = local_40 * 0.4 + *(float *)(iVar7 + 300);
          local_30 = local_3c * 0.4 + *(float *)(iVar7 + 0x130);
          cVar6 = FUN_0043d9b0(DAT_00746f9c,*(undefined1 *)(iVar7 + 0x376),(float *)(iVar7 + 300),
                               iVar3,&local_34,0xffffffff,local_1c);
          if ((cVar6 != '\0') && (local_58 <= 0.95)) {
            local_58 = 0.95;
          }
        }
      }
    }
switchD_004184df_default:
    bVar4 = false;
    if (local_54 <= local_58) goto LAB_004185fe;
  }
  bVar4 = true;
LAB_004185fe:
  fVar8 = (float10)FUN_00429620();
  fVar9 = (float10)param_10[2] * (float10)param_10[2] +
          (float10)param_10[1] * (float10)param_10[1] + (float10)*param_10 * (float10)*param_10;
  local_50 = (float)fVar9;
  *(bool *)param_15 = fVar9 < fVar8 * fVar8;
  FUN_004173a0();
  if ((*(char *)(iVar7 + 0x46e) == '\0') && (local_50 < param_1 * param_1)) {
    local_50 = SQRT(local_50);
    if ((local_50 <= local_58 + 0.05) || (param_1 <= local_58)) {
      param_9 = 0.0;
    }
    else {
      fVar2 = (local_50 - local_58) / (param_1 - local_58);
      if (fVar2 < param_9) {
        param_9 = fVar2;
      }
    }
  }
  local_40 = *(float *)PTR_DAT_00696714;
  local_3c = *(float *)(PTR_DAT_00696714 + 4);
  local_38 = *(float *)(PTR_DAT_00696714 + 8);
  if (bVar4) {
    switch((undefined2)local_5c) {
    case 0:
      local_40 = 1.0;
      break;
    case 1:
      local_40 = -1.0;
      break;
    case 2:
      local_3c = -1.0;
      break;
    case 3:
      local_3c = 1.0;
      break;
    case 4:
      local_40 = local_28;
      local_3c = local_24;
      local_38 = local_20;
    }
    local_40 = local_40 * param_9;
    *param_14 = 0;
    local_3c = local_3c * param_9;
    local_38 = local_38 * param_9;
  }
  else {
    *(undefined1 *)(iVar7 + 0x591) = 1;
    *param_14 = 1;
  }
  if ((0.0 < param_5) || (0.0 < param_7)) {
    if (local_54 < 1.0) {
      if (local_54 < -1.0 == (local_54 == -1.0)) {
        fVar8 = (float10)FUN_00628140();
      }
      else {
        fVar8 = (float10)3.1415927;
      }
    }
    else {
      fVar8 = (float10)0.0;
    }
    fVar9 = fVar8;
    if (0.0 < param_5) {
      param_1 = param_5 * param_8;
      local_54 = param_5;
      if (1.0 < param_8) {
        if (1.5 < param_8) {
          param_8 = 1.5;
        }
        local_54 = param_8 * param_5;
      }
      if (fVar8 * (float10)3.0 <= (float10)param_1) {
        param_1 = (float)(fVar8 * (float10)3.0);
      }
      if ((float10)param_1 <= fVar8) {
        if ((float10)local_54 < fVar8) {
          fVar9 = (float10)local_54;
        }
      }
      else {
        fVar9 = (float10)param_1;
      }
    }
    if (fVar9 <= (float10)*(float *)(iVar7 + 0x594)) {
      if (0.0 < *(float *)(iVar7 + 0x594)) {
        if ((float10)param_6 <= fVar9) {
          fVar9 = (float10)*(float *)(iVar7 + 0x594);
        }
        else {
          *(undefined4 *)(iVar7 + 0x594) = 0;
        }
      }
    }
    else if ((*(char *)(iVar7 + 0x591) != '\0') && ((float10)param_6 < fVar9)) {
      if (fVar9 <= (float10)param_7) {
        *(float *)(iVar7 + 0x594) = (float)fVar9;
      }
      else {
        *(float *)(iVar7 + 0x594) = param_7;
      }
    }
    fVar2 = (float)(fVar9 - fVar8);
    if (0.0001 < ABS(fVar2)) {
      vector3d_cross_product(pfVar1);
      fVar8 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 < fVar8) {
        fVar8 = (float10)fcos((float10)fVar2);
        fVar9 = (float10)fsin((float10)fVar2);
        vector3d_rotate_about_axis((float)fVar9,(float)fVar8);
      }
    }
  }
  *param_12 = (undefined2)local_5c;
  *param_11 = local_4c;
  param_11[1] = local_48;
  param_11[2] = local_44;
  *param_13 = local_40;
  param_13[1] = local_3c;
  param_13[2] = local_38;
  return;
}
#endif
