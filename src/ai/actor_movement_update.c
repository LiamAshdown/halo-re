// actor_movement_update  (Ghidra: already named)
// address 0x416790, size 3074 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/ai_functions.md "Per-tick actor movement decision routine that selects
//   a movement mode and desired direction, then applies obstacle-aware steering to it."; this
//   is the sole caller of both actor_movement_apply_steering (0x4180c0) and
//   actor_movement_choose_avoidance_direction (0x4193d0), and the writer of actor.unknown_6dc
//   (the movement-style selector both of those read).
// register convention: a plain single stack parameter, the actor index.
//   // blam-cc: stack -> actor_index
//
// This file was re-derived from the disassembly (objdump -d -M intel 0x416790..0x417390) after
// the first pass, written from the Ghidra pseudocode alone, turned out not to compile and to
// carry several semantic errors. The stack-slot map below is what the rewrite is built on;
// with esp at its post-prologue value E (entry esp - 0x4c - 0x10) the frame is:
//   [E+0x10] byte  sidestep_mode      -> the hidden ECX argument of actor_movement_apply_steering
//   [E+0x11] byte  face_along_heading -> bVar20 in the Ghidra listing
//   [E+0x12] byte  vehicle_stuck      -> bVar7
//   [E+0x13] byte  clear_recognition  -> bVar6
//   [E+0x14] float avoid_threshold    -> local_48
//   [E+0x1c] byte  want_avoid_check   -> local_40
//   [E+0x20] dword avoidance scale out-parameter, then reloaded with actor+0x42e
//                                      -> the hidden EAX argument of actor_movement_apply_steering
//   [E+0x24] float throttle_maximum   -> local_38 (seeded 1.0)
//   [E+0x28] float avoidance_scale    -> local_34
//   [E+0x2c] float oversteer_max      -> local_30
//   [E+0x30] float oversteer_min      -> local_2c
//   [E+0x34] float steering_maximum   -> local_28
//   [E+0x38] byte  order_failed       -> local_24
//
// Four real decompiler errors were found and are fixed here. Each was also an error in the
// previous draft of this file, which trusted the pseudocode:
//  1. Ghidra drops [E+0x10] entirely, because it is the ECX register argument of the callee.
//     It is a genuine local, cleared at 0x4167fc and set at 0x416c86 and 0x416f6a. The previous
//     draft modelled the ECX input of actor_movement_apply_steering as actor.flying; the
//     disassembly says it is this local. See the matching UNSURE in
//     src/ai/actor_movement_apply_steering.c.
//  2. The EAX register argument of that same call is [E+0x20], which at 0x416b40 is loaded with
//     the 16-bit actor+0x42e and is zeroed outright in the failed-vehicle-direction branch at
//     0x416c7e. Ghidra renders that load as the dead local_3c = CONCAT22(...) artefact, because
//     the same slot earlier received the avoidance sampler scale out-parameter. The previous
//     draft passed actor.unknown_50a instead, which is an output of that call, not an input.
//  3. The flipped-vehicle recovery direction at 0x416cee is a *three*-component normalize of
//     (object.up.i, object.up.j, 0): 0x416d0c stores an explicit 0.0 into the third slot and
//     0x416d14 calls vector3d_normalize_with_length (0x401990). The previous draft called the
//     2D variant (0x4018e0) on a real_vector2d, which is a different function.
//  4. The "is the actor already carrying a unit" test at 0x417131 reads actor.unit_index
//     (+0x18), not actor.unknown_0c, and hands it to unit_is_in_busy_animation_state in ECX.
//
// UNSURE:
//  - actor.movement_context is the movement-context selector (below 1 on foot, 1 and up riding a
//    unit). Only the literal comparisons are transcribed; nothing here says what the values mean.
//  - actor+0x42c / 0x42e / 0x426..0x429 / 0x430 / 0x434..0x450 sit in the order-bookkeeping
//    range of the actor and are named only by offset.
//  - actor.movement_timer (+0x4a0) is declared int32_t in types/ai.h but 0x416b47 does
//    fld [ebp+0x4a0] and compares it against Actor.stationary_movement_dist, so it is read as a
//    float here (actor_movement_action_resolve already writes it as one). Left as a typed
//    reinterpretation rather than changing a header field other files assign 0 to.
//  - actor.target_unit_index (+0x270) is indexed into prop_data at stride 0x138 here, i.e. it
//    holds a prop handle, not a unit handle. The header name is kept for consistency with the
//    twenty other files that use it; see src/ai/README.md.
//  - unit_try_ready_weapon_variant (units module) takes the unit handle in ESI and the 2D facing in EDI;
//    unit_is_in_busy_animation_state takes the unit handle in ECX. Both read off these call sites only.
//  - The three dwords copied to actor+0x6ec..0x6f4 at the tail are the secondary-action record
//    (actor+0x418..0x420) saved for the next tick; types/ai.h has no field wider than the int16
//    at 0x6ec, so the copy is written through byte offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern const real_vector3d *global_origin3d_pointer;   // 0x00696714
extern const real_vector2d *global_forward2d_pointer; // 0x006966e8

extern double sin(double x); // FSIN
extern double cos(double x); // FCOS
extern double sqrt(double x);

extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX -> v
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX -> v
extern void vector3d_rotate_about_axis(real_vector3d *v, const real_vector3d *axis,
                                       real sin_angle, real cos_angle); // 0x4cd820, EAX->v, ECX->axis

extern void actor_movement_choose_avoidance_direction(datum_index actor_index,
                                                      const real_vector3d *desired_direction,
                                                      real_vector3d *out_direction,
                                                      float *out_scale); // 0x4193d0
extern void actor_movement_apply_steering(
    int16_t cached_axis, uint8_t keep_z,
    datum_index actor_index, uint8_t want_avoid_check, float avoid_threshold, uint8_t order_failed,
    float steering_maximum, float oversteer_min, float oversteer_max, float avoidance_scale,
    float throttle_maximum,
    real_vector3d *desired_direction, real_vector3d *out_direction, int16_t *out_axis,
    real_vector3d *out_heading, uint8_t *out_flag_507, uint8_t *out_flag_506); // 0x4180c0

