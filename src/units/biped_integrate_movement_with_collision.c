// biped_integrate_movement_with_collision  (Ghidra: FUN_0055cfd0, named here)
// address 0x55cfd0, size 4295 bytes
// name confidence: 0.6   rewrite confidence: 0.4
// evidence for the name: this is the live per-tick movement update biped_update (0x5590a0)
//   calls, sharing the two-byte animation-state block with biped_update_facing (0x55b7c0) on
//   the line above it. It is instruction-for-instruction the same solve as
//   biped_integrate_movement (0x55bea0) -- same biped_movement_solver_data laid out at the
//   same relative offsets, same three displacement sources, same write-back -- plus five
//   things the prediction variant leaves out: the BSP cluster relink around the position
//   write (object_unlink_cluster_or_notify_parent / object_set_cluster_and_parent), the
//   crouch-transition pill height change, the melee lunge trace, the jump-tick and target-lock
//   bookkeeping, and unit_apply_fall_damage. Its predecessor name was suggested by the
//   rewriter that wrote unit_predict_movement_delta.c.
// evidence for the types: identical to biped_integrate_movement.c -- see that file's header
//   for the GlobalsPlayerInformation identification (the 0x3d-dword block move out of
//   *(global_globals + 0x174) matches its 0xf4 size and every index lands on a named
//   field), the ModelAnimationsAnimation frame_info decoding, and the Biped / Unit tag field
//   offsets. Two fields are pinned only here: biped_movement_solver_data.height_change (0x38),
//   which this function alone ever sets non-zero, from
//   (standing_collision_height - crouching_collision_height) * the crouch step; and
//   biped_data.melee_target_index (0x4f4), which gates the lunge trace.
// register convention: both arguments are on the stack.
//   // blam-cc: param_1 -> object_index, param_2 -> state[2]
// UNSURE: the EAX / ECX operands of vector3d_rotate_about_axis and of the two
//   vector3d_cross_product calls, and every argument of the melee-lunge collision chain
//   (object_collision_context_build / object_collision_context_test_segment / collision_test_movement_segment / unit_process_melee_special_interaction and matrix4x3_transform_plane)
//   -- those are collision-module and damage-module functions that have not been processed,
//   and Ghidra binds only part of their arguments. The trace's stack buffers (local_468,
//   local_474, local_424..local_40a) are reproduced as opaque byte arrays with the sizes the
//   frame gives them.
// UNSURE: biped_movement_solver_data fields unknown_48 / unknown_5c / unknown_60 and the
//   meaning of result_flags bit 0x10 -- values known, semantics not.
// reconciled: R46 biped_data +0x4d4 last_ground_surface_index -> last_ground_object_index (an object datum)
// reconciled: R04 0x006f1d20 int32_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "physics.h"
#include "projectiles.h"
#include "fn_ai.h"
#include "fn_game.h"
#include "fn_units.h"
#include "fn_math.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *player_data;     // 0x0087a480, types/units.h
extern real_point3d *global_origin3d_pointer; // 0x00696714
extern Globals *global_globals;
extern uint8_t *cinematic_globals_ptr; // 0x006f187c, UNSURE: +9 gates the old-physics override
extern uint8_t *object_update_gate_globals; // 0x006b0b80, +2 is the double-speed switch
extern game_engine_definition *current_game_engine;  // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)

extern double cos(double x);  // x87 FCOS
extern double sin(double x);  // x87 FSIN
extern double sqrt(double x); // x87 FSQRT
extern double fabs(double x); // x87 FABS

// vector3d_cross_product (0x4052c0) computes  *out = stack_operand x ecx_operand,  with out
// in EAX, ecx_operand in ECX and stack_operand pushed -- read out of the callee own
// decompilation (in_EAX / in_ECX / param_1) and matching
// src/objects/object_set_position_and_orientation.c. Ghidra binds only the stack operand at
// the call sites below, so the declaration is left unprototyped.
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, blam-cc: EAX out, ECX a, stack b (b x a); checked at 0x55c3db/0x55c3f2 and 0x55d51e/0x55d52f
// vector3d_rotate_about_axis (0x4cd820) rotates the vector in EAX about the axis in ECX in
// place, by the (sin_angle, cos_angle) pair pushed on the stack -- the callee own
// decompilation is a Rodrigues formula over in_EAX / in_ECX / param_1 / param_2, and
// src/math/vector3d_rotate_toward.c reads it the same way. Ghidra binds only the two stack
// arguments at the call sites below, so the declaration is left unprototyped.
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle);
    // 0x4cd820, blam-cc: EAX -> v, ECX -> axis, stack -> (sin_angle, cos_angle); checked at 0x55c374 / 0x55d4bc


    // 0x46fe10, blam-cc: stack -> zoom_table_index, CX -> magnification (every caller passes the difficulty)
extern game_main_globals *main_game_globals; // 0x006b0b80


extern void matrix4x3_transform_plane(real_plane3d *out, real_matrix4x3 *m, real_plane3d *plane); // 0x4cbf10, EAX, ECX, EDX
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30, src/objects
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0, EAX -> object_index (src/objects)
extern uint8_t object_collision_context_build(uint32_t object_index, object_collision_context *out_context); // 0x504e10, EDI, ECX
extern uint8_t object_collision_context_test_segment(object_collision_context *context, uint32_t flags,
    real_point3d *origin, real_vector3d *delta, object_node_collision_result *out_result); // 0x504f60
extern int8_t collision_test_movement_segment(int32_t mask, real_point3d *origin, real_vector3d *delta,
                            uint32_t ignore_object_index, void *out_record); // 0x505880, UNSURE signature
extern void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height,
                                          float *pill_radius_out);    // 0x55a2e0, blam-cc: EAX position, ECX object, EBX radius, stack height


    // 0x55eaa0, blam-cc: ECX -> timing_table (the Biped tag: EDI, reloaded from the tag slot), ESI -> object_base, stack -> threshold


extern void unit_update_up_vector(Biped *biped_tag, object *obj);     // 0x560800
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
// real signature (unit_process_melee_special_interaction.c):
//   void unit_process_melee_special_interaction(uint32_t attacker_index, uint32_t target_index);
// Ghidra shows five arguments at this call site; they are reproduced verbatim.


