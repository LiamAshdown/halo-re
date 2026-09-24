// vehicle_update  (Ghidra: already named vehicle_update)
// address 0x570ee0, size 2531 bytes
// name confidence: 0.85 (cea-pdb hint 'vehicle_update' confirmed by decompilation)
// rewrite confidence: 0.15 -- by far the largest and most register-dense function in this
//   batch. Fields with a clear struct match are named; everything else (the turret/steering
//   dispatch buffers, the "~blur" impact-damage table, the 0x746f9c table) is preserved as raw
//   offsets with UNSURE notes, and the seven per-vehicle-type physics functions this dispatches
//   to (0x572b60..0x574780/0x507840) are declared with only the argument counts Ghidra shows at
//   this call site -- their own files (this batch) may refine the signatures.
// evidence: types/units.h vehicle_data (flags 0x4cc as both a byte and a uint16, network_update_tick
//   0x5ac); types/objects.h object.parent_object (0x11c), .flags (0x010), .velocity (0x068),
//   .angular_velocity (0x08c), .forward/.up (0x074/0x080), .first_child_object (0x118),
//   .next_object (0x114); types/units.h unit_data.control_flags (0x208), .throttle (0x278),
//   .desired_facing_vector (0x224), .unknown_338/.unknown_33c (0x338/0x33c); types/tags.h
//   Vehicle.vehicle_flags (0x2f0), .maximum_forward_speed (0x2f8), .maximum_left_turn/
//   .maximum_right_turn (0x308/0x30c), .wheel_circumference (0x310), .turn_rate (0x314),
//   .blur_speed (0x318), .maximum_left_slide/.maximum_right_slide (0x330/0x334),
//   .minimum_flipping_angular_velocity/.maximum_flipping_angular_velocity (0x340/0x344), and
//   the animation_graph TagDependency inherited from Object (tag_id at absolute 0x44); callees
//   unit_get_recently_updated_flag (0x570c80), unit_has_child_of_type5 (0x570d70),
//   unit_set_facing_from_index_table (0x570de0), unit_update_recoil_decay (0x574780),
//   object_set_permutation_by_name (0x4f6c60, established 4-argument form),
//   object_apply_damage.
// register convention: vehicle object index in EAX (param_1).
//   // blam-cc: EAX -> object_index
// UNSURE: both vector3d_cross_product calls in the flipping-turn branch are followed, in
//   Ghidra's own decompile, by code that reuses the *already-computed* "V = up x forward"
//   vector (fVar17/fVar10/fVar11) rather than either call's result -- i.e. both calls are
//   reproduced for fidelity but their outputs go unused here, exactly as decompiled.
// UNSURE: vector3d_distance's two point arguments are register-only; guessed as the object's
//   current position against unit_data.unknown_34c (the same "cached reference point" field
//   0x56e820 maintains), since no other per-vehicle "last synced position" field is documented.
// UNSURE: DAT_00746f9c+0x10/+0x14 (a two-float table used to bias turning_velocity, gated on a
//   vehicle-type bitmask of 0x28 = types 3 and 5) and globals_tag_data+0x18c+0x38/+0x8c (an
//   "excess speed impact" damage effect and threshold) are not named in any header available to
//   this module.
// UNSURE: the seven vehicle-type dispatch targets (FUN_00572b60.. object_physics_tick) and the small
//   foreign helpers physics_scalar_move_toward_target/physics_scalar_step_to_target_clamped/unit_any_flagged_seat_occupied are declared with exactly the
//   argument counts visible at their call sites here.
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)
// reconciled: R25 damage_data.unknown_4c -> material_type (int16 collision material of the damaged surface, 0xffff = none; indexes DamageEffect +0x200)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "hs.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern int32_t game_connection_role; // 0x00719720
extern int32_t DAT_006f1cf0;         // the vehicle network update period (types/units.h)
extern game_time_globals *game_time; // 0x006f1d6c
extern uint8_t unit_updates_suppressed; // 0x0071c419
extern uint8_t *global_structure_bsp;  // 0x00746f9c, UNSURE
extern uint8_t *globals_tag_data;       // 0x00746fa0

extern double atan2(double y, double x); // fpatan
extern float fabsf(float x);

