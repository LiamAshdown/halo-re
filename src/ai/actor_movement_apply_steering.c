// actor_movement_apply_steering  (Ghidra: actor_movement_apply_steering, renamed)
// address 0x4180c0, size 2239 bytes
// name confidence: 0.45   rewrite confidence: 0.3
// evidence: out/phase4/ai_functions.md "Core steering/turn-smoothing routine that converts a
//   desired movement direction into the actor's applied heading, honoring per-mode special
//   handling and a maximum turn rate."; types/ai.h actor.unknown_524 ("steering scratch
//   written by 0x4180c0") and actor.unknown_594[4] ("turn-smoothing scratch written by
//   0x4180c0") are both already attributed to this function by the header, and this rewrite
//   confirms both writes. The five output pointers this function is handed by its one caller
//   (actor_movement_update @0x416790) land on actor.unknown_518 (input), actor.position_cache_a
//   (0x5a4), actor.unknown_50a, actor.queued_look_vector (0x6e0) and the two flag bytes
//   actor.unknown_506 / actor.unknown_507.
// register convention: two register-passed inputs Ghidra could not fold into the recognized
//   stack signature -- a cached "last chosen axis" selector in AX and a "keep the Z
//   component" flag in CL -- ahead of the 15 recognized stack parameters, in the order the
//   sole call site (actor_movement_update) passes them.
//   // blam-cc: EAX -> cached_axis, ECX -> keep_z, stack -> param_1, want_avoid_check,
//   avoid_threshold, order_failed, param_5, param_6, param_7, param_8, param_9,
//   desired_direction, out_direction, out_axis, out_heading, out_flag_507, out_flag_506
//
// UNSURE, broadly -- this function needs a disassembly review pass:
//  - cached_axis (in_AX) is read once, at entry, and the same-shaped value (0..4) is written
//    back at the end through out_axis into actor.unknown_50a. Several of the caller's paths
//    into this call never touch actor.unknown_50a beforehand, which is consistent with AX
//    being loaded straight from actor.unknown_50a's *previous* tick value (a "try to keep
//    last tick's chosen avoidance axis" cache) rather than a value the caller computes fresh.
//    Modeled that way in the caller (actor_movement_update.c).
//  - keep_z (in_CL) is used exactly where the function decides whether to flatten a direction
//    to 2D. actor.flying (+0x99) gates the analogous decision everywhere else in this module,
//    and this function itself re-reads +0x99 a few lines later for a related branch, so
//    keep_z is modeled as actor.flying in the caller. Not proven.
//  - param_1 is read as a truncated array index at entry (`(uint)param_1 & 0xffff`, fed the
//    caller's own actor index) and, later, as a plain float compared against a squared
//    distance and against a cosine-like bound, and is finally overwritten with an unrelated
//    computed float. Both kinds of use are transcribed literally below; whether this is a
//    genuine dual-purpose parameter or a decompiler artifact merging two distinct stack slots
//    across the parameter's dead range is not resolved here.
//  - The direction-dispatch block just past entry ends in an indirect call Ghidra flags as
//    unrecoverable ("Could not recover jumptable at 0x0041818e. Too many branches", "Treating
//    indirect jump as call"): preserved literally as a call through a 4-entry function
//    pointer table, since nothing in this session can recover what the four targets are or
//    whether they take arguments.
//  - path_find_trace_bsp_boundary, actor_movement_choose_strafe_axis, actor_movement_project_into_frame, actor_movement_get_stopping_distances and actor_compute_accuracy_scale are outside this
//    session's range; their signatures below are read off this call site only, not verified
//    against their own bodies.
//  - FUN_00628140 (CRT, 0x628140) is called with no visible argument at a point where the
//    preceding comparisons already have local_54 as the live FPU top-of-stack value; modeled
//    as acos(local_54), matching the surrounding clamp-to-[-1,1] pattern.
//  - The unit type tag fields at unit_type_def+0x398/0x39c/0x3a0/0x3a4 have no entry in
//    types/tags.h at this granularity (that struct belongs to the units module); left as raw
//    offsets.
// reconciled: R06 0x00746f9c is ScenarioStructureBSP *global_structure_bsp (was extern int32_t bsp_generation); ai.h path_find_context/actor_movement_context bsp_generation -> structure_bsp, bsp_index -> collision_bsp

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, scenario.h (formerly bsp_generation)
extern const real_vector3d *global_origin3d_pointer; // 0x00696714