extern void actor_clear_recognition_history(datum_index actor_index, uint8_t keep_when_typed); // 0x414140, EAX -> actor_index
extern uint8_t actor_queue_secondary_action(datum_index actor_index, int16_t action,
                                            uint32_t payload[2]); // 0x417a60, EAX -> actor_index
extern uint8_t actor_action_has_queued_secondary(datum_index actor_index); // 0x417b70, EAX -> actor_index
// Sets actor.flags bit 0x2, the "no movement order this tick" marker; EAX -> actor_index.
extern void actor_set_flag_bit1(datum_index actor_index); // 0x42a5b0, not yet rewritten (this module)
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index,
                                       datum_index object_a, int32_t param_d,
                                       datum_index object_b, datum_index object_c,
                                       uint32_t *param_g);
// 0x42d340, not yet rewritten (this module). Always seven stack arguments: every call
// site in the binary cleans up 0x1c bytes, so the shorter forms Ghidra recovers at some
// sites are artefacts, not a reduced-arity overload.
// Units-module callees, signatures read off these call sites only (see UNSURE above).
extern uint8_t unit_try_ready_weapon_variant(datum_index unit_index, const real_vector2d *facing); // 0x569b30, ESI, EDI
extern uint8_t unit_is_in_busy_animation_state(datum_index actor_index);                              // 0x569c90, ECX
extern uint8_t unit_get_average_active_marker_direction(datum_index unit_index, real_vector3d *out_direction); // 0x575e30