extern real vector3d_distance(real_point3d *a, real_point3d *b); // 0x4088b0
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand,
                                    real_vector3d *stack_operand); // 0x4052c0
extern uint8_t unit_get_recently_updated_flag(uint32_t object_index);       // 0x570c80, this batch
extern uint8_t unit_has_child_of_type5(uint32_t unit_index);                // 0x570d70, this batch
extern void unit_set_facing_from_index_table(uint32_t object_index);        // 0x570de0, this batch
extern void unit_update_recoil_decay(uint32_t object_index);                  // 0x574780, this batch
extern uint32_t unit_update_marker_traction_effects(uint32_t object_index);                           // 0x575170, this batch
extern void unit_update_steering_deviation_effects(uint32_t unit_index);                              // 0x574f30, this batch
  // real signature (unit_update_steering_deviation_effects.c): void unit_update_steering_deviation_effects(uint32_t unit_index, real_vector3d *reference_direction, uint8_t *contact_points); Ghidra recovered 1 of 3 args at this call site
extern void unit_update_marker_skid_effects(void *transform);                                  // 0x575460, this batch
  // real signature (unit_update_marker_skid_effects.c): void unit_update_marker_skid_effects(uint32_t unit_index, uint8_t *contact_points); Ghidra recovered 1 of 2 args at this call site
extern void unit_update_ground_contact_counter(void);                                             // 0x575640, this batch, UNSURE args
  // real signature (unit_update_ground_contact_counter.c): void unit_update_ground_contact_counter(uint32_t unit_index, uint8_t *contact_points); Ghidra recovered 0 of 2 args at this call site
extern void unit_update_animation_state_machine(uint32_t unit_index);                              // 0x565420, unit_update_animation_state_machine
  // real signature (unit_update_animation_state_machine.c): uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); Ghidra recovered 1 of 2 args at this call site
extern int8_t unit_any_flagged_seat_occupied(void); // 0x56cc80, UNSURE: zero visible args
  // real signature (unit_any_flagged_seat_occupied.c): uint8_t unit_any_flagged_seat_occupied(uint32_t unit_index); Ghidra recovered 0 of 1 args at this call site
extern void physics_scalar_move_toward_target(uint32_t param_1, float angle, float rate); // 0x50b2f0, UNSURE signature
extern void physics_scalar_step_to_target_clamped(float value, float scale); // 0x50b460, UNSURE signature
extern void object_set_permutation_by_name(uint32_t object_index, char *name, int16_t region_filter,
                                           char use_matched_index); // 0x4f6c60
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t param_3,
                                 int16_t param_4, int16_t param_5, uint32_t param_6); // 0x4ee5e0

extern void vehicle_calculate_turret_controls(uint32_t unit_index, void *param_2);              // 0x572b60, this batch
extern void vehicle_calculate_steering_wheel_controls(uint32_t unit_index, void *param_2);              // 0x572cd0, this batch
extern void vehicle_calculate_lean_controls(uint32_t unit_index, void *param_2);              // 0x572df0, this batch
extern void vehicle_calculate_ground_lean_controls(uint32_t unit_index, uint8_t *out_transform);      // 0x573100, this batch
extern void vehicle_calculate_wing_flex_controls(uint32_t unit_index, float angle, void *scratch3072,
                          uint8_t *out_transform);                                  // 0x5734d0, this batch
extern void vehicle_calculate_mounted_controls_dispatch(uint32_t unit_index);    // 0x573ee0, this batch
extern void object_physics_tick(uint32_t unit_index, uint32_t param_2, void *transform,
                          uint32_t param_4, uint32_t param_5);                    // 0x507840, UNSURE signature