// Integrates one tick of a live biped's movement, resolves it against the BSP, and applies
// everything that falls out of the result.
//
// The displacement comes from the same three sources biped_integrate_movement documents (the
// current animation's frame_info, the GlobalsPlayerInformation player-physics speeds, or the
// Biped tag's own flight speeds), and the same biped_movement_solver_data goes to the object
// movement solver. On top of that this variant: feeds the solver the collision-pill height
// change caused by this tick's crouch step, relinks the object into its BSP cluster around the
// position write, runs the melee lunge trace when melee_state is 3 and a melee target is held,
// refreshes the target-lock timers, and applies fall damage from the solver's impact speed.
// state[0] receives the movement-direction animation state (4..7 forward / back / left /
// right, 8..11 for the stunned variants) and state[1] is the landing latch it shares with
// biped_update_facing.
void biped_integrate_movement_with_collision(uint32_t object_index, int8_t *state) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;

    biped_movement_solver_data solve;
    GlobalsPlayerInformation player_info_copy;
    GlobalsPlayerInformation *player_info;
    float speed_scale;
    float dyaw;
    float crouch_step;
    uint32_t biped_flags;
    uint8_t result_flags;

    // A dead unit whose ejection animation is still running is not moved at all.
    if ((obj->flags & 0x20) != 0 && (obj->vitality_flags & _object_health_frozen_bit) != 0 &&
        (unit->animation_state_flags & _unit_animation_flag_unknown_4) != 0) {
        return;
    }

    solve.object_index = object_index;
    solve.facing = obj->forward;
    // Only the low 16 bits are cleared; the record is an uninitialized stack frame.
    solve.flags = solve.flags & 0xffff0000;

    if ((((Unit *)tag)->unit_flags & 0x00000800) == 0) {          // "simple_creature"
        solve.aiming = unit->aiming_vector;
    } else {
        solve.aiming = obj->forward;
    }

    solve.velocity = obj->velocity;
    solve.height_change = 0.0f;
    solve.maximum_acceleration = 0.0053333333f;   // 0.16 per second
    solve.airborne_acceleration = 0.0f;
    // FIXED (0x55bf7e / 0x55d0b9): EAX = &solve.start_position -- this call is what fills it (object position
    // plus the pill radius); the draft dropped it and left start_position uninitialised
    unit_get_crouch_height_offset(&solve.start_position, object_index, &solve.pill_height, &solve.pill_radius);

    solve.cosine_maximum_slope_angle = tag->cosine_maximum_slope_angle;
    solve.negative_sine_downhill_falloff_angle = tag->negative_sine_downhill_falloff_angle;
    solve.negative_sine_downhill_cutoff_angle = tag->negative_sine_downhill_cutoff_angle;
    solve.downhill_velocity_scale = tag->downhill_velocity_scale;
    solve.sine_uphill_falloff_angle = tag->sine_uphill_falloff_angle;
    solve.sine_uphill_cutoff_angle = tag->sine_uphill_cutoff_angle;
    solve.uphill_velocity_scale = tag->uphill_velocity_scale;
    solve.ground_normal = biped->ground_normal;
    solve.ground_plane = biped->ground_plane_offset;
    solve.ground_surface_index = biped->ground_surface_index;
    solve.unknown_5c = 3.4028235e+38f;   // FLT_MAX
    solve.unknown_60 = 0.0f;

    if (biped->unknown_508 == 1) {
        solve.movement_delta.i = 0.0f;
        solve.movement_delta.j = 0.0f;
        solve.movement_delta.k = 0.0f;
        solve.unknown_48 = 1.0f;
        goto step_crouch;
    }

    biped_flags = tag->biped_flags;
    speed_scale = 1.0f;
    if ((biped_flags & 0x00000800) != 0 && unit->aiming_speed == 0) {   // "random_speed_increase"
        speed_scale = (float)(object_index % 0x89) * 0.00729927f * weapon_get_zoom_fov(8, main_game_globals->difficulty) + 1.0f;
    }

    if ((biped_flags & 0x00000004) == 0 ||                              // "flying"
        (obj->vitality_flags & _object_health_frozen_bit) != 0) {
        if (unit->throttle.i != 0.0f || unit->throttle.j != 0.0f || unit->throttle.k != 0.0f) {
            uint8_t hurt = (0.2f < unit->stun_amount);
            if (0.0f < ((Unit *)tag)->stunned_movement_threshold &&
                ((Unit *)tag)->stunned_movement_threshold < obj->recent_body_damage) {
                hurt = 1;
            }
            if ((float)fabs((double)unit->throttle.j) <= (float)fabs((double)unit->throttle.i)) {
                state[0] = (int8_t)(hurt * 4 + (0.0f <= unit->throttle.i ? 4 : 5));
            } else {
                state[0] = (int8_t)(hurt * 4 + (0.0f <= unit->throttle.j ? 6 : 7));
            }
        }
    } else {
        state[0] = 0;
    }

    solve.movement_delta.i = global_origin3d_pointer->x;
    solve.movement_delta.j = global_origin3d_pointer->y;
    solve.movement_delta.k = global_origin3d_pointer->z;

    // --- displacement from the current animation's frame_info -------------------------
    if (obj->animation_index != -1 &&
        ((obj->vitality_flags & _object_health_frozen_bit) != 0 ||
         (tag->biped_flags & 0x00000004) == 0) &&
        (unit->animation_state_flags & _unit_animation_flag_unknown_4) == 0) {
        ModelAnimationsAnimation *animation =
            (ModelAnimationsAnimation *)(*(uint8_t **)((uint8_t *)tag_instances[obj->animation_graph & 0xffff].data + 0x78) +
                                         obj->animation_index * 0xb4);
        float *frame_info = (float *)(uint8_t *)animation->frame_info.pointer;

        dyaw = 0.0f;
        if (animation->frame_info_type == 1) {
            float *f = frame_info + obj->animation_frame * 2;
            solve.movement_delta.i = f[0];
            solve.movement_delta.j = f[1];
        } else if (animation->frame_info_type == 2) {
            float *f = frame_info + obj->animation_frame * 3;
            solve.movement_delta.i = f[0];
            solve.movement_delta.j = f[1];
            dyaw = f[2];
        } else if (animation->frame_info_type == 3) {
            float *f = frame_info + obj->animation_frame * 4;
            solve.movement_delta.i = f[0];
            solve.movement_delta.j = f[1];
            solve.movement_delta.k = f[2];
            dyaw = f[3];
        }
        solve.movement_delta.i = solve.movement_delta.i * speed_scale;
        solve.movement_delta.j = solve.movement_delta.j * speed_scale;
        solve.movement_delta.k = solve.movement_delta.k * speed_scale;

        if (!((float)fabs((double)dyaw) < 0.0001f)) { // 0x55c335 jnp: only |dyaw| < eps skips (NaN rotates)
            real_vector3d new_forward = obj->forward;
            float dyaw_cos = (float)cos((double)dyaw);
            float dyaw_sin = (float)sin((double)dyaw);

            vector3d_rotate_about_axis(&new_forward, &obj->up, dyaw_sin, dyaw_cos);

            if ((unit->control_flags & _unit_control_flag_exact_facing) != 0 &&
                (unit->animation_state == _unit_animation_state_unknown_02 ||
                 unit->animation_state == _unit_animation_state_unknown_03) &&
                0.5f < unit->desired_facing_vector.i * obj->forward.i +
                           unit->desired_facing_vector.j * obj->forward.j +
                           unit->desired_facing_vector.k * obj->forward.k) {
                // The animation's yaw crosses the desired facing when the two cross products
                // (old forward x desired, new forward x desired) point opposite ways along up.
                real_vector3d before;
                real_vector3d after;

                vector3d_cross_product(&before, &obj->forward, &unit->desired_facing_vector);
                vector3d_cross_product(&after, &new_forward, &unit->desired_facing_vector);
                if ((before.i * obj->up.i + before.k * obj->up.k + before.j * obj->up.j) *
                        (after.i * obj->up.i + after.k * obj->up.k + after.j * obj->up.j) <= 0.0f) {
                    new_forward = unit->desired_facing_vector;
                    unit_try_set_animation_state(object_index, 0);   // stop the turn animation
                }
            }

            obj->forward = new_forward;
            unit_update_up_vector(tag, obj);
        }
    }

    biped_flags = tag->biped_flags;
    if ((biped_flags & 0x00000004) == 0 ||                              // "flying"
        (obj->vitality_flags & _object_health_frozen_bit) != 0) {
        if ((biped_flags & 0x00000002) == 0 ||                          // "uses_player_physics"
            unit->animation_state == _unit_animation_state_custom_animation) {
            if ((biped->flags & 3) == 0) {
                obj->velocity.i = solve.movement_delta.i;
                obj->velocity.j = solve.movement_delta.j;
                solve.maximum_acceleration = 3.4028235e+38f;   // FLT_MAX
            }
        } else {
            // --- player physics ------------------------------------------------------
            float stand_weight;
            float forward_speed;
            float sideways_speed;
            float player_speed_scale;

            player_info = (GlobalsPlayerInformation *)global_globals->player_information.pointer;

            if (cinematic_globals_ptr[9] != 0 || (biped_flags & 0x00001000) != 0) {
                player_info_copy = *player_info;               // 0x3d-dword block move
                player_info = &player_info_copy;
                // the original NTSC player-physics constants
                player_info->walking_speed = 0.512f;
                player_info->run_forward = 2.25f;
                player_info->run_backward = 2.0f;
                player_info->run_sideways = 2.0f;
                player_info->run_acceleration = 0.32f;
            }
            if (current_game_engine != 0) {
                player_info_copy = *player_info;
                player_info = &player_info_copy;
                player_info->walking_speed = player_info->speed_multiplier * player_info->walking_speed +
                                             player_info->walking_speed;
                player_info->run_forward = player_info->speed_multiplier * player_info->run_forward +
                                           player_info->run_forward;
                player_info->run_backward = player_info->speed_multiplier * player_info->run_backward +
                                            player_info->run_backward;
                player_info->run_sideways = player_info->speed_multiplier * player_info->run_sideways +
                                            player_info->run_sideways;
            }

            player_speed_scale = 1.0f;
            if (unit->controlling_player != k_datum_index_none) {
                player_speed_scale = *(float *)((uint8_t *)player_data->data +
                                                (unit->controlling_player & 0xffff) * 0x200 + 0x6c);
            }
            speed_scale = (1.0f - player_info->stun_movement_penalty * unit->stun_amount) *
                          player_speed_scale * speed_scale;

            stand_weight = 1.0f - biped->crouch_fraction;
            if (unit->throttle.i <= 0.0f) {
                forward_speed = player_info->sneak_backward * biped->crouch_fraction +
                                stand_weight * player_info->run_backward;
            } else {
                forward_speed = player_info->sneak_forward * biped->crouch_fraction +
                                stand_weight * player_info->run_forward;
            }
            solve.movement_delta.k = 0.0f;
            sideways_speed = player_info->sneak_sideways * biped->crouch_fraction +
                             stand_weight * player_info->run_sideways;

            if (unit->base_animation_state == _unit_base_animation_state_alert) {
                forward_speed = (unit->throttle.i <= 0.0f) ? 0.0f : player_info->walking_speed;
                sideways_speed = 0.0f;
            }

            solve.movement_delta.i = speed_scale * unit->throttle.i * forward_speed * 0.033333335f;
            solve.movement_delta.j = speed_scale * unit->throttle.j * sideways_speed * 0.033333335f;
            solve.maximum_acceleration = (player_info->sneak_acceleration * biped->crouch_fraction +
                                          stand_weight * player_info->run_acceleration) *
                                         0.033333335f;
            solve.airborne_acceleration = player_info->airborne_acceleration * 0.033333335f;

            if ((unit->control_flags & _unit_control_flag_look_dont_turn) == 0) {
                solve.facing = unit->desired_facing_vector;
            }

            if (object_update_gate_globals[2] != 0) {          // the double-speed switch
                solve.movement_delta.k = player_info->double_speed_multiplier;
                solve.movement_delta.i = solve.movement_delta.i * solve.movement_delta.k;
                solve.movement_delta.j = solve.movement_delta.j * solve.movement_delta.k;
                solve.movement_delta.k = solve.movement_delta.k * 0.0f;
            }
        }
    } else {
        // --- flying biped -------------------------------------------------------------
        float throttle_length;
        float crouch_modifier;

        throttle_length = (float)sqrt((double)(unit->throttle.k * unit->throttle.k +
                                               unit->throttle.j * unit->throttle.j +
                                               unit->throttle.i * unit->throttle.i));
        if (1.0f <= throttle_length) {
            throttle_length = 1.0f;
        }

        crouch_modifier = 1.0f;
        if (0.0f < tag->crouch_velocity_modifier) {
            if (biped->crouch_fraction == 1.0f) {
                crouch_modifier = tag->crouch_velocity_modifier;
            } else if (0.0f < biped->crouch_fraction) {
                crouch_modifier = (tag->crouch_velocity_modifier - 1.0f) * biped->crouch_fraction + 1.0f;
            }
        }

        solve.movement_delta.i = crouch_modifier * tag->max_velocity * speed_scale *
                                 unit->throttle.i * 0.033333335f;
        solve.movement_delta.j = crouch_modifier * tag->max_sidestep_velocity * speed_scale *
                                 unit->throttle.j * 0.033333335f;
        solve.movement_delta.k = crouch_modifier * tag->max_sidestep_velocity * speed_scale *
                                 unit->throttle.k * 0.033333335f;
        solve.maximum_acceleration = ((1.0f - throttle_length) * tag->deceleration +
                                      throttle_length * tag->acceleration) *
                                     crouch_modifier * speed_scale * 0.033333335f;
        solve.airborne_acceleration = solve.maximum_acceleration;
    }

    // A grounded AI actor that has only just landed gets a much tighter step allowance.
    if ((biped->flags & 1) != 0 && biped->flags_bit0_ticks < 0x16 &&
        unit->actor_index != k_datum_index_none && actor_check_vehicle_mode_timeout(unit->actor_index) != 0) {
        solve.unknown_5c = 0.1f;
        solve.unknown_60 = 0.5f;
    }
    solve.unknown_48 = 0.0f;