// blam-cc: stack -> actor_index
void actor_movement_update(datum_index actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffffu];
    uint8_t *actor_base = (uint8_t *)a;
    Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;

    uint8_t sidestep_mode = 0;      // [E+0x10], the hidden ECX argument of the steering call
    uint8_t face_along_heading = 0; // [E+0x11]
    uint8_t vehicle_stuck = 0;      // [E+0x12]
    uint8_t clear_recognition = 0;  // [E+0x13]
    float avoid_threshold = 0.0f;   // [E+0x14]
    uint8_t want_avoid_check = 0;   // [E+0x1c]
    int16_t cached_axis;            // [E+0x20] once it stops being the avoidance scale
    float throttle_maximum = 1.0f;  // [E+0x24]
    float avoidance_scale = 0.0f;   // [E+0x28]
    float oversteer_max = 0.0f;     // [E+0x2c]
    float oversteer_min = 0.0f;     // [E+0x30]
    float steering_maximum = 0.0f;  // [E+0x34]
    uint8_t order_failed = 0;       // [E+0x38]

    uint8_t movement_mode;          // bVar16 in the Ghidra listing, held in BL throughout
    int16_t movement_style;         // sVar11
    int16_t context;

    a->position_cache_a = *(const real_point3d *)&a->facing;

    a->unknown_591 = 0;
    actor_base[0x58d] = 1;
    actor_base[0x58e] = 1;

    if (a->explicit_steering_set != 0) {
        // An explicit steering direction was handed to the actor: use it verbatim and reset the
        // avoidance filter so the next sampled direction starts from the origin.
        a->desired_direction = *(const real_point3d *)&a->explicit_steering_direction;
        a->desired_direction_valid = 1;
        actor_base[0x58d] = 0;
        a->avoidance_direction = *global_origin3d_pointer;
        a->avoidance_scale = 0.0f;
        a->unknown_5ec = 0.0f;
    } else if (a->movement_context == 4) {
        const real_vector3d *desired;
        real_vector3d probe;
        real_vector3d sampled;
        float sampled_scale = 0.0f;
        float blend;
        float keep;

        if (a->desired_direction_valid == 0) {
            probe.i = a->facing.i * 3.0f;
            probe.j = a->facing.j * 3.0f;
            probe.k = a->facing.k * 3.0f;
            desired = &probe;
        } else {
            desired = (const real_vector3d *)&a->desired_direction;
        }
        actor_movement_choose_avoidance_direction(actor_index, desired, &sampled, &sampled_scale);

        // Low-pass filter the sampled direction: react slowly (0.05) while the new direction is
        // no longer than the filtered one, quickly (0.3) when it grows.
        if (sampled.j * sampled.j + sampled.k * sampled.k + sampled.i * sampled.i <=
            a->avoidance_direction.k * a->avoidance_direction.k +
            a->avoidance_direction.j * a->avoidance_direction.j +
            a->avoidance_direction.i * a->avoidance_direction.i) {
            blend = 0.05f;
        } else {
            blend = 0.3f;
        }
        keep = 1.0f - blend;
        a->avoidance_direction.i *= keep;
        a->avoidance_direction.j *= keep;
        a->avoidance_direction.k *= keep;
        a->avoidance_direction.i += sampled.i * blend;
        a->avoidance_direction.j += sampled.j * blend;
        a->avoidance_direction.k += sampled.k * blend;
        if (a->avoidance_direction.k * a->avoidance_direction.k +
            a->avoidance_direction.j * a->avoidance_direction.j +
            a->avoidance_direction.i * a->avoidance_direction.i < 0.0001f) {
            a->avoidance_direction = *global_origin3d_pointer;
        }
        a->unknown_5ec = sampled_scale;
        a->avoidance_scale = blend * sampled_scale + keep * a->avoidance_scale;
        if (a->avoidance_scale < 0.001f) {
            a->avoidance_scale = 0.0f;
        }
        if (a->desired_direction_valid != 0) {
            // Treat the filtered vector as an axis-angle turn: its length is the angle.
            real_vector3d turn = a->avoidance_direction;
            float length_squared = turn.j * turn.j + turn.k * turn.k + turn.i * turn.i;
            if (0.0001f < length_squared) {
                double length = sqrt((double)length_squared);
                float inverse = (float)(1.0 / length);
                turn.i *= inverse;
                turn.j *= inverse;
                turn.k *= inverse;
                vector3d_rotate_about_axis(&turn, &turn, (real)sin(length), (real)cos(length));
            }
            avoidance_scale = a->avoidance_scale;
        }
    }

    // Pick the movement style for this tick, unless the order already forced one.
    movement_style = *(int16_t *)&a->unknown_42b[1]; // 0x42c
    if (movement_style == -1) {
        movement_style = 2;
        if (actor_base[0x429] != 0) {
            movement_style = 4;
        } else if (actor_base[0x428] != 0) {
            movement_style = 3;
        } else if (a->awareness_level == 1) {
            movement_style = 1;
        } else if (a->awareness_level == 2) {
            movement_style = 0;
        } else if (a->awareness_level == 3) {
            movement_style = 2;
        }
    }
    a->unknown_6dc = movement_style;
    cached_axis = *(int16_t *)&a->unknown_42b[3]; // 0x42e, the hidden EAX steering argument

    if (a->movement_action_complete != 0 &&
        actor_def->stationary_movement_dist <= *(float *)&a->movement_timer) {
        movement_mode = actor_base[0x427];
    } else {
        movement_mode = actor_base[0x426];
    }

    context = a->movement_context;
    if (context < 1) {
        // ---- on foot -----------------------------------------------------------------
        if (a->order_committed != 0) {
            a->desired_direction_valid = 0;
            a->unknown_50a = 0;
            actor_base[0x58d] = (uint8_t)((a->type == 0xf || a->unknown_161 != 0) ? 1 : 0);
            actor_base[0x58e] = 0;
            movement_mode = 0;
        } else if (a->secondary_action != -1) {
            a->desired_direction_valid = 0;
            actor_base[0x58d] = 0;
            actor_base[0x58e] = 0;
            movement_mode = 0;
        } else if (a->unknown_6dc == 1) {
            a->desired_direction_valid = 0;
            actor_base[0x58d] = 0;
            actor_base[0x58e] = 0;
            face_along_heading = 1;
            movement_mode = 0;
        } else if (a->unknown_15c != 0 && a->flying == 0) {
            a->desired_direction_valid = 0;
            actor_base[0x58d] = 1;
            movement_mode = 0;
        } else if (a->unknown_6a0 != 0) {
            // Flee straight away from the recorded grenade impact point.
            real_vector3d away;
            away.i = a->grenade_impact_point.x - a->body_position.x;
            away.j = a->grenade_impact_point.y - a->body_position.y;
            away.k = a->grenade_impact_point.z - a->body_position.z;
            a->desired_direction_valid = 0;
            movement_mode = 0;
            if (vector3d_normalize_with_length(&away) == 0.0f) {
                actor_base[0x58d] = 1;
            } else {
                a->position_cache_a.x = away.i;
                a->position_cache_a.y = away.j;
                a->position_cache_a.z = away.k;
                actor_base[0x58d] = 0;
                actor_base[0x58e] = 0;
                a->unknown_591 = 1;
            }
        } else if (*(int16_t *)&a->unknown_350[16] >= 1) { // 0x360
            a->desired_direction_valid = 0;
            actor_base[0x58d] = 1;
            movement_mode = (uint8_t)((actor_def->flags >> 0x1e) & 1);
        } else {
            clear_recognition = 1;
            if (movement_style == 2 &&
                ((movement_mode == 0 && (actor_def->flags & 0x4000) == 0) ||
                 (movement_mode != 0 && (int8_t)(actor_def->flags >> 8) >= 0))) {
                // leave unknown_505 alone
            } else {
                a->unknown_505 = 0;
            }
            if (movement_style == 4) {
                face_along_heading = 1;
            }
            if ((actor_def->flags & 0x200000) != 0) { // "free flying sidestep"
                avoid_threshold = actor_def->free_flying_sidestep * actor_def->free_flying_sidestep;
                sidestep_mode = 1;
                want_avoid_check = 1;
                if (a->unknown_505 != 0) {
                    avoid_threshold *= 4.0f;
                }
            }
        }
    } else {
        // ---- driving or riding a unit --------------------------------------------------
        object *unit_object = ((object_header *)object_data->data)[a->active_unit_index & 0xffff].data;
        Vehicle *vehicle_def = (Vehicle *)tag_instances[unit_object->definition_tag & 0xffff].data;
        uint8_t take_sideslip = 0;

        steering_maximum = vehicle_def->ai_steering_maximum;
        if (0.0f < vehicle_def->ai_throttle_maximum) {
            throttle_maximum = vehicle_def->ai_throttle_maximum;
        }
        oversteer_min = vehicle_def->ai_oversteering_bounds[0];
        oversteer_max = vehicle_def->ai_oversteering_bounds[1];

        if (movement_style == 2) {
            vehicle_data *unit_vehicle =
                (vehicle_data *)((uint8_t *)unit_object + k_unit_object_size);
            if (unit_vehicle->airborne_ticks != 0) {
                vehicle_stuck = 1;
                a->desired_direction_valid = 0;
                actor_base[0x58d] = 1;
                movement_mode = 0;
            } else if (0.7f <= unit_vehicle->ground_lean) {
                take_sideslip = 1;
            } else {
                vehicle_stuck = 1;
                if (0.8f <= unit_object->up.k) {
                    take_sideslip = 1;
                } else {
                    // Flipped over: steer along the flattened up vector to right the vehicle.
                    real_vector3d righting;
                    righting.i = unit_object->up.i;
                    righting.j = unit_object->up.j;
                    righting.k = 0.0f;
                    movement_mode = 0;
                    if (vector3d_normalize_with_length(&righting) <= 0.0f) {
                        a->desired_direction_valid = 0;
                    } else {
                        a->desired_direction_valid = 1;
                        a->desired_direction.x = righting.i * 3.0f;
                        a->desired_direction.y = righting.j * 3.0f;
                        a->desired_direction.z = righting.k * 3.0f;
                    }
                }
            }
        } else if (movement_style == 3) {
            take_sideslip = 1;
        } else if (movement_style == 4) {
            real_vector3d direction;
            if (unit_get_average_active_marker_direction(a->active_unit_index, &direction) == 0) {
                cached_axis = 0;
                sidestep_mode = 1;
                order_failed = 1;
                movement_mode = 0;
            } else {
                a->desired_direction_valid = 0;
                actor_base[0x58d] = 0;
                actor_base[0x58e] = 0;
                a->position_cache_a.x = -direction.i;
                movement_mode = 0;
                a->position_cache_a.y = -direction.j;
                a->position_cache_a.z = -direction.k;
            }
        } else {
            a->desired_direction_valid = 0;
            a->unknown_50a = 0;
            actor_base[0x58d] = (uint8_t)((a->type == 0xf || a->unknown_161 != 0) ? 1 : 0);
            movement_mode = 0;
        }

        if (take_sideslip) {
            want_avoid_check = 1;
            movement_mode = 0;
            avoid_threshold = vehicle_def->ai_sideslip_distance * vehicle_def->ai_sideslip_distance;
        }
    }

    if (a->desired_direction_valid != 0 && a->unknown_506 == 0) {
        actor_movement_apply_steering(
            cached_axis, sidestep_mode,
            actor_index, want_avoid_check, avoid_threshold, order_failed,
            steering_maximum, oversteer_min, oversteer_max, avoidance_scale, throttle_maximum,
            (real_vector3d *)&a->desired_direction, (real_vector3d *)&a->position_cache_a,
            &a->unknown_50a, &a->queued_look_vector, &a->unknown_507, &a->unknown_506);
        if (a->unknown_506 != 0) {
            a->desired_direction_valid = 0;
        }
    }

    if (a->desired_direction_valid != 0) {
        actor_base[0x58e] = 0;
        actor_base[0x58d] = 0;
    } else if (face_along_heading) {
        a->position_cache_a = *(const real_point3d *)&a->facing;
        actor_base[0x58e] = 0;
        a->unknown_50a = 0;
        actor_base[0x58d] = 0;
    } else if (actor_base[0x590] != 0) {
        a->position_cache_a.x = a->unknown_594[1]; // 0x598
        a->position_cache_a.y = a->unknown_594[2]; // 0x59c
        a->position_cache_a.z = a->unknown_594[3]; // 0x5a0
        actor_base[0x58e] = 1;
        a->unknown_50a = 0;
        actor_base[0x58d] = 0;
    }

    if (clear_recognition && a->desired_direction_valid == 0) {
        actor_clear_recognition_history(actor_index, 1);
    }

    if (a->desired_direction_valid != 0 && (actor_def->flags & 0x10000000) != 0) {
        movement_mode = 0;
    }
    actor_base[0x58f] = 0;
    if (movement_mode != 0 && (actor_def->flags & 0x20000000) != 0) {
        actor_base[0x58f] = 1;
    }
    a->unknown_508 = movement_mode;
    if (movement_mode != 0) {
        a->flags |= 1u;
    } else {
        a->flags &= ~1u;
    }

    // With no queued action, no unit being carried and no vehicle, ask the unit to turn on the
    // spot toward the current target once, and say so over the comms channel.
    if (a->secondary_action == -1 &&
        (a->unit_index == (datum_index)k_datum_index_none || unit_is_in_busy_animation_state(a->unit_index) == 0) &&
        a->active_unit_index == (datum_index)k_datum_index_none &&
        a->unknown_15c == 0 && a->unknown_378 != 0 && a->unknown_379 == 0) {
        real_vector2d facing;
        datum_index target_object = (datum_index)k_datum_index_none;

        facing.i = a->facing.i;
        facing.j = a->facing.j;
        if (a->target_unit_index != (datum_index)k_datum_index_none) {
            prop *target_prop = &((prop *)prop_data->data)[a->target_unit_index & 0xffff];
            target_object = target_prop->object_index;
            facing.i = target_prop->unknown_e0.x;
            facing.j = target_prop->unknown_e0.y;
            if (vector2d_normalize_with_length(&facing) == 0.0f) {
                facing.i = a->facing.i;
                facing.j = a->facing.j;
            }
        }
        actor_queue_secondary_action(actor_index, 0, (uint32_t *)&facing);
        ai_communication_broadcast(0x2a, a->unit_index, target_object, 3,
                                   (datum_index)k_datum_index_none,
                                   (datum_index)k_datum_index_none, 0);
        a->unknown_379 = 1;
    }

    if (vehicle_stuck) {
        a->flags |= 2u;
    } else if (a->unknown_15c != 0 || a->active_unit_index != (datum_index)k_datum_index_none) {
        a->unknown_530[0] = 0; // 0x530
    } else if (actor_action_has_queued_secondary(actor_index) == 0 && a->unknown_440 != 0) {
        uint8_t handled = 0;
        if (a->unknown_441 != 0) {
            real_vector2d facing;
            if (a->unknown_442 != 0) {
                facing = a->unknown_444;
            } else {
                facing.i = a->facing.i;
                facing.j = a->facing.j;
                if (vector2d_normalize_with_length(&facing) == 0.0f) {
                    facing = *global_forward2d_pointer;
                }
            }
            if (unit_try_ready_weapon_variant(a->unit_index, &facing) != 0) {
                ai_communication_broadcast(0x2f, a->unit_index,
                                           (datum_index)k_datum_index_none, -1,
                                           (datum_index)k_datum_index_none,
                                           (datum_index)k_datum_index_none, 0);
                handled = 1;
            }
        }
        if (!handled) {
            actor_set_flag_bit1(actor_index);
        }
        if (a->unknown_442 != 0) {
            *(float *)&a->unknown_530[4]  = a->unknown_444.i; // 0x534
            a->unknown_530[0] = 1;                            // 0x530
            *(float *)&a->unknown_530[8]  = a->unknown_444.j; // 0x538
            *(float *)&a->unknown_530[12] = a->unknown_44c;   // 0x53c
            *(float *)&a->unknown_530[16] = a->unknown_450;   // 0x540
        }
    }

    // Save this tick's secondary-action record for the next one.
    *(uint32_t *)&((struct actor *)actor_base)->unknown_6ec = *(uint32_t *)&((struct actor *)actor_base)->secondary_action;
    *(uint32_t *)(actor_base + 0x6f0) = *(uint32_t *)(actor_base + 0x41c);
    *(uint32_t *)(actor_base + 0x6f4) = *(uint32_t *)(actor_base + 0x420);
}