// Per-tick update for vehicle-type units: resyncs position/orientation over the network when
// stale, zeroes velocity while parentless, tracks the "braking" and "over blur speed" flags,
// computes the flipping angular-velocity clamp and turret-limit lean, dispatches to the
// per-vehicle-type control/animation calculation, applies excess-speed impact damage to
// attached children, drives the animation state machine, and updates the "~blur" motion
// permutation.
uint32_t vehicle_update(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint8_t scratch_transform[9724]; // local_2608, UNSURE exact shape
    uint8_t scratch_3072[3072];      // local_3208, UNSURE exact shape
    float aim_angle;
    real_vector3d v; // up x forward (component-wise); reused by the flipping-turn branch below

    if (game_connection_role == 2 && vehicle->network_update_tick != -1 && DAT_006f1cf0 != 0 &&
        (int32_t)(vehicle->network_update_tick + DAT_006f1cf0) <= game_time->game_time) {
        real vect_dist = vector3d_distance(&obj->position, (real_point3d *)&unit->unknown_34c); // UNSURE args
        if (vect_dist > 1.5f && unit_get_recently_updated_flag(object_index) == 1 &&
            unit_has_child_of_type5(object_index) == 0) {
            unit_set_facing_from_index_table(object_index);
        }
        vehicle->network_update_tick = game_time->game_time;
    }

    if (obj->parent_object == k_datum_index_none) {
        obj->angular_velocity.i = 0.0f;
        obj->angular_velocity.j = 0.0f;
        obj->angular_velocity.k = 0.0f;
        obj->velocity.i = 0.0f;
        obj->velocity.j = 0.0f;
        obj->velocity.k = 0.0f;
        obj->flags &= ~0x20u;
        goto after_physics;
    }

    if ((unit->control_flags & _unit_control_flag_crouch) == 0) {
        vehicle->flags &= ~4;
    } else {
        vehicle->flags |= 4;
    }

    if ((unit->control_flags & _unit_control_flag_jump) != 0 ||
        ((tag->vehicle_flags & 0x10) != 0 &&
         ((unit->throttle.i > 0.0f && vehicle->forward_velocity < 0.0f) ||
          (unit->throttle.i < 0.0f && vehicle->forward_velocity > 0.0f)))) {
        vehicle->flags |= 8;
    } else {
        vehicle->flags &= ~8;
    }

    {
        // V = up x forward (component-wise, matching the Ghidra arithmetic exactly), used as a
        // reference axis so the angle between "forward" and "desired_facing_vector" comes out
        // signed. The flipping-turn branch below reuses this same vector as its rotation axis.
        v.i = obj->forward.k * obj->up.j - obj->up.k * obj->forward.j;
        v.j = obj->up.k * obj->forward.i - obj->forward.k * obj->up.i;
        v.k = obj->up.i * obj->forward.j - obj->forward.i * obj->up.j;

        aim_angle = (float)atan2(
            (double)(v.i * unit->desired_facing_vector.i + v.k * unit->desired_facing_vector.k +
                     v.j * unit->desired_facing_vector.j),
            (double)(unit->desired_facing_vector.i * obj->forward.i +
                     unit->desired_facing_vector.j * obj->forward.j +
                     unit->desired_facing_vector.k * obj->forward.k));
    }

    if ((obj->network_role == 2 || obj->network_role == 1) && *((int8_t *)obj + 0x18) == 1) {
        unit_any_flagged_seat_occupied();
    }

    if (((vehicle->flags & 0x10) == 0) || (vehicle->unknown_4d1 == 0) ||
        (vehicle->unknown_4d2 > 0x1d) ||
        ((vehicle->turning_velocity < 0.9f) == (vehicle->turning_velocity == 0.9f))) {
        vehicle->unknown_4d2 = 0;
        vehicle->unknown_4d1 = 0;
        vehicle->flags &= ~0x10;
    } else {
        float sign = (vehicle->unknown_4d1 == 2 || vehicle->unknown_4d1 == 4) ? 0.3f : -0.3f;
        real_vector3d axis;

        if (vehicle->unknown_4d1 == 4 || vehicle->unknown_4d1 == 3) {
            real_vector3d unused_out;
            vector3d_cross_product(&unused_out, &obj->up, &obj->forward); // UNSURE: result unused, see header
            axis = v; // reuses the up x forward vector computed above
        } else {
            axis = obj->forward;
        }

        {
            float clamp = vehicle->turning_velocity * -2.0f;
            if (tag->minimum_flipping_angular_velocity <= clamp) {
                if (tag->maximum_flipping_angular_velocity < clamp) {
                    clamp = tag->maximum_flipping_angular_velocity;
                }
            } else {
                clamp = tag->minimum_flipping_angular_velocity;
            }
            clamp *= sign;
            obj->flags &= ~0x20u;

            if (vehicle->unknown_4d1 == 2 || vehicle->unknown_4d1 == 1) {
                real_vector3d unused_out;
                float f = -obj->forward.k;
                vector3d_cross_product(&unused_out, &obj->forward, &obj->up); // UNSURE: result unused, see header
                axis.i = v.i * f + axis.i;
                axis.j = v.j * f + axis.j;
                axis.k = f * v.k + axis.k;
            }

            obj->angular_velocity.i = axis.i * clamp;
            obj->angular_velocity.j = axis.j * clamp;
            obj->angular_velocity.k = axis.k * clamp;

            if (tag->vehicle_type == 0) {
                // Reproject the object's own velocity fully onto its forward direction (no
                // sideways slip for this vehicle type).
                float along = obj->velocity.i * obj->forward.i + obj->velocity.j * obj->forward.j +
                              obj->velocity.k * obj->forward.k;
                obj->velocity.i = along * obj->forward.i;
                obj->velocity.j = along * obj->forward.j;
                obj->velocity.k = along * obj->forward.k;
            } else if (tag->vehicle_type == 5) {
                // Clamp the vertical velocity component to a slow minimum fall rate.
                obj->velocity.k = (obj->velocity.k >= -0.01f) ? -0.01f : obj->velocity.k;
            }
        }
        vehicle->unknown_4d2 += 1;
    }

    if ((vehicle->flags & 8) == 0) {
        physics_scalar_step_to_target_clamped(unit->throttle.i, 1.0f);
        physics_scalar_step_to_target_clamped(unit->throttle.j, 1.0f);
    } else {
        physics_scalar_step_to_target_clamped(0.0f, 1.0f);
    }

    if (tag->vehicle_type == 0) {
        float rate, angle;
        if (vehicle->sideways_velocity == 0.0f) {
            rate = 1.0f; angle = 0.0f;
        } else {
            angle = aim_angle * 0.63661975f;
            if (angle < -1.0f) angle = -1.0f;
            else if (angle > 1.0f) angle = 1.0f;
            angle *= tag->maximum_forward_speed;
            rate = 2.0f;
        }
        physics_scalar_step_to_target_clamped(angle, rate);
    } else {
        float signed_angle = (vehicle->sideways_velocity < 0.0f) ? -aim_angle : aim_angle;
        float limit = tag->maximum_right_turn * 0.017453292f;
        if (limit <= signed_angle) {
            float other = tag->maximum_left_turn * 0.017453292f;
            limit = signed_angle;
            if (other < signed_angle) limit = other;
        }
        physics_scalar_move_toward_target(0, limit, tag->turn_rate * 0.017453292f * 0.033333335f);
    }

    if (*(int32_t *)((uint8_t *)tag + 0x8c) == -1) { // tag->physics.tag_id
        goto recoil_only;
    }

    {
        uint32_t flags = tag->vehicle_flags;
        if ((((flags & 1) != 0 && vehicle->forward_velocity != 0.0f) ||
             ((flags & 2) != 0 && vehicle->turning_velocity != 0.0f) ||
             ((flags & 4) != 0 && unit->unknown_338 != 0.0f) ||
             ((flags & 8) != 0 && unit->unknown_33c != 0.0f) ||
             ((flags & 0x20) != 0 && vehicle->sideways_velocity != 0.0f))) {
            obj->flags &= ~0x20u;
        }

        if (*(int32_t *)((uint8_t *)tag + 0x8c) == -1 || (obj->flags & 0x20) != 0) {
            goto recoil_only;
        }

        switch (tag->vehicle_type) {
        case 0: vehicle_calculate_turret_controls(object_index, scratch_transform); break;
        case 1: vehicle_calculate_steering_wheel_controls(object_index, scratch_transform); break;
        case 2: vehicle_calculate_lean_controls(object_index, scratch_transform); break;
        case 3: vehicle_calculate_ground_lean_controls(object_index, scratch_transform); break;
        case 4: vehicle_calculate_wing_flex_controls(object_index, aim_angle, scratch_3072, scratch_transform); break;
        case 5: vehicle_calculate_mounted_controls_dispatch(object_index); break;
        case 6: object_physics_tick(object_index, 0, scratch_transform, 0, 0); break;
        }

        if (unit_updates_suppressed == 0) {
            unit_update_marker_skid_effects(scratch_transform);
        }

        {
            uint8_t had_traction = unit_update_marker_traction_effects(object_index);
            if (had_traction == 0 && unit_updates_suppressed == 0) {
                unit_update_steering_deviation_effects(object_index);
            }
        }
        unit_update_ground_contact_counter();

        if ((obj->flags & 0x20) != 0) {
            vehicle->unknown_4ce = 0xf;
        }

        if ((obj->flags & 0x1000000) == 0 && (1 << (tag->vehicle_type & 0x1f) & 0x28) != 0) {
            float lo = *(float *)(global_structure_bsp + 0x10);
            float hi = *(float *)(global_structure_bsp + 0x14);

            if (lo != 0.0f && unit->unknown_338 < lo) {
                vehicle->turning_velocity = ((lo - unit->unknown_338) * 0.015625f -
                                             vehicle->turning_velocity * 0.0625f) * unit->unknown_338 +
                                            vehicle->turning_velocity;
            }
            if (hi != 0.0f && hi < unit->unknown_338) {
                vehicle->turning_velocity = vehicle->turning_velocity -
                    (vehicle->turning_velocity * 0.0625f + (unit->unknown_338 - hi) * 0.015625f) * unit->unknown_338;
            }
        }
        goto skip_recoil_label;
    }

recoil_only:
    if (vehicle->unknown_4ce > 0) {
        unit_update_recoil_decay(object_index);
        unit_update_marker_traction_effects(object_index);
    }

skip_recoil_label:
    if ((tag->vehicle_flags & 0x40) != 0 && unit_updates_suppressed == 0) {
        uint8_t *impact_table = *(uint8_t **)(globals_tag_data + 0x18c);
        if (vehicle->turning_velocity < -*(float *)(impact_table + 0x8c)) {
            datum_index child = obj->first_child_object;
            while (child != k_datum_index_none) {
                object *child_obj = ((object_header *)object_data->data)[child & 0xffff].data;
                damage_data dd = {0};

                dd.damage_effect_tag = *(datum_index *)(impact_table + 0x38);
                dd.team_index = -1;
                dd.responsible_player = k_datum_index_none;
                dd.responsible_object = k_datum_index_none;
                dd.random_blend = 1.0f;
                dd.multiplier = 1.0f;
                dd.material_type = -1;
                object_apply_damage(&dd, child, -1, -1, -1, 0);

                child = child_obj->next_object;
            }
        }
    }

after_physics:
    if (*(int32_t *)((uint8_t *)tag + 0x44) != -1) { // tag->animation_graph.tag_id
        unit_update_animation_state_machine(object_index);
    }

    {
        uint8_t over_blur = fabsf(vehicle->forward_velocity) > tag->blur_speed;
        if (over_blur != ((vehicle->flags & 1) != 0)) {
            object_set_permutation_by_name(object_index, "~blur", -1, over_blur);
            if (over_blur) {
                vehicle->flags |= 1;
            } else {
                vehicle->flags &= ~1;
            }
        }
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x570ee0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 vehicle_update(uint param_1)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  uint *puVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  bool bVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  char cVar13;
  int iVar14;
  undefined4 *puVar15;
  float10 fVar16;
  float fVar17;
  undefined4 uVar18;
  float local_3284;
  float local_3278;
  float local_3274;
  float local_3270;
  undefined4 local_3260 [4];
  undefined2 local_3250;
  undefined2 local_3248;
  undefined4 local_3220;
  undefined4 local_321c;
  undefined2 local_3214;
  undefined1 local_3208 [3072];
  undefined1 local_2608 [9724];
  undefined4 uStack_c;

  uStack_c = 0x570ef0;
  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar5 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((((DAT_00719720 == 2) && (puVar4[0x16b] != 0xffffffff)) && (DAT_006f1cf0 != 0)) &&
     ((int)(puVar4[0x16b] + DAT_006f1cf0) <= *(int *)(DAT_006f1d6c + 0xc))) {
    fVar16 = (float10)vector3d_distance();
    if ((((float10)1.5 < fVar16) && (cVar13 = FUN_00570c80(), cVar13 == '\x01')) &&
       (cVar13 = FUN_00570d70(), cVar13 == '\0')) {
      FUN_00570de0(param_1);
    }
    puVar4[0x16b] = *(uint *)(DAT_006f1d6c + 0xc);
  }
  if (puVar4[0x47] != 0xffffffff) {
    puVar4[0x23] = 0;
    puVar4[0x24] = 0;
    puVar4[0x25] = 0;
    puVar4[0x1a] = 0;
    puVar4[0x1b] = 0;
    puVar4[0x1c] = 0;
    puVar4[4] = puVar4[4] & 0xffffffdf;
    goto LAB_00571828;
  }
  if ((puVar4[0x82] & 1) == 0) {
    *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] & 0xfb;
  }
  else {
    *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] | 4;
  }
  if (((puVar4[0x82] & 2) != 0) ||
     (((*(byte *)(iVar5 + 0x2f0) & 0x10) != 0 &&
      (((0.0 < (float)puVar4[0x9e] && ((float)puVar4[0x135] < 0.0)) ||
       (((float)puVar4[0x9e] < 0.0 && (0.0 < (float)puVar4[0x135])))))))) {
    *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] | 8;
  }
  else {
    *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] & 0xf7;
  }
  pfVar1 = (float *)(puVar4 + 0x1d);
  fVar17 = (float)puVar4[0x1f] * (float)puVar4[0x21] - (float)puVar4[0x22] * (float)puVar4[0x1e];
  fVar10 = (float)puVar4[0x22] * *pfVar1 - (float)puVar4[0x1f] * (float)puVar4[0x20];
  fVar11 = (float)puVar4[0x20] * (float)puVar4[0x1e] - *pfVar1 * (float)puVar4[0x21];
  fVar16 = (float10)fpatan((float10)fVar17 * (float10)(float)puVar4[0x89] +
                           (float10)fVar11 * (float10)(float)puVar4[0x8b] +
                           (float10)fVar10 * (float10)(float)puVar4[0x8a],
                           (float10)(float)puVar4[0x89] * (float10)*pfVar1 +
                           (float10)(float)puVar4[0x8a] * (float10)(float)puVar4[0x1e] +
                           (float10)(float)puVar4[0x8b] * (float10)(float)puVar4[0x1f]);
  fVar2 = (float)fVar16;
  if (((puVar4[1] == 2) || (puVar4[1] == 1)) && ((char)puVar4[6] == '\x01')) {
    FUN_0056cc80();
  }
  if (((((ushort)puVar4[0x133] & 0x10) == 0) ||
      (cVar13 = *(char *)((int)puVar4 + 0x4d1), cVar13 == '\0')) ||
     ((0x1d < *(byte *)((int)puVar4 + 0x4d2) ||
      ((float)puVar4[0x22] < 0.9 == ((float)puVar4[0x22] == 0.9))))) {
    *(undefined1 *)((int)puVar4 + 0x4d2) = 0;
    *(undefined1 *)((int)puVar4 + 0x4d1) = 0;
    *(ushort *)(puVar4 + 0x133) = (ushort)puVar4[0x133] & 0xffef;
  }
  else {
    if ((cVar13 == '\x02') || (cVar13 == '\x04')) {
      fVar12 = 0.3;
    }
    else {
      fVar12 = -0.3;
    }
    if ((cVar13 == '\x04') || (cVar13 == '\x03')) {
      vector3d_cross_product(pfVar1);
      local_3278 = fVar17;
      local_3274 = fVar10;
      local_3270 = fVar11;
    }
    else {
      local_3278 = *pfVar1;
      local_3274 = (float)puVar4[0x1e];
      local_3270 = (float)puVar4[0x1f];
    }
    fVar3 = (float)puVar4[0x22] * -2.0;
    if (*(float *)(iVar5 + 0x340) <= fVar3) {
      if (*(float *)(iVar5 + 0x344) < fVar3) {
        fVar3 = *(float *)(iVar5 + 0x344);
      }
    }
    else {
      fVar3 = *(float *)(iVar5 + 0x340);
    }
    fVar3 = fVar3 * fVar12;
    puVar4[4] = puVar4[4] & 0xffffffdf;
    if ((*(char *)((int)puVar4 + 0x4d1) == '\x02') || (*(char *)((int)puVar4 + 0x4d1) == '\x01')) {
      vector3d_cross_product(pfVar1);
      fVar12 = -(float)puVar4[0x1f];
      local_3278 = fVar17 * fVar12 + local_3278;
      local_3274 = fVar10 * fVar12 + local_3274;
      local_3270 = fVar12 * fVar11 + local_3270;
    }
    puVar4[0x23] = (uint)(local_3278 * fVar3);
    puVar4[0x24] = (uint)(local_3274 * fVar3);
    puVar4[0x25] = (uint)(local_3270 * fVar3);
    if (*(short *)(iVar5 + 0x2f4) == 0) {
      fVar17 = (float)puVar4[0x1a] * *pfVar1 +
               (float)puVar4[0x1e] * (float)puVar4[0x1b] + (float)puVar4[0x1f] * (float)puVar4[0x1c]
      ;
      puVar4[0x1a] = (uint)(fVar17 * *pfVar1);
      puVar4[0x1b] = (uint)(fVar17 * (float)puVar4[0x1e]);
      fVar17 = fVar17 * (float)puVar4[0x1f];
LAB_005712ad:
      puVar4[0x1c] = (uint)fVar17;
    }
    else if (*(short *)(iVar5 + 0x2f4) == 5) {
      if (-0.01 <= (float)puVar4[0x1c]) {
        fVar17 = -0.01;
      }
      else {
        fVar17 = (float)puVar4[0x1c];
      }
      goto LAB_005712ad;
    }
    *(char *)((int)puVar4 + 0x4d2) = *(char *)((int)puVar4 + 0x4d2) + '\x01';
  }
  if ((puVar4[0x133] & 8) == 0) {
    FUN_0050b460(puVar4[0x9e],0x3f800000);
    FUN_0050b460(puVar4[0x9f],0x3f800000);
  }
  else {
    FUN_0050b460(0,0x3f800000);
  }
  if (*(short *)(iVar5 + 0x2f4) == 0) {
    if ((float)puVar4[0x135] == 0.0) {
      uVar18 = 0x3f800000;
      fVar17 = 0.0;
    }
    else {
      fVar17 = fVar2 * 0.63661975;
      if (-1.0 <= fVar17) {
        if (1.0 < fVar17) {
          fVar17 = 1.0;
        }
      }
      else {
        fVar17 = -1.0;
      }
      fVar17 = fVar17 * *(float *)(iVar5 + 0x2f8);
      uVar18 = 0x40000000;
    }
    FUN_0050b460(fVar17,uVar18);
  }
  else {
    local_3284 = fVar2;
    if ((float)puVar4[0x135] < 0.0) {
      local_3284 = -fVar2;
    }
    fVar17 = *(float *)(iVar5 + 0x30c) * 0.017453292;
    if ((fVar17 <= local_3284) &&
       (fVar10 = *(float *)(iVar5 + 0x308) * 0.017453292, fVar17 = local_3284, fVar10 < local_3284))
    {
      fVar17 = fVar10;
    }
    local_3284 = fVar17;
    FUN_0050b2f0(0,local_3284,*(float *)(iVar5 + 0x314) * 0.017453292 * 0.033333335);
  }
  if (*(int *)(iVar5 + 0x8c) == -1) {
LAB_00571725:
    if (0 < *(short *)((int)puVar4 + 0x4ce)) {
      unit_update_recoil_decay(param_1);
      FUN_00575170(param_1);
    }
  }
  else {
    uVar6 = *(uint *)(iVar5 + 0x2f0);
    if (((((((uVar6 & 1) != 0) && ((float)puVar4[0x135] != 0.0)) ||
          (((uVar6 & 2) != 0 && ((float)puVar4[0x137] != 0.0)))) ||
         (((uVar6 & 4) != 0 && ((float)puVar4[0xce] != 0.0)))) ||
        (((uVar6 & 8) != 0 && ((float)puVar4[0xcf] != 0.0)))) ||
       (((uVar6 & 0x20) != 0 && ((float)puVar4[0x136] != 0.0)))) {
      puVar4[4] = puVar4[4] & 0xffffffdf;
    }
    if ((*(int *)(iVar5 + 0x8c) == -1) || ((puVar4[4] & 0x20) != 0)) goto LAB_00571725;
    switch(*(undefined2 *)(iVar5 + 0x2f4)) {
    case 0:
      FUN_00572b60(param_1,local_2608);
      break;
    case 1:
      FUN_00572cd0(param_1,local_2608);
      break;
    case 2:
      FUN_00572df0(param_1,local_2608);
      break;
    case 3:
      FUN_00573100(param_1,local_2608);
      break;
    case 4:
      FUN_005734d0(param_1,fVar2,local_3208,local_2608);
      break;
    case 5:
      FUN_00573ee0();
      break;
    case 6:
      FUN_00507840(param_1,0,local_2608,0,0);
    }
    if (DAT_0071c419 == '\0') {
      FUN_00575460(local_2608);
    }
    cVar13 = FUN_00575170(param_1);
    if ((cVar13 == '\0') && (DAT_0071c419 == '\0')) {
      FUN_00574f30(param_1);
    }
    FUN_00575640();
    if ((puVar4[4] & 0x20) != 0) {
      *(undefined2 *)((int)puVar4 + 0x4ce) = 0xf;
    }
    if (((puVar4[4] & 0x1000000) == 0) && ((1 << (*(byte *)(iVar5 + 0x2f4) & 0x1f) & 0x28U) != 0)) {
      fVar2 = *(float *)(DAT_00746f9c + 0x10);
      fVar17 = *(float *)(DAT_00746f9c + 0x14);
      if ((fVar2 != 0.0) && ((float)puVar4[0x19] < fVar2)) {
        puVar4[0x1c] = (uint)(((fVar2 - (float)puVar4[0x19]) * 0.015625 -
                              (float)puVar4[0x1c] * 0.0625) * (float)puVar4[0xce] +
                             (float)puVar4[0x1c]);
      }
      if ((fVar17 != 0.0) && (fVar17 < (float)puVar4[0x19])) {
        puVar4[0x1c] = (uint)((float)puVar4[0x1c] -
                             ((float)puVar4[0x1c] * 0.0625 +
                             ((float)puVar4[0x19] - fVar17) * 0.015625) * (float)puVar4[0xce]);
      }
    }
  }
  if ((((*(byte *)(iVar5 + 0x2f0) & 0x40) != 0) && (DAT_0071c419 == '\0')) &&
     (iVar7 = *(int *)(DAT_00746fa0 + 0x18c), (float)puVar4[0x1c] < -*(float *)(iVar7 + 0x8c))) {
    uVar6 = puVar4[0x46];
    while (uVar6 != 0xffffffff) {
      iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
      puVar15 = local_3260;
      for (iVar14 = 0x15; iVar14 != 0; iVar14 = iVar14 + -1) {
        *puVar15 = 0;
        puVar15 = puVar15 + 1;
      }
      local_3260[0] = *(undefined4 *)(iVar7 + 0x38);
      local_3214 = 0xffff;
      local_3260[2] = 0xffffffff;
      local_3260[3] = 0xffffffff;
      local_3250 = 0xffff;
      local_3248 = 0xffff;
      local_3220 = 0x3f800000;
      local_321c = 0x3f800000;
      object_apply_damage(local_3260,uVar6,0xffffffff,0xffffffff,0xffffffff,0);
      uVar6 = *(uint *)(iVar8 + 0x114);
    }
  }
LAB_00571828:
  if (*(int *)(iVar5 + 0x44) != -1) {
    FUN_00565420(param_1);
  }
  bVar9 = *(float *)(iVar5 + 0x318) < ABS((float)puVar4[0x135]) !=
          (*(float *)(iVar5 + 0x318) == ABS((float)puVar4[0x135]));
  if (bVar9 != (bool)((byte)puVar4[0x133] & 1)) {
    object_set_permutation_by_name("~blur",0xffffffff,bVar9);
    if (bVar9) {
      *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] | 1;
      return 1;
    }
    *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] & 0xfe;
    return 1;
  }
  return 1;
}
#endif