step_crouch:
    // Step the crouch fraction toward or away from 1 at crouch_camera_velocity per tick, and
    // remember how far it actually moved so the pill height change can go to the solver.
    if (unit->base_animation_state == _unit_base_animation_state_crouch) {
        crouch_step = 1.0f - biped->crouch_fraction;
        if (crouch_step <= tag->crouch_camera_velocity) {
            biped->crouch_fraction = 1.0f;
        } else {
            biped->crouch_fraction = tag->crouch_camera_velocity + biped->crouch_fraction;
            crouch_step = tag->crouch_camera_velocity;
        }
    } else {
        crouch_step = -biped->crouch_fraction;
        if (-tag->crouch_camera_velocity <= crouch_step) {
            biped->crouch_fraction = 0.0f;
        } else {
            biped->crouch_fraction = biped->crouch_fraction - tag->crouch_camera_velocity;
            crouch_step = -tag->crouch_camera_velocity;
        }
    }
    if (0.01f < (float)fabs((double)crouch_step) && (biped->flags & 1) != 0) {
        solve.height_change = (tag->standing_collision_height - tag->crouching_collision_height) *
                              crouch_step;
    }

    if (biped->crouch_fraction != 0.0f) {
        solve.flags = solve.flags | _biped_movement_solver_crouching;
        if (state[1] == 0) {
            solve.flags = solve.flags | _biped_movement_solver_crouch_began;
        }
    }

    {
        uint32_t biped_state = biped->flags;
        uint16_t dead = obj->vitality_flags & _object_health_frozen_bit;

        if ((biped_state & 1) != 0) solve.flags |= _biped_movement_solver_airborne;
        if ((biped_state & 2) != 0) solve.flags |= _biped_movement_solver_jumping;
        if ((biped_state & 4) != 0) solve.flags |= _biped_movement_solver_unknown_20;
        if ((biped_state & 8) != 0) solve.flags |= _biped_movement_solver_unknown_40;
        if (dead != 0) solve.flags |= _biped_movement_solver_dead;

        if ((tag->biped_flags & 0x00000004) != 0 && dead == 0) {
            solve.flags |= _biped_movement_solver_flying;
        }
        if ((tag->biped_flags & 0x00000020) != 0) {
            solve.flags |= _biped_movement_solver_passes_through_bipeds;
        }
        if ((tag->biped_flags & 0x00000040) != 0 && dead == 0) {
            solve.flags |= _biped_movement_solver_climbs_any_surface;
        }
    }

    biped_movement_solve(&solve);

    // The last supporting surface is remembered for 60 ticks after leaving the ground.
    if (solve.result_surface_index == k_datum_index_none) {
        if (biped->last_ground_object_ticks < 1) {
            biped->last_ground_object_index = k_datum_index_none;
        } else {
            biped->last_ground_object_ticks = biped->last_ground_object_ticks - 1;
        }
    } else {
        biped->last_ground_object_ticks = 0x3c;
        biped->last_ground_object_index = solve.result_surface_index;
    }

    if ((unit->flags & _unit_flag_unknown_1000000) != 0) {
        // Pinned in place: keep the starting position and drop all velocity.
        solve.velocity.i = global_origin3d_pointer->x;
        solve.velocity.j = global_origin3d_pointer->y;
        solve.velocity.k = global_origin3d_pointer->z;
        solve.result_flags = solve.result_flags & 0xfe;
        solve.result_position = solve.start_position;
        solve.result_velocity = solve.velocity;
    }

    {
        real_point3d final_position = solve.result_position;

        if ((tag->biped_flags & 0x00000008) == 0) {   // "physics_pill_centered_at_origin"
            // undo the pill-radius lift unit_get_crouch_height_offset applied
            final_position.z = solve.result_position.z - solve.pill_radius;
        }

        // The position write has to happen between the two cluster calls so the BSP
        // location bookkeeping follows the object.
        object_unlink_cluster_or_notify_parent(object_index); // EAX-carried; Ghidra bound no argument
        obj->position = final_position;
        object_set_cluster_and_parent(object_index, 0);

        result_flags = solve.result_flags;
        obj->velocity = solve.result_velocity;
        biped->cached_position.x = final_position.x;
        biped->ground_surface_index = solve.result_ground_surface_index;
        biped->cached_position.y = final_position.y;
        biped->cached_surface_index = k_datum_index_none;
        biped->cached_position.z = final_position.z;
    }

    if (state[1] == 0 && (solve.result_flags & _biped_movement_result_landed) != 0) {
        state[1] = 1;
    }

    biped->flags = ((solve.result_flags & _biped_movement_result_airborne) == 0)
                       ? (biped->flags & 0xfffffffe)
                       : (biped->flags | 1);
    biped->flags = ((solve.result_flags & _biped_movement_result_jumping) == 0)
                       ? (biped->flags & 0xfffffffd)
                       : (biped->flags | 2);
    biped->flags = ((tag->biped_flags & 0x00000020) == 0)   // "passes_through_other_bipeds"
                       ? (biped->flags & 0xffffffef)
                       : (biped->flags | 0x10);

    biped->ground_normal = solve.ground_normal;
    biped->ground_plane_offset = solve.ground_plane;

    if (0.0f < solve.result_impact_speed) {
        biped_update_animation_frame_trigger(solve.result_impact_speed, (uint8_t *)tag, obj);
    }
    if ((result_flags & _biped_movement_result_landed) == 0) {
        unit_track_target_lock_timeout(object_index);
    }

    // --- melee lunge trace (0x55de09): a lunging unit whose step crossed its target's bounding sphere and hit
    // the target's collision model before any structure hands the contact to the special melee interaction
    if (unit->melee_state == _unit_melee_state_unknown_3 &&
        biped->melee_target_index != k_datum_index_none) {
        datum_index target_index = biped->melee_target_index;
        uint8_t *target = (uint8_t *)((object_header *)object_data->data)[target_index & 0xffff].data;
        real_vector3d lunge;
        object_collision_context context;           // [esp+0x24], later the contact plane
        object_node_collision_result node_hit;      // [esp+0x260]
        collision_result structure_hit;             // [esp+0x210]

        lunge.i = solve.result_position.x - solve.start_position.x;
        lunge.j = solve.result_position.y - solve.start_position.y;
        lunge.k = solve.result_position.z - solve.start_position.z;
        if (ray_intersects_sphere_test(&solve.start_position, (real_point3d *)(target + 0xa0), &lunge,
                                       ((object *)target)->bounding_radius) &&
            object_collision_context_build(target_index, &context) &&
            object_collision_context_test_segment(&context, 3, &solve.start_position, &lunge, &node_hit) &&
            !collision_test_movement_segment(0xc2a0, &solve.start_position, &lunge, object_index, &structure_hit)) {
            real_point3d contact_point;
            real_plane3d contact_plane;
            uint8_t *hit = (uint8_t *)&node_hit;

            contact_point.x = lunge.i * node_hit.segment.t + solve.start_position.x;
            contact_point.y = lunge.j * node_hit.segment.t + solve.start_position.y;
            contact_point.z = lunge.k * node_hit.segment.t + solve.start_position.z;
            matrix4x3_transform_plane(&contact_plane,
                                      (real_matrix4x3 *)((uint8_t *)context.nodes + *(int16_t *)hit * 0x34),
                                      (real_plane3d *)node_hit.segment.plane);
            if (node_hit.segment.plane_index < 0) {
                contact_plane.normal.i = -contact_plane.normal.i;
                contact_plane.normal.j = -contact_plane.normal.j;
                contact_plane.normal.k = -contact_plane.normal.k;
                contact_plane.d = -contact_plane.d;
            }
            unit_process_melee_special_interaction(object_index, target_index, *(uint32_t *)(hit + 0x0),
                                                   *(uint32_t *)(hit + 0x2), *(uint32_t *)(hit + 0x1a),
                                                   &contact_point, &contact_plane, &structure_hit.leaf);
        }
    }

    // 0x55dfcd: EAX = solve+0x98 (the contacted object moving fastest relative to us), ECX = the biped
    biped_update_target_lock_timer(solve.unknown_98, object_index);
    unit_apply_fall_damage(object_index, solve.result_impact_speed);

    if ((solve.flags & _biped_movement_solver_flying) == 0 && (biped->flags & 1) == 0) {
        obj->flags = obj->flags | 2;
    } else {
        obj->flags = obj->flags & 0xfffffffd;
    }

    if ((int16_t)(solve.flags & _biped_movement_solver_flying) != 0 || (biped->flags & 1) != 0 ||
        (result_flags & _biped_movement_result_moving) != 0 ||
        0.0001f <= obj->velocity.k * obj->velocity.k + obj->velocity.j * obj->velocity.j +
                       obj->velocity.i * obj->velocity.i) {
        obj->flags = obj->flags & 0xffffffdf;
    } else {
        obj->flags = obj->flags | 0x20;
    }

    if ((obj->flags & 2) != 0) {
        obj->angular_velocity.i = global_origin3d_pointer->x;
        obj->angular_velocity.j = global_origin3d_pointer->y;
        obj->angular_velocity.k = global_origin3d_pointer->z;
    }
}