#if 0
/* Ghidra decompilation of actor_movement_update @ 0x416790 (unmodified). */

void actor_movement_update(uint param_1)

{
  short sVar1;
  uint *puVar2;
  uint *puVar3;
  float fVar4;
  float fVar5;
  bool bVar6;
  bool bVar7;
  undefined *puVar8;
  undefined1 uVar9;
  char cVar10;
  short sVar11;
  float *pfVar12;
  int iVar13;
  int iVar14;
  uint uVar15;
  byte bVar16;
  int iVar17;
  undefined4 uVar18;
  int iVar19;
  bool bVar20;
  float10 fVar21;
  float10 fVar22;
  float local_48;
  undefined4 local_40;
  float local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  uint local_24;
  int local_20;
  undefined4 local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  iVar19 = (param_1 & 0xffff) * 0x724;
  iVar17 = *(int *)(DAT_00880360 + 0x34) + iVar19;
  puVar2 = *(uint **)((*(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x58 + iVar19) & 0xffff) * 0x20 +
                      0x14 + DAT_0087bc14);
  *(float *)(iVar17 + 0x5a4) = *(float *)(iVar17 + 0x174);
  *(undefined4 *)(iVar17 + 0x5a8) = *(undefined4 *)(iVar17 + 0x178);
  *(undefined4 *)(iVar17 + 0x5ac) = *(undefined4 *)(iVar17 + 0x17c);
  local_40 = 0;
  local_24 = local_24 & 0xffffff00;
  bVar6 = false;
  bVar20 = false;
  bVar7 = false;
  local_48 = 0.0;
  local_28 = 0;
  local_30 = 0;
  local_2c = 0;
  local_34 = 0;
  local_38 = 0x3f800000;
  *(undefined1 *)(iVar17 + 0x591) = 0;
  *(undefined1 *)(iVar17 + 0x58d) = 1;
  *(undefined1 *)(iVar17 + 0x58e) = 1;
  local_20 = iVar19;
  if (*(char *)(iVar17 + 0x430) == '\0') {
    if (*(short *)(iVar17 + 0x15e) == 4) {
      if (*(char *)(iVar17 + 0x504) == '\0') {
        local_c = *(float *)(iVar17 + 0x174) * 3.0;
        local_8 = *(float *)(iVar17 + 0x178) * 3.0;
        pfVar12 = &local_c;
        local_4 = *(float *)(iVar17 + 0x17c) * 3.0;
      }
      else {
        pfVar12 = (float *)(iVar17 + 0x518);
      }
      actor_movement_choose_avoidance_direction(param_1,pfVar12,&local_18,&local_3c);
      pfVar12 = (float *)(iVar17 + 0x5dc);
      if (local_14 * local_14 + local_10 * local_10 + local_18 * local_18 <=
          *(float *)(iVar17 + 0x5e4) * *(float *)(iVar17 + 0x5e4) +
          *(float *)(iVar17 + 0x5e0) * *(float *)(iVar17 + 0x5e0) + *pfVar12 * *pfVar12) {
        fVar4 = 0.05;
      }
      else {
        fVar4 = 0.3;
      }
      fVar5 = 1.0 - fVar4;
      *pfVar12 = fVar5 * *pfVar12;
      *(float *)(iVar17 + 0x5e0) = fVar5 * *(float *)(iVar17 + 0x5e0);
      *(float *)(iVar17 + 0x5e4) = fVar5 * *(float *)(iVar17 + 0x5e4);
      *pfVar12 = local_18 * fVar4 + *pfVar12;
      *(float *)(iVar17 + 0x5e0) = local_14 * fVar4 + *(float *)(iVar17 + 0x5e0);
      *(float *)(iVar17 + 0x5e4) = local_10 * fVar4 + *(float *)(iVar17 + 0x5e4);
      puVar8 = PTR_DAT_00696714;
      if (*(float *)(iVar17 + 0x5e4) * *(float *)(iVar17 + 0x5e4) +
          *(float *)(iVar17 + 0x5e0) * *(float *)(iVar17 + 0x5e0) + *pfVar12 * *pfVar12 < 0.0001) {
        *pfVar12 = *(float *)PTR_DAT_00696714;
        *(undefined4 *)(iVar17 + 0x5e0) = *(undefined4 *)(puVar8 + 4);
        *(undefined4 *)(iVar17 + 0x5e4) = *(undefined4 *)(puVar8 + 8);
      }
      *(float *)(iVar17 + 0x5ec) = local_3c;
      fVar4 = fVar4 * local_3c + fVar5 * *(float *)(iVar17 + 0x5e8);
      *(float *)(iVar17 + 0x5e8) = fVar4;
      if (fVar4 < 0.001) {
        *(undefined4 *)(iVar17 + 0x5e8) = 0;
      }
      if (*(char *)(iVar17 + 0x504) != '\0') {
        local_18 = *pfVar12;
        local_14 = *(float *)(iVar17 + 0x5e0);
        local_10 = *(float *)(iVar17 + 0x5e4);
        fVar22 = (float10)local_14 * (float10)local_14 +
                 (float10)local_10 * (float10)local_10 + (float10)local_18 * (float10)local_18;
        if ((float10)0.0001 < fVar22) {
          fVar22 = SQRT(fVar22);
          fVar21 = (float10)1.0 / fVar22;
          local_18 = (float)((float10)local_18 * fVar21);
          local_14 = (float)((float10)local_14 * fVar21);
          local_10 = (float)((float10)local_10 * fVar21);
          fVar21 = (float10)fcos(fVar22);
          fVar22 = (float10)fsin(fVar22);
          vector3d_rotate_about_axis((float)fVar22,(float)fVar21);
        }
        local_34 = *(undefined4 *)(iVar17 + 0x5e8);
      }
    }
  }
  else {
    *(undefined4 *)(iVar17 + 0x518) = *(undefined4 *)(iVar17 + 0x434);
    *(undefined4 *)(iVar17 + 0x51c) = *(undefined4 *)(iVar17 + 0x438);
    puVar8 = PTR_DAT_00696714;
    *(undefined4 *)(iVar17 + 0x520) = *(undefined4 *)(iVar17 + 0x43c);
    *(undefined1 *)(iVar17 + 0x504) = 1;
    *(undefined1 *)(iVar17 + 0x58d) = 0;
    *(undefined4 *)(iVar17 + 0x5dc) = *(undefined4 *)puVar8;
    *(undefined4 *)(iVar17 + 0x5e0) = *(undefined4 *)(puVar8 + 4);
    *(undefined4 *)(iVar17 + 0x5e4) = *(undefined4 *)(puVar8 + 8);
    *(undefined4 *)(iVar17 + 0x5e8) = 0;
    *(undefined4 *)(iVar17 + 0x5ec) = 0;
  }
  sVar11 = *(short *)(iVar17 + 0x42c);
  if (sVar11 == -1) {
    sVar11 = 2;
    if (*(char *)(iVar17 + 0x429) == '\0') {
      if (*(char *)(iVar17 + 0x428) == '\0') {
        sVar1 = *(short *)(iVar17 + 0x6a);
        if (sVar1 == 1) {
          sVar11 = 1;
        }
        else if (sVar1 == 2) {
          sVar11 = 0;
        }
        else if (sVar1 == 3) {
          sVar11 = 2;
        }
      }
      else {
        sVar11 = 3;
      }
    }
    else {
      sVar11 = 4;
    }
  }
  *(short *)(iVar17 + 0x6dc) = sVar11;
  local_3c = (float)CONCAT22(local_3c._2_2_,*(undefined2 *)(iVar17 + 0x42e));
  if ((*(char *)(iVar19 + 0x4a8 + *(int *)(DAT_00880360 + 0x34)) == '\0') ||
     (*(float *)(iVar17 + 0x4a0) < (float)puVar2[0x25])) {
    bVar16 = *(byte *)(iVar17 + 0x426);
  }
  else {
    bVar16 = *(byte *)(iVar17 + 0x427);
  }
  sVar11 = *(short *)(iVar17 + 0x15e);
  if (sVar11 < 1) {
    if (*(char *)(iVar17 + 0x160) == '\0') {
      if (*(short *)(iVar17 + 0x418) == -1) {
        sVar11 = *(short *)(iVar17 + 0x6dc);
        if (sVar11 == 1) {
          *(undefined1 *)(iVar17 + 0x504) = 0;
          *(undefined1 *)(iVar17 + 0x58d) = 0;
          *(undefined1 *)(iVar17 + 0x58e) = 0;
          bVar20 = true;
          bVar16 = 0;
        }
        else if ((*(char *)(iVar17 + 0x15c) == '\0') || (*(char *)(iVar17 + 0x99) != '\0')) {
          if (*(char *)(iVar17 + 0x6a0) == '\0') {
            if (*(short *)(iVar17 + 0x360) < 1) {
              bVar6 = true;
              if (sVar11 == 2) {
                if (bVar16 == 0) {
                  if ((*puVar2 & 0x4000) != 0) goto LAB_00416f3e;
                }
                else if ((char)(*puVar2 >> 8) < '\0') goto LAB_00416f3e;
              }
              else {
LAB_00416f3e:
                *(undefined1 *)(iVar17 + 0x505) = 0;
              }
              bVar20 = sVar11 == 4;
              if ((*puVar2 & 0x200000) != 0) {
                local_48 = (float)puVar2[0x26] * (float)puVar2[0x26];
                local_40 = 1;
                if (*(char *)(iVar17 + 0x505) != '\0') {
                  local_48 = local_48 * 4.0;
                }
              }
            }
            else {
              *(undefined1 *)(iVar17 + 0x504) = 0;
              *(undefined1 *)(iVar17 + 0x58d) = 1;
              bVar16 = (byte)(*puVar2 >> 0x1e) & 1;
            }
          }
          else {
            *(undefined1 *)(iVar17 + 0x504) = 0;
            local_18 = *(float *)(iVar17 + 0x6a8) - *(float *)(iVar17 + 300);
            bVar16 = 0;
            local_14 = *(float *)(iVar17 + 0x6ac) - *(float *)(iVar17 + 0x130);
            local_10 = *(float *)(iVar17 + 0x6b0) - *(float *)(iVar17 + 0x134);
            fVar22 = (float10)vector3d_normalize_with_length();
            if ((float10)0.0 == fVar22) {
              *(undefined1 *)(iVar17 + 0x58d) = 1;
            }
            else {
              *(float *)(iVar17 + 0x5a4) = local_18;
              *(float *)(iVar17 + 0x5a8) = local_14;
              *(float *)(iVar17 + 0x5ac) = local_10;
              *(undefined1 *)(iVar17 + 0x58d) = 0;
              *(undefined1 *)(iVar17 + 0x58e) = 0;
              *(undefined1 *)(iVar17 + 0x591) = 1;
            }
          }
        }
        else {
          *(undefined1 *)(iVar17 + 0x504) = 0;
          *(undefined1 *)(iVar17 + 0x58d) = 1;
          bVar16 = 0;
        }
      }
      else {
        *(undefined1 *)(iVar17 + 0x504) = 0;
        *(undefined1 *)(iVar17 + 0x58d) = 0;
        *(undefined1 *)(iVar17 + 0x58e) = 0;
        bVar16 = 0;
      }
    }
    else {
      *(undefined1 *)(iVar17 + 0x504) = 0;
      *(undefined2 *)(iVar17 + 0x50a) = 0;
      if ((*(short *)(iVar17 + 4) == 0xf) || (*(char *)(iVar17 + 0x161) != '\0')) {
        *(undefined1 *)(iVar17 + 0x58d) = 1;
        *(undefined1 *)(iVar17 + 0x58e) = 0;
        bVar16 = 0;
      }
      else {
        *(undefined1 *)(iVar17 + 0x58d) = 0;
        *(undefined1 *)(iVar17 + 0x58e) = 0;
        bVar16 = 0;
      }
    }
  }
  else {
    puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                       (*(uint *)(iVar17 + 0x158) & 0xffff) * 0xc);
    iVar19 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    local_28 = *(undefined4 *)(iVar19 + 0x3a0);
    if (0.0 < *(float *)(iVar19 + 0x3a4)) {
      local_38 = *(undefined4 *)(iVar19 + 0x3a4);
    }
    local_2c = *(undefined4 *)(iVar19 + 0x398);
    local_30 = *(undefined4 *)(iVar19 + 0x39c);
    if (sVar11 == 2) {
      if ((char)puVar3[0x134] == '\0') {
        if ((0.7 <= (float)puVar3[0x13b]) || (bVar7 = true, 0.8 <= (float)puVar3[0x22]))
        goto LAB_00416d72;
        local_18 = (float)puVar3[0x20];
        local_14 = (float)puVar3[0x21];
        local_10 = 0.0;
        fVar22 = (float10)vector3d_normalize_with_length();
        if (fVar22 <= (float10)0.0) {
          *(undefined1 *)(iVar17 + 0x504) = 0;
          bVar16 = 0;
        }
        else {
          *(undefined1 *)(iVar17 + 0x504) = 1;
          bVar16 = 0;
          *(float *)(iVar17 + 0x518) = local_18 * 3.0;
          *(float *)(iVar17 + 0x51c) = local_14 * 3.0;
          *(float *)(iVar17 + 0x520) = local_10 * 3.0;
        }
      }
      else {
        bVar7 = true;
        *(undefined1 *)(iVar17 + 0x504) = 0;
        *(undefined1 *)(iVar17 + 0x58d) = 1;
        bVar16 = 0;
      }
    }
    else if (sVar11 == 3) {
LAB_00416d72:
      local_40 = 1;
      bVar16 = 0;
      local_48 = *(float *)(iVar19 + 0x380) * *(float *)(iVar19 + 0x380);
    }
    else if (sVar11 == 4) {
      cVar10 = FUN_00575e30(*(uint *)(iVar17 + 0x158),&local_c);
      if (cVar10 == '\0') {
        local_3c = 0.0;
        local_24 = CONCAT31(local_24._1_3_,1);
        bVar16 = 0;
      }
      else {
        *(undefined1 *)(iVar17 + 0x504) = 0;
        *(undefined1 *)(iVar17 + 0x58d) = 0;
        *(undefined1 *)(iVar17 + 0x58e) = 0;
        *(float *)(iVar17 + 0x5a4) = -local_c;
        bVar16 = 0;
        *(float *)(iVar17 + 0x5a8) = -local_8;
        *(float *)(iVar17 + 0x5ac) = -local_4;
      }
    }
    else {
      uVar9 = 0;
      *(undefined1 *)(iVar17 + 0x504) = 0;
      *(undefined2 *)(iVar17 + 0x50a) = 0;
      if ((*(short *)(iVar17 + 4) == 0xf) || (*(char *)(iVar17 + 0x161) != '\0')) {
        uVar9 = 1;
      }
      *(undefined1 *)(iVar17 + 0x58d) = uVar9;
      bVar16 = 0;
    }
  }
  if (((*(char *)(iVar17 + 0x504) != '\0') && (*(char *)(iVar17 + 0x506) == '\0')) &&
     (FUN_004180c0(param_1,local_40,local_48,local_24,local_28,local_2c,local_30,local_34,local_38,
                   iVar17 + 0x518,iVar17 + 0x5a4,iVar17 + 0x50a,iVar17 + 0x6e0,iVar17 + 0x507,
                   (char *)(iVar17 + 0x506)), *(char *)(iVar17 + 0x506) != '\0')) {
    *(undefined1 *)(iVar17 + 0x504) = 0;
  }
  if (*(char *)(iVar17 + 0x504) == '\0') {
    if (bVar20) {
      *(undefined4 *)(iVar17 + 0x5a4) = *(undefined4 *)(iVar17 + 0x174);
      *(undefined4 *)(iVar17 + 0x5a8) = *(undefined4 *)(iVar17 + 0x178);
      *(undefined4 *)(iVar17 + 0x5ac) = *(undefined4 *)(iVar17 + 0x17c);
      *(undefined1 *)(iVar17 + 0x58e) = 0;
LAB_0041708e:
      *(undefined2 *)(iVar17 + 0x50a) = 0;
      goto LAB_00417097;
    }
    if (*(char *)(iVar17 + 0x590) != '\0') {
      *(undefined4 *)(iVar17 + 0x5a4) = *(undefined4 *)(iVar17 + 0x598);
      *(undefined4 *)(iVar17 + 0x5a8) = *(undefined4 *)(iVar17 + 0x59c);
      *(undefined4 *)(iVar17 + 0x5ac) = *(undefined4 *)(iVar17 + 0x5a0);
      *(undefined1 *)(iVar17 + 0x58e) = 1;
      goto LAB_0041708e;
    }
  }
  else {
    *(undefined1 *)(iVar17 + 0x58e) = 0;
LAB_00417097:
    *(undefined1 *)(iVar17 + 0x58d) = 0;
  }
  if ((bVar6) && (*(char *)(iVar17 + 0x504) == '\0')) {
    FUN_00414140(1);
  }
  iVar19 = local_20;
  if ((*(char *)(iVar17 + 0x504) != '\0') && ((*puVar2 & 0x10000000) != 0)) {
    bVar16 = 0;
  }
  *(undefined1 *)(iVar17 + 0x58f) = 0;
  if ((bVar16 != 0) && ((*puVar2 & 0x20000000) != 0)) {
    *(undefined1 *)(iVar17 + 0x58f) = 1;
  }
  iVar14 = DAT_00880360;
  *(byte *)(iVar17 + 0x508) = bVar16;
  uVar15 = *(uint *)(*(int *)(iVar14 + 0x34) + 0x6d0 + local_20);
  if (bVar16 == 0) {
    uVar15 = uVar15 & 0xfffffffe;
  }
  else {
    uVar15 = uVar15 | 1;
  }
  *(uint *)(*(int *)(iVar14 + 0x34) + local_20 + 0x6d0) = uVar15;
  iVar13 = *(int *)(iVar14 + 0x34) + local_20;
  if (((*(short *)(iVar13 + 0x418) == -1) &&
      ((((*(int *)(iVar13 + 0x18) == -1 || (cVar10 = FUN_00569c90(), cVar10 == '\0')) &&
        (*(int *)(iVar17 + 0x158) == -1)) &&
       ((*(char *)(iVar17 + 0x15c) == '\0' && (*(char *)(iVar17 + 0x378) != '\0')))))) &&
     (*(char *)(iVar17 + 0x379) == '\0')) {
    local_1c = *(undefined4 *)(iVar17 + 0x178);
    local_20 = *(int *)(iVar17 + 0x174);
    uVar18 = 0xffffffff;
    if (*(uint *)(iVar17 + 0x270) != 0xffffffff) {
      iVar14 = (*(uint *)(iVar17 + 0x270) & 0xffff) * 0x138;
      uVar18 = *(undefined4 *)(iVar14 + 0x18 + *(int *)(DAT_008802c0 + 0x34));
      iVar14 = iVar14 + *(int *)(DAT_008802c0 + 0x34);
      local_20 = *(int *)(iVar14 + 0xe0);
      local_1c = *(undefined4 *)(iVar14 + 0xe4);
      fVar22 = (float10)vector2d_normalize_with_length();
      if ((float10)0.0 == fVar22) {
        local_20 = *(int *)(iVar17 + 0x174);
        local_1c = *(undefined4 *)(iVar17 + 0x178);
      }
    }
    FUN_00417a60(0,&local_20);
    ai_communication_broadcast(0x2a,*(undefined4 *)(iVar17 + 0x18),uVar18,3,0xffffffff,0xffffffff,0)
    ;
    iVar14 = DAT_00880360;
    *(undefined1 *)(iVar17 + 0x379) = 1;
  }
  if (bVar7) {
    *(uint *)(*(int *)(iVar14 + 0x34) + iVar19 + 0x6d0) =
         *(uint *)(*(int *)(iVar14 + 0x34) + 0x6d0 + iVar19) | 2;
    goto LAB_0041736d;
  }
  if ((*(char *)(iVar17 + 0x15c) != '\0') || (*(int *)(iVar17 + 0x158) != -1)) {
    *(undefined1 *)(iVar17 + 0x530) = 0;
    goto LAB_0041736d;
  }
  cVar10 = actor_action_has_queued_secondary();
  if ((cVar10 != '\0') || (*(char *)(iVar17 + 0x440) == '\0')) goto LAB_0041736d;
  if (*(char *)(iVar17 + 0x441) == '\0') {
LAB_0041731a:
    FUN_0042a5b0();
  }
  else {
    if (*(char *)(iVar17 + 0x442) == '\0') {
      local_20 = *(int *)(iVar17 + 0x174);
      local_1c = *(undefined4 *)(iVar17 + 0x178);
      fVar22 = (float10)vector2d_normalize_with_length();
      if ((float10)0.0 == fVar22) {
        local_20 = *(int *)PTR_DAT_006966e8;
        local_1c = *(undefined4 *)(PTR_DAT_006966e8 + 4);
      }
    }
    else {
      local_20 = *(int *)(iVar17 + 0x444);
      local_1c = *(undefined4 *)(iVar17 + 0x448);
    }
    cVar10 = FUN_00569b30();
    if (cVar10 == '\0') goto LAB_0041731a;
    ai_communication_broadcast
              (0x2f,*(undefined4 *)(iVar17 + 0x18),0xffffffff,0xffffffff,0xffffffff,0xffffffff,0);
  }
  if (*(char *)(iVar17 + 0x442) != '\0') {
    *(undefined4 *)(iVar17 + 0x534) = *(undefined4 *)(iVar17 + 0x444);
    *(undefined1 *)(iVar17 + 0x530) = 1;
    *(undefined4 *)(iVar17 + 0x538) = *(undefined4 *)(iVar17 + 0x448);
    *(undefined4 *)(iVar17 + 0x53c) = *(undefined4 *)(iVar17 + 0x44c);
    *(undefined4 *)(iVar17 + 0x540) = *(undefined4 *)(iVar17 + 0x450);
  }
LAB_0041736d:
  *(undefined4 *)(iVar17 + 0x6ec) = *(undefined4 *)(iVar17 + 0x418);
  *(undefined4 *)(iVar17 + 0x6f0) = *(undefined4 *)(iVar17 + 0x41c);
  *(undefined4 *)(iVar17 + 0x6f4) = *(undefined4 *)(iVar17 + 0x420);
  return;
}
#endif