extern double acos(double x); // 0x628140, CRT; see UNSURE above
extern double sqrt(double x);
extern double sin(double x);  // FSIN
extern double cos(double x);  // FCOS
extern double fabs(double x);

extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX -> v
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX -> v
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX->out, stack->a, ECX->b
extern void vector3d_rotate_about_axis(real_vector3d *v, const real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820, EAX->v, ECX->axis, stack->sin,cos

extern void actor_update_target_lead_position(datum_index actor_index); // 0x429570, not yet rewritten (this module)
extern float actor_compute_accuracy_scale(datum_index actor_index); // 0x429620, EAX
extern void actor_movement_get_stopping_distances(datum_index actor_index, float *out_accelerate_stop_distance, float *out_stop_distance);                       // 0x4173a0, not yet rewritten (this module): turning-radius bounds, called for its side effects only
// Picks whichever of four candidate axis directions best matches two reference vectors;
// writes the refined direction and the chosen axis index (0..3). UNSURE signature.
extern void actor_movement_choose_strafe_axis(real_vector3d *direction_in_out, int32_t *axis_out); // 0x418a40, not yet rewritten (this module)
// Rotates a direction vector into (or out of) the actor's local orientation frame. UNSURE signature.
extern void actor_movement_project_into_frame(const real_vector3d *in, real_vector3d *out); // 0x418c20, not yet rewritten (this module)
// Traces a straight segment across a BSP cluster's connected edges for the first portal
// boundary it crosses. UNSURE signature, read off this call site only.
extern uint8_t path_find_trace_bsp_boundary(int32_t structure_bsp, uint8_t ignores_glass, const real_point3d *from,
                            int32_t surface_index, const real_point3d *to, uint32_t sentinel,
                            uint8_t out_result[28]); // 0x43d9b0, not yet rewritten (this module)

// The four direction-dispatch targets the entry fast path calls through. Ghidra could not
// recover this jump table (see UNSURE above); left as an opaque function-pointer table.
extern void (*const actor_movement_apply_steering_dispatch[4])(void); // 0x418a04

// blam-cc: EAX -> cached_axis, ECX -> keep_z, stack -> the 15 parameters below
void actor_movement_apply_steering(
    int16_t cached_axis, uint8_t keep_z,
    datum_index actor_index, uint8_t want_avoid_check, float avoid_threshold, uint8_t order_failed,
    float param_5, float param_6, float param_7, float param_8, float param_9,
    real_vector3d *desired_direction, real_vector3d *out_direction, int16_t *out_axis,
    real_vector3d *out_heading, uint8_t *out_flag_507, uint8_t *out_flag_506)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffffu];
    uint8_t *actor_base = (uint8_t *)a;
    Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;

    real max_turn_cos = 0.8660254f; // ~30 degrees; the default max-turn bound
    int32_t chosen_axis = -1;       // local_5c
    real_vector3d aim_dir;          // local_4c/local_48/local_44
    real_vector3d heading;          // local_40/local_3c/local_38 (the function's return heading)
    real_vector3d rotated_result;   // local_28/local_24/local_20, only set (and only meaningful) when chosen_axis == 4
    real desired_length_sq;
    real dot_facing;                // local_54
    uint8_t take_max_turn_path;     // bVar4 (reused by Ghidra for a second, unrelated bool later; split here)

    if (a->unknown_42a != 0) {
        a->unknown_591 = 1;
    }

    if (-1 < cached_axis && cached_axis < 4) {
        heading.i = desired_direction->i;
        heading.j = desired_direction->j;
        heading.k = desired_direction->k;
        if (keep_z == 0) {
            heading.k = 0.0f;
        }
        if (vector3d_normalize_with_length(&heading) == 0.0f) {
            heading = a->facing;
        }
        // UNSURE: Ghidra could not recover this jump table; preserved literally as an
        // indirect call with no visible arguments, then an immediate return. See file header.
        actor_movement_apply_steering_dispatch[cached_axis]();
        return;
    }

    desired_length_sq = desired_direction->k * desired_direction->k +
                        desired_direction->j * desired_direction->j +
                        desired_direction->i * desired_direction->i;
    if (0.64000005f < desired_length_sq) {
        max_turn_cos = actor_def->cosine_begin_moving_angle;
    }

    if (want_avoid_check == 0 || avoid_threshold <= desired_length_sq) {
        if (a->unknown_505 == 0) {
            aim_dir.i = desired_direction->i;
            aim_dir.j = desired_direction->j;
            aim_dir.k = desired_direction->k;
            if (keep_z == 0) {
                aim_dir.k = 0.0f;
            }
            if (vector3d_normalize_with_length(&aim_dir) == 0.0f) {
                aim_dir = a->facing;
            }
            chosen_axis = 0;
        } else {
            actor_movement_choose_strafe_axis(&aim_dir, &chosen_axis);
        }
    } else {
        real_vector3d desired_copy;
        uint8_t use_steering_scratch = 0;

        desired_copy = *desired_direction;
        if (a->unknown_505 == 0) {
            aim_dir.i = a->facing.i;
            aim_dir.j = a->facing.j;
            aim_dir.k = a->facing.k;
        } else {
            aim_dir = a->unknown_524; // steering scratch this function itself maintains
            if (0 < a->unknown_15e) {
                use_steering_scratch = 1;
            }
        }
        if (keep_z == 0) {
            desired_copy.k = 0.0f;
            aim_dir.k = 0.0f;
        }
        if (vector3d_normalize_with_length(&aim_dir) == 0.0f) {
            aim_dir = a->facing;
        }
        {
            real saved_i = aim_dir.i, saved_j = aim_dir.j, saved_k = aim_dir.k;
            if (vector3d_normalize_with_length(&desired_copy) == 0.0f) {
                desired_copy.i = saved_i;
                desired_copy.j = saved_j;
                desired_copy.k = saved_k;
            }
            if (use_steering_scratch) {
                heading.i = a->facing.i;
                heading.j = a->facing.j;
                heading.k = a->facing.k;
                if (keep_z == 0) {
                    heading.k = 0.0f;
                }
                if (vector3d_normalize_with_length(&heading) == 0.0f) {
                    heading.i = saved_i;
                    heading.j = saved_j;
                    heading.k = saved_k;
                }
            }
        }
        actor_movement_project_into_frame(&desired_copy, &rotated_result);
        chosen_axis = 4;
    }

    dot_facing = aim_dir.i * a->facing.i + aim_dir.k * a->facing.k + aim_dir.j * a->facing.j;

    take_max_turn_path = 0;
    if (order_failed == 0 && a->unknown_6dc != 4) {
        if (a->flying == 0) {
            int32_t lead_target_index;

            actor_update_target_lead_position(actor_index);
            lead_target_index = *(int32_t *)(actor_base + 0x164);
            if (lead_target_index != -1) {
                real_vector2d probe_dir;
                uint8_t got_probe_dir = 0;

                switch (chosen_axis) {
                case 0:
                    probe_dir.i = a->facing.i;
                    probe_dir.j = a->facing.j;
                    got_probe_dir = 1;
                    break;
                case 1:
                    probe_dir.i = -a->facing.i;
                    probe_dir.j = -a->facing.j;
                    got_probe_dir = 1;
                    break;
                case 2:
                    probe_dir.i = a->facing.j;
                    probe_dir.j = -a->facing.i;
                    got_probe_dir = 1;
                    break;
                case 3:
                    probe_dir.i = -a->facing.j;
                    probe_dir.j = a->facing.i;
                    got_probe_dir = 1;
                    break;
                default:
                    break;
                }
                if (got_probe_dir && 0.0f < vector2d_normalize_with_length(&probe_dir)) {
                    real_point3d probe_point;
                    uint8_t probe_result[28];
                    uint8_t hit;

                    probe_point.z = a->body_position.z;
                    probe_point.x = probe_dir.i * 0.4f + a->body_position.x;
                    probe_point.y = probe_dir.j * 0.4f + a->body_position.y;
                    hit = path_find_trace_bsp_boundary((uint32_t)global_structure_bsp, a->ignores_glass, &a->body_position,
                                       lead_target_index, &probe_point, 0xffffffffu, probe_result);
                    if (hit != 0 && max_turn_cos <= 0.95f) {
                        max_turn_cos = 0.95f;
                    }
                }
            }
        }
        if (dot_facing <= max_turn_cos) {
            take_max_turn_path = 0;
        } else {
            take_max_turn_path = 1;
        }
    } else {
        take_max_turn_path = 1;
    }

    {
        real accuracy_scale = actor_compute_accuracy_scale(actor_index);
        real desired_len_sq = desired_direction->k * desired_direction->k +
                              desired_direction->j * desired_direction->j +
                              desired_direction->i * desired_direction->i;
        real turn_limit = param_9;
        float stop_distance = 0.0f;

        *out_flag_506 = (uint8_t)(desired_len_sq < accuracy_scale * accuracy_scale);
        // 0x418653: EBX = &max_turn_cos (overwritten with the accelerate-then-stop distance), EDI = the actor
        // index argument slot, reused for the plain stop distance.
        actor_movement_get_stopping_distances(actor_index, &max_turn_cos, &stop_distance);

        if (a->active_movement.cancelled == 0 && desired_len_sq < stop_distance * stop_distance) {
            real dist = (real)sqrt((double)desired_len_sq);

            if (dist <= max_turn_cos + 0.05f || stop_distance <= max_turn_cos) {
                turn_limit = 0.0f;
            } else {
                real t = (dist - max_turn_cos) / (stop_distance - max_turn_cos);
                if (t < turn_limit) {
                    turn_limit = t;
                }
            }
        }

        heading.i = global_origin3d_pointer->i;
        heading.j = global_origin3d_pointer->j;
        heading.k = global_origin3d_pointer->k;

        if (take_max_turn_path) {
            switch (chosen_axis) {
            case 0:
                heading.i = 1.0f;
                heading.j = 0.0f;
                heading.k = 0.0f;
                break;
            case 1:
                heading.i = -1.0f;
                heading.j = 0.0f;
                heading.k = 0.0f;
                break;
            case 2:
                heading.i = 0.0f;
                heading.j = -1.0f;
                heading.k = 0.0f;
                break;
            case 3:
                heading.i = 0.0f;
                heading.j = 1.0f;
                heading.k = 0.0f;
                break;
            case 4:
                heading = rotated_result;
                break;
            default:
                break;
            }
            heading.i *= turn_limit;
            heading.j *= turn_limit;
            heading.k *= turn_limit;
            *out_flag_507 = 0;
        } else {
            a->unknown_591 = 1;
            *out_flag_507 = 1;
        }

        if (0.0f < param_5 || 0.0f < param_7) {
            real target_angle;
            real angle_delta;

            if (dot_facing < 1.0f) {
                if ((dot_facing < -1.0f) == (dot_facing == -1.0f)) {
                    target_angle = (real)acos((double)dot_facing);
                } else {
                    target_angle = 3.1415927f;
                }
            } else {
                target_angle = 0.0f;
            }

            angle_delta = target_angle;
            if (0.0f < param_5) {
                real speed_b = param_8;
                real limited = param_5 * param_8;
                real speed_cap = param_5;

                if (1.0f < speed_b) {
                    if (1.5f < speed_b) {
                        speed_b = 1.5f;
                    }
                    speed_cap = speed_b * param_5;
                }
                if (target_angle * 3.0f <= limited) {
                    limited = target_angle * 3.0f;
                }
                if (limited <= target_angle) {
                    if (speed_cap < target_angle) {
                        angle_delta = speed_cap;
                    }
                } else {
                    angle_delta = limited;
                }
            }

            if (angle_delta <= a->unknown_594[0]) {
                if (0.0f < a->unknown_594[0]) {
                    if (param_6 <= angle_delta) {
                        angle_delta = a->unknown_594[0];
                    } else {
                        a->unknown_594[0] = 0.0f;
                    }
                }
            } else if (a->unknown_591 != 0 && param_6 < angle_delta) {
                if (angle_delta <= param_7) {
                    a->unknown_594[0] = angle_delta;
                } else {
                    a->unknown_594[0] = param_7;
                }
            }

            {
                real step = angle_delta - target_angle;
                if (0.0001f < fabs((double)step)) {
                    real_vector3d axis;
                    // UNSURE: only the third (ECX/b) operand is visible at this call site
                    // (facing); 'a' is inferred to be the in-progress heading, by analogy
                    // with the vector3d_cross_product(out, a, b) convention established in
                    // src/ai/actor_get_body_axis_vector.c.
                    vector3d_cross_product(&axis, &heading, &a->facing);
                    if (0.0f < vector3d_normalize_with_length(&axis)) {
                        vector3d_rotate_about_axis(&heading, &axis, (real)sin((double)step), (real)cos((double)step));
                    }
                }
            }
        }
    }

    *out_axis = (int16_t)chosen_axis;
    *out_direction = aim_dir;
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