#if 0
Original Ghidra decompilation (0x55cfd0); the locals block is elided for width -- the
unabridged text is in out/halo_decompiled.c under "FUN_0055cfd0 @ 0055cfd0".

/* WARNING: Removing unreachable block (ram,0x0055d216) */

void FUN_0055cfd0(uint param_1,char *param_2)

{
  local_650 = (param_1 & 0xffff) * 0xc;
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_650);
  if ((((puVar3[4] & 0x20) != 0) && ((*(byte *)((int)puVar3 + 0x106) & 4) != 0)) &&
     ((puVar3[0xa6] & 4) != 0)) {
    return;
  }
  iVar14 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_638 = puVar3[0x1d];
  local_634 = puVar3[0x1e];
  local_630 = puVar3[0x1f];
  local_64c = param_1;
  local_648 = local_648 & 0xffff0000;
  if ((*(uint *)(iVar14 + 0x17c) & 0x800) == 0) {
    iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_650);
    local_62c = *(uint *)(iVar4 + 0x23c);
    local_628 = *(uint *)(iVar4 + 0x240);
    local_624 = *(uint *)(iVar4 + 0x244);
  }
  else {
    local_62c = puVar3[0x1d];
    local_628 = puVar3[0x1e];
    local_624 = puVar3[0x1f];
  }
  local_620 = puVar3[0x1a];
  local_61c = puVar3[0x1b];
  local_618 = puVar3[0x1c];
  local_614 = 0.0;
  local_600 = 0.0053333333;
  local_5fc = 0.0;
  local_57c = iVar14;
  FUN_0055a2e0(local_5f8);
  local_5e8 = *(undefined4 *)(iVar14 + 0x4d0);
  local_5e4 = *(undefined4 *)(iVar14 + 0x4d4);
  local_5e0 = *(undefined4 *)(iVar14 + 0x4d8);
  local_5dc = *(undefined4 *)(iVar14 + 0x364);
  local_5d8 = *(undefined4 *)(iVar14 + 0x4dc);
  local_5d4 = *(undefined4 *)(iVar14 + 0x4e0);
  local_5d0 = *(undefined4 *)(iVar14 + 0x370);
  local_5cc = puVar3[0x145];
  local_5c8 = puVar3[0x146];
  local_5c4 = puVar3[0x147];
  local_5c0 = puVar3[0x148];
  local_5bc = puVar3[0x136];
  local_5f0 = 0x7f7fffff;
  local_5ec = 0;
  if ((short)puVar3[0x142] == 1) {
    local_610 = 0.0;
    local_60c = 0.0;
    local_608 = 0.0;
    local_604 = 0x3f800000;
    goto LAB_0055da67;
  }
  uVar11 = *(uint *)(iVar14 + 0x2f4);
  local_670 = 1.0;
  if (((uVar11 & 0x800) != 0) && ((char)puVar3[0xa2] == '\0')) {
    fVar19 = (float10)FUN_0046fe10(8);
    local_670 = (float)((float10)(param_1 % 0x89) * (float10)0.00729927 * fVar19 + (float10)1.0);
  }
  if (((uVar11 & 4) == 0) || ((*(byte *)((int)puVar3 + 0x106) & 4) != 0)) {
    if (((float)puVar3[0x9e] != 0.0) ||
       (((float)puVar3[0x9f] != 0.0 || ((float)puVar3[0xa0] != 0.0)))) {
      bVar12 = 0.2 < (float)puVar3[0x109];
      if ((0.0 < *(float *)(iVar14 + 0x240)) && (*(float *)(iVar14 + 0x240) < (float)puVar3[0x3e]))
      {
        bVar12 = true;
      }
      if (ABS((float)puVar3[0x9f]) <= ABS((float)puVar3[0x9e])) {
        if (0.0 <= (float)puVar3[0x9e]) {
          cVar8 = bVar12 * '\x04' + '\x04';
          goto LAB_0055d356;
        }
        *param_2 = bVar12 * '\x04' + '\x05';
      }
      else if (0.0 <= (float)puVar3[0x9f]) {
        *param_2 = bVar12 * '\x04' + '\x06';
      }
      else {
        cVar8 = bVar12 * '\x04' + '\a';
LAB_0055d356:
        *param_2 = cVar8;
      }
    }
  }
  else {
    *param_2 = '\0';
  }
  local_610 = *(float *)PTR_DAT_00696714;
  local_60c = *(float *)(PTR_DAT_00696714 + 4);
  local_608 = *(float *)(PTR_DAT_00696714 + 8);
  if (((short)puVar3[0x34] != -1) &&
     ((((*(byte *)((int)puVar3 + 0x106) & 4) != 0 || ((*(byte *)(iVar14 + 0x2f4) & 4) == 0)) &&
      ((puVar3[0xa6] & 4) == 0)))) {
    fVar19 = (float10)0.0;
    iVar10 = (short)puVar3[0x34] * 0xb4;
    iVar4 = *(int *)(*(int *)((puVar3[0x33] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x78);
    sVar2 = *(short *)(iVar10 + 0x26 + iVar4);
    iVar10 = iVar10 + iVar4;
    if (sVar2 == 1) {
      pfVar13 = (float *)(*(int *)(iVar10 + 0x54) + *(short *)((int)puVar3 + 0xd2) * 8);
      local_610 = *pfVar13;
      local_60c = pfVar13[1];
    }
    else if (sVar2 == 2) {
      pfVar13 = (float *)(*(int *)(iVar10 + 0x54) + *(short *)((int)puVar3 + 0xd2) * 0xc);
      local_610 = *pfVar13;
      local_60c = pfVar13[1];
      fVar19 = (float10)pfVar13[2];
    }
    else if (sVar2 == 3) {
      pfVar13 = (float *)(*(short *)((int)puVar3 + 0xd2) * 0x10 + *(int *)(iVar10 + 0x54));
      local_610 = *pfVar13;
      local_60c = pfVar13[1];
      local_608 = pfVar13[2];
      fVar19 = (float10)pfVar13[3];
    }
    local_610 = local_610 * local_670;
    local_60c = local_60c * local_670;
    local_608 = local_608 * local_670;
    if ((float10)9.999999747378752e-05 <= ABS(fVar19)) {
      fVar20 = (float10)fcos(fVar19);
      local_66c = (float)puVar3[0x1d];
      local_668 = (float)puVar3[0x1e];
      local_664 = (float)puVar3[0x1f];
      fVar19 = (float10)fsin(fVar19);
      vector3d_rotate_about_axis((float)fVar19,(float)fVar20);
      fVar16 = local_668;
      fVar17 = local_664;
      if ((((puVar3[0x82] & 0x20) != 0) &&
          ((*(char *)((int)puVar3 + 0x2a3) == '\x02' || (*(char *)((int)puVar3 + 0x2a3) == '\x03')))
          ) && (pfVar13 = (float *)(puVar3 + 0x89),
               0.5 < *pfVar13 * (float)puVar3[0x1d] +
                     (float)puVar3[0x8a] * (float)puVar3[0x1e] +
                     (float)puVar3[0x8b] * (float)puVar3[0x1f])) {
        vector3d_cross_product(pfVar13);
        vector3d_cross_product(pfVar13);
        fVar1 = (local_578 * (float)puVar3[0x20] +
                local_570 * (float)puVar3[0x22] + local_574 * (float)puVar3[0x21]) *
                (local_660 * (float)puVar3[0x20] +
                local_658 * (float)puVar3[0x22] + local_65c * (float)puVar3[0x21]);
        fVar16 = local_668;
        fVar17 = local_664;
        if (fVar1 < 0.0 != (fVar1 == 0.0)) {
          local_66c = *pfVar13;
          fVar16 = (float)puVar3[0x8a];
          fVar17 = (float)puVar3[0x8b];
          unit_try_set_animation_state(param_1,0);
        }
      }
      puVar3[0x1d] = (uint)local_66c;
      puVar3[0x1e] = (uint)fVar16;
      puVar3[0x1f] = (uint)fVar17;
      FUN_00560800();
    }
  }
  uVar11 = *(uint *)(iVar14 + 0x2f4);
  if (((uVar11 & 4) == 0) || ((*(byte *)((int)puVar3 + 0x106) & 4) != 0)) {
    if (((uVar11 & 2) == 0) || (*(char *)((int)puVar3 + 0x2a3) == '\x1c')) {
      if ((puVar3[0x133] & 3) == 0) {
        puVar3[0x1a] = (uint)local_610;
        puVar3[0x1b] = (uint)local_60c;
        local_600 = 3.4028235e+38;
      }
    }
    else {
      puVar15 = *(undefined4 **)(DAT_00746fa0 + 0x174);
      if ((*(char *)(DAT_006f187c + 9) != '\0') || ((uVar11 & 0x1000) != 0)) {
        puVar18 = local_56c;
        for (iVar14 = 0x3d; iVar14 != 0; iVar14 = iVar14 + -1) {
          *puVar18 = *puVar15;
          puVar15 = puVar15 + 1;
          puVar18 = puVar18 + 1;
        }
        puVar15 = local_56c;
        local_540 = 0.512;
        local_538 = 2.25;
        local_534 = 2.0;
        local_530 = 2.0;
        local_52c = 0x3ea3d70a;
        iVar14 = local_57c;
      }
      if (DAT_006f1d20 != 0) {
        puVar18 = local_56c;
        for (iVar14 = 0x3d; iVar14 != 0; iVar14 = iVar14 + -1) {
          *puVar18 = *puVar15;
          puVar15 = puVar15 + 1;
          puVar18 = puVar18 + 1;
        }
        puVar15 = local_56c;
        local_540 = local_514 * local_540 + local_540;
        local_538 = local_514 * local_538 + local_538;
        local_534 = local_514 * local_534 + local_534;
        local_530 = local_514 * local_530 + local_530;
        iVar14 = local_57c;
      }
      fVar16 = 1.0;
      if (puVar3[0x86] != 0xffffffff) {
        fVar16 = *(float *)((puVar3[0x86] & 0xffff) * 0x200 + 0x6c + *(int *)(DAT_0087a480 + 0x34));
      }
      local_670 = (1.0 - (float)puVar15[0x20] * (float)puVar3[0x109]) * fVar16 * local_670;
      fVar16 = 1.0 - (float)puVar3[0x143];
      if ((float)puVar3[0x9e] <= 0.0) {
        fVar1 = (float)puVar15[0xe];
        fVar17 = (float)puVar15[0x12];
      }
      else {
        fVar1 = (float)puVar15[0xd];
        fVar17 = (float)puVar15[0x11];
      }
      fVar17 = fVar17 * (float)puVar3[0x143] + fVar16 * fVar1;
      local_608 = 0.0;
      local_60c = (float)puVar15[0x13] * (float)puVar3[0x143] + fVar16 * (float)puVar15[0xf];
      if (*(char *)((int)puVar3 + 0x2a7) == '\x01') {
        if ((float)puVar3[0x9e] <= 0.0) {
          fVar17 = 0.0;
          local_60c = 0.0;
        }
        else {
          fVar17 = (float)puVar15[0xb];
          local_60c = 0.0;
        }
      }
      local_610 = local_670 * (float)puVar3[0x9e] * fVar17 * 0.033333335;
      local_60c = local_670 * (float)puVar3[0x9f] * local_60c * 0.033333335;
      local_600 = ((float)puVar15[0x14] * (float)puVar3[0x143] + fVar16 * (float)puVar15[0x10]) *
                  0.033333335;
      local_5fc = (float)puVar15[0x15] * 0.033333335;
      if ((puVar3[0x82] & 0x100) == 0) {
        local_638 = puVar3[0x89];
        local_634 = puVar3[0x8a];
        local_630 = puVar3[0x8b];
      }
      if (*(char *)(DAT_006b0b80 + 2) != '\0') {
        local_608 = (float)puVar15[0xc];
        local_610 = local_610 * local_608;
        local_60c = local_60c * local_608;
        local_608 = local_608 * 0.0;
      }
    }
  }
  else {
    if (1.0 <= SQRT((float)puVar3[0xa0] * (float)puVar3[0xa0] +
                    (float)puVar3[0x9f] * (float)puVar3[0x9f] +
                    (float)puVar3[0x9e] * (float)puVar3[0x9e])) {
      fVar16 = 1.0;
    }
    else {
      fVar16 = SQRT((float)puVar3[0xa0] * (float)puVar3[0xa0] +
                    (float)puVar3[0x9f] * (float)puVar3[0x9f] +
                    (float)puVar3[0x9e] * (float)puVar3[0x9e]);
    }
    fVar17 = 1.0;
    if (0.0 < *(float *)(iVar14 + 0x34c)) {
      if (puVar3[0x143] == 0x3f800000) {
        fVar17 = *(float *)(iVar14 + 0x34c);
      }
      else {
        fVar17 = 1.0;
        if (0.0 < (float)puVar3[0x143]) {
          fVar17 = (*(float *)(iVar14 + 0x34c) - 1.0) * (float)puVar3[0x143] + 1.0;
        }
      }
    }
    local_610 = fVar17 * *(float *)(iVar14 + 0x334) * local_670 * (float)puVar3[0x9e] * 0.033333335;
    local_60c = fVar17 * *(float *)(iVar14 + 0x338) * local_670 * (float)puVar3[0x9f] * 0.033333335;
    local_608 = fVar17 * *(float *)(iVar14 + 0x338) * local_670 * (float)puVar3[0xa0] * 0.033333335;
    local_600 = ((1.0 - fVar16) * *(float *)(iVar14 + 0x340) +
                (1.0 - (1.0 - fVar16)) * *(float *)(iVar14 + 0x33c)) * fVar17 * local_670 *
                0.033333335;
    local_5fc = local_600;
  }
  if (((((puVar3[0x133] & 1) != 0) && (*(char *)((int)puVar3 + 0x501) < '\x16')) &&
      (puVar3[0x7d] != 0xffffffff)) && (cVar8 = FUN_00428270(), cVar8 != '\0')) {
    local_5f0 = 0x3dcccccd;
    local_5ec = 0x3f000000;
  }
  local_604 = 0;
LAB_0055da67:
  puVar5 = PTR_DAT_00696714;
  if (*(char *)((int)puVar3 + 0x2a7) == '\x03') {
    fVar16 = 1.0 - (float)puVar3[0x143];
    if (fVar16 <= *(float *)(iVar14 + 0x4cc)) {
      puVar3[0x143] = 0x3f800000;
    }
    else {
      puVar3[0x143] = (uint)(*(float *)(iVar14 + 0x4cc) + (float)puVar3[0x143]);
      fVar16 = *(float *)(iVar14 + 0x4cc);
    }
  }
  else {
    fVar16 = -(float)puVar3[0x143];
    if (-*(float *)(iVar14 + 0x4cc) <= fVar16) {
      puVar3[0x143] = 0;
    }
    else {
      puVar3[0x143] = (uint)((float)puVar3[0x143] - *(float *)(iVar14 + 0x4cc));
      fVar16 = -*(float *)(iVar14 + 0x4cc);
    }
  }
  if ((0.01 < ABS(fVar16)) && ((puVar3[0x133] & 1) != 0)) {
    local_614 = (*(float *)(iVar14 + 0x424) - *(float *)(iVar14 + 0x428)) * fVar16;
  }
  if ((float)puVar3[0x143] != 0.0) {
    uVar6 = local_648._1_3_;
    local_648 = local_648 | 4;
    if (param_2[1] == '\0') {
      local_648 = CONCAT31(uVar6,(undefined1)local_648) | 8;
    }
  }
  uVar11 = puVar3[0x133];
  if ((uVar11 & 1) != 0) {
    local_648 = local_648 | 1;
  }
  if ((uVar11 & 2) != 0) {
    local_648 = local_648 | 2;
  }
  if ((uVar11 & 4) != 0) {
    local_648 = local_648 | 0x20;
  }
  if ((uVar11 & 8) != 0) {
    local_648 = local_648 | 0x40;
  }
  uVar9 = *(ushort *)((int)puVar3 + 0x106) & 4;
  if (uVar9 != 0) {
    local_648 = local_648 | 0x80;
  }
  if (((*(byte *)(iVar14 + 0x2f4) & 4) != 0) && (uVar9 == 0)) {
    local_648 = local_648 | 0x10;
  }
  if ((*(byte *)(iVar14 + 0x2f4) & 0x20) != 0) {
    local_648 = local_648 | 0x100;
  }
  if (((*(byte *)(iVar14 + 0x2f4) & 0x40) != 0) && (uVar9 == 0)) {
    local_648 = local_648 | 0x200;
  }
  FUN_0055efd0(&local_64c);
  if (local_5b0 == 0xffffffff) {
    if (*(char *)((int)puVar3 + 0x4d3) < '\x01') {
      puVar3[0x135] = 0xffffffff;
    }
    else {
      *(char *)((int)puVar3 + 0x4d3) = *(char *)((int)puVar3 + 0x4d3) + -1;
    }
  }
  else {
    *(undefined1 *)((int)puVar3 + 0x4d3) = 0x3c;
    puVar3[0x135] = local_5b0;
  }
  if ((puVar3[0x81] & 0x1000000) != 0) {
    local_620 = *(uint *)puVar5;
    local_61c = *(uint *)(puVar5 + 4);
    local_618 = *(uint *)(puVar5 + 8);
    local_5ac = local_5ac & 0xfe;
    local_5a0 = local_644;
    local_59c = local_640;
    local_598 = local_63c;
    local_594 = local_620;
    local_590 = local_61c;
    local_58c = local_618;
  }
  fVar16 = local_5a0;
  local_668 = local_59c;
  local_664 = local_598;
  if ((*(byte *)(iVar14 + 0x2f4) & 8) == 0) {
    local_664 = local_598 - local_5f4;
  }
  fVar17 = local_664;
  local_650 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_650);
  object_unlink_cluster_or_notify_parent();
  *(float *)(local_650 + 0x5c) = fVar16;
  *(float *)(local_650 + 0x60) = local_668;
  *(float *)(local_650 + 100) = fVar17;
  object_set_cluster_and_parent(param_1,0);
  bVar7 = local_5ac;
  puVar3[0x1a] = local_594;
  puVar3[0x1b] = local_590;
  puVar3[0x1c] = local_58c;
  puVar3[0x138] = (uint)fVar16;
  puVar3[0x136] = local_5a8;
  puVar3[0x139] = (uint)local_668;
  puVar3[0x137] = 0xffffffff;
  puVar3[0x13a] = (uint)fVar17;
  if ((param_2[1] == '\0') && ((local_5ac & 4) != 0)) {
    param_2[1] = '\x01';
  }
  if ((local_5ac & 1) == 0) {
    uVar11 = puVar3[0x133] & 0xfffffffe;
  }
  else {
    uVar11 = puVar3[0x133] | 1;
  }
  puVar3[0x133] = uVar11;
  if ((local_5ac & 2) == 0) {
    uVar11 = uVar11 & 0xfffffffd;
  }
  else {
    uVar11 = uVar11 | 2;
  }
  puVar3[0x133] = uVar11;
  if ((*(byte *)(iVar14 + 0x2f4) & 0x20) == 0) {
    uVar11 = puVar3[0x133] & 0xffffffef;
  }
  else {
    uVar11 = puVar3[0x133] | 0x10;
  }
  puVar3[0x133] = uVar11;
  puVar3[0x145] = local_5cc;
  puVar3[0x146] = local_5c8;
  puVar3[0x147] = local_5c4;
  puVar3[0x148] = local_5c0;
  if (0.0 < local_588) {
    FUN_0055eaa0(local_588);
  }
  if ((bVar7 & 4) == 0) {
    FUN_0055ec90();
  }
  if ((*(char *)((int)puVar3 + 0x289) == '\x03') && (puVar3[0x13d] != 0xffffffff)) {
    local_66c = local_5a0 - local_644;
    local_668 = local_59c - local_640;
    local_664 = local_598 - local_63c;
    cVar8 = ray_intersects_sphere_test
                      (*(undefined4 *)
                        (*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar3[0x13d] & 0xffff) * 0xc
                                 ) + 0xac));
    if ((cVar8 != '\0') &&
       (((cVar8 = FUN_00504e10(), cVar8 != '\0' &&
         (cVar8 = FUN_00504f60(&local_660,3,&local_644,&local_66c,&local_424), cVar8 != '\0')) &&
        (cVar8 = FUN_00505880(0xc2a0,&local_644,&local_66c,param_1,local_474), cVar8 == '\0')))) {
      local_578 = local_66c * local_41c + local_644;
      local_574 = local_668 * local_41c + local_640;
      local_570 = local_664 * local_41c + local_63c;
      matrix4x3_transform_plane();
      if (local_410 < 0) {
        local_660 = -local_660;
        local_65c = -local_65c;
        local_658 = -local_658;
        local_654 = -local_654;
      }
      FUN_0056ff40(puVar3[0x13d],CONCAT22(uStack_422,local_424),CONCAT22(uStack_420,uStack_422),
                   local_40a,&local_578,&local_660,local_468);
    }
  }
  FUN_0055e0a0(&local_620);
  unit_apply_fall_damage(param_1,local_588);
  if (((local_648 & 0x10) == 0) && ((puVar3[0x133] & 1) == 0)) {
    uVar11 = puVar3[4] | 2;
  }
  else {
    uVar11 = puVar3[4] & 0xfffffffd;
  }
  puVar3[4] = uVar11;
  if ((((short)(local_648 & 0x10) != 0) || ((puVar3[0x133] & 1) != 0)) ||
     (((local_5ac & 0x10) != 0 ||
      (0.0001 <= (float)puVar3[0x1c] * (float)puVar3[0x1c] +
                 (float)puVar3[0x1b] * (float)puVar3[0x1b] +
                 (float)puVar3[0x1a] * (float)puVar3[0x1a])))) {
    uVar11 = puVar3[4] & 0xffffffdf;
  }
  else {
    uVar11 = uVar11 | 0x20;
  }
  puVar3[4] = uVar11;
  puVar5 = PTR_DAT_00696714;
  if ((uVar11 & 2) != 0) {
    puVar3[0x23] = *(uint *)PTR_DAT_00696714;
    puVar3[0x24] = *(uint *)(puVar5 + 4);
    puVar3[0x25] = *(uint *)(puVar5 + 8);
  }
  return;
}
#endif
