// biped_integrate_movement  (Ghidra: FUN_0055bea0, named here)
// address 0x55bea0, size 3577 bytes
// name confidence: 0.6   rewrite confidence: 0.45
// evidence for the name: this is the movement integrator that takes an explicit object record
//   pointer instead of looking one up, which is how unit_predict_movement_delta (0x55cca0)
//   drives it over a scratch copy of the local player's unit; it is byte-for-byte the same
//   solve as biped_integrate_movement_with_collision (0x55cfd0) minus the BSP-cluster update,
//   the melee-lunge trace, the fall damage and the jump-tick bookkeeping. The name was
//   proposed by the rewriter that wrote unit_predict_movement_delta.c, which already declares
//   it under this name.
// evidence for the types: the 244-byte block copied out of *(globals_tag_data + 0x174) is
//   GlobalsPlayerInformation (types/tags.h, size 0xf4 = 0x3d dwords, copied with a 0x3d-dword
//   block move). Every index the code uses lands on a named field of it:
//     [0x0b] walking_speed (overridden with 0.512), [0x0c] double_speed_multiplier,
//     [0x0d] run_forward (2.25), [0x0e] run_backward (2.0), [0x0f] run_sideways (2.0),
//     [0x10] run_acceleration (0.32), [0x11..0x14] the four sneak_* fields,
//     [0x15] airborne_acceleration, [0x16] speed_multiplier, [0x20] stun_movement_penalty.
//   That last one is multiplied by unit_data.unknown_424, which independently confirms 0x424
//   as the unit's stun meter (types/units.h already describes it that way).
//   The animation record is ModelAnimationsAnimation (stride 0xb4, frame_info_type at +0x26,
//   frame_info data pointer at +0x54) inside the graph's animations block at tag data +0x78;
//   frame_info_type 1 / 2 / 3 give 8 / 0xc / 0x10 bytes per frame, which is exactly the
//   stride each branch indexes by.
//   Biped tag fields: biped_flags 0x2f4 (bit 0x02 uses_player_physics, 0x04 flying,
//   0x08 physics_pill_centered_at_origin, 0x20 passes_through_other_bipeds,
//   0x40 can_climb_any_surface, 0x800 random_speed_increase,
//   0x1000 unit_uses_old_ntsc_player_physics), max_velocity 0x334, max_sidestep_velocity
//   0x338, acceleration 0x33c, deceleration 0x340, crouch_velocity_modifier 0x34c,
//   downhill_velocity_scale 0x364, uphill_velocity_scale 0x370, crouch_camera_velocity 0x4cc,
//   and the six trigonometric slope constants at 0x4d0..0x4e0.
//   Unit tag fields: unit_flags 0x17c (bit 0x800 simple_creature), stunned_movement_threshold
//   0x240.
// register convention: all three arguments are on the stack.
//   // blam-cc: param_1 -> object_index, param_2 -> object_base, param_3 -> state[2]
// UNSURE: the EAX / ECX operands of vector3d_rotate_about_axis and of the two
//   vector3d_cross_product calls. Ghidra binds only the stack operand; the reconstruction
//   below follows biped_integrate_movement_with_collision (0x55cfd0), whose copy of the same
//   block keeps the rotated forward in named stack slots and so shows the data flow plainly.
// UNSURE: biped_movement_solver_data fields unknown_48 / unknown_5c / unknown_60 / unknown_94
//   / unknown_98 / unknown_a8 -- the values written are known, their meaning to 0x55efd0 is
//   not (that function belongs to the physics module and was not processed).
// UNSURE: actor_check_vehicle_mode_timeout (an actor-side predicate) and weapon_get_zoom_fov (the difficulty-scaled
//   globals lookup already used under that name in src/objects) are foreign-module calls.
// reconciled: R46 biped_data +0x4d4 last_ground_surface_index -> last_ground_object_index (an object datum)
// reconciled: R04 0x006f1d20 int32_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *player_data;     // 0x0087a480, types/units.h
extern real_point3d *global_origin3d_pointer; // 0x00696714
extern uint8_t *globals_tag_data;   // 0x00746fa0, +0x174 is the player_information block
extern uint8_t *some_globals_006f187c; // 0x006f187c, UNSURE: +9 gates the old-physics override
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
extern void vector3d_rotate_about_axis(); // 0x4cd820  // real signature (vector3d_rotate_about_axis.c): void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); Ghidra recovered 0 of 4 args at this call site

extern real weapon_get_zoom_fov(int32_t index);   // 0x46fe10, difficulty-scaled globals lookup
extern uint8_t actor_check_vehicle_mode_timeout(datum_index actor_index); // 0x428270, blam-cc: ECX -> actor_index (object+0x1f4 at both call sites)
extern void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height,
                                          float *pill_radius_out);    // 0x55a2e0, blam-cc: EAX position, ECX object, EBX radius, stack height
extern void biped_update_animation_frame_trigger(float threshold, uint8_t *timing_table, object *object_base);
    // 0x55eaa0, blam-cc: ECX -> timing_table (the Biped tag: EDI, reloaded from the tag slot), ESI -> object_base, stack -> threshold
extern void biped_movement_solve(biped_movement_solver_data *solve);   // 0x55efd0
extern void unit_update_up_vector(Biped *biped_tag, object *obj);      // 0x560800

// Integrates one tick of biped movement against a caller-supplied object record, without
// touching the BSP cluster or applying any damage.
//
// The per-tick displacement comes from one of three sources: the current animation's
// frame_info block (the default, which also yaws the body by the animation's dyaw), the
// GlobalsPlayerInformation walk / run / sneak speeds (for a uses_player_physics biped), or the
// Biped tag's own max_velocity / max_sidestep_velocity (for a flying biped). That displacement,
// the collision pill, the slope-handling constants and the cached ground plane are packed into
// a biped_movement_solver_data and handed to the object movement solver, whose results are
// written back into the record: position, velocity, ground surface, ground normal, and the
// grounded / jumping flags. state[0] receives the movement-direction animation state
// (4..7 walking forward / back / left / right, 8..11 for the stunned variants) and state[1] is
// the caller's landing latch.
void biped_integrate_movement(uint32_t object_index, object *obj, int8_t *state) // blam-cc: see file header
{
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;

    biped_movement_solver_data solve;
    GlobalsPlayerInformation player_info_copy;
    GlobalsPlayerInformation *player_info;
    float speed_scale;
    float dyaw;
    uint32_t biped_flags;

    // A dead unit whose ejection animation is still running is not moved at all.
    if ((obj->flags & 0x20) != 0 && (obj->vitality_flags & _object_health_frozen_bit) != 0 &&
        (unit->animation_state_flags & _unit_animation_flag_unknown_4) != 0) {
        return;
    }

    solve.object_index = object_index;
    solve.facing = obj->forward;
    // Only the low 16 bits are cleared; the record is an uninitialized stack frame and the
    // solver never looks at the high half.
    solve.flags = solve.flags & 0xffff0000;

    if ((((Unit *)tag)->unit_flags & 0x00000800) == 0) {          // "simple_creature"
        object *live = ((object_header *)object_data->data)[object_index & 0xffff].data;
        solve.aiming = ((unit_data *)((uint8_t *)live + k_unit_data_offset))->aiming_vector;
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
    solve.ground_plane = biped->unknown_520;
    solve.ground_surface_index = biped->ground_surface_index;
    solve.unknown_5c = 3.4028235e+38f;   // FLT_MAX
    solve.unknown_60 = 0.0f;

    if (biped->unknown_508 == 1) {
        // Frozen: no displacement at all this tick, but still run the solve so the ground
        // state stays current.
        solve.movement_delta.i = 0.0f;
        solve.movement_delta.j = 0.0f;
        solve.movement_delta.k = 0.0f;
        solve.unknown_48 = 1.0f;
        goto step_crouch;
    }

    biped_flags = tag->biped_flags;
    speed_scale = 1.0f;
    if ((biped_flags & 0x00000800) != 0 && unit->aiming_speed == 0) {   // "random_speed_increase"
        // A stable per-object pseudo-random in [0, 1) scaled by the difficulty value.
        speed_scale = (float)(object_index % 0x89) * 0.00729927f * weapon_get_zoom_fov(8) + 1.0f;
    }

    if ((biped_flags & 0x00000004) == 0 ||                              // "flying"
        (obj->vitality_flags & _object_health_frozen_bit) != 0) {
        if (unit->throttle.i != 0.0f || unit->throttle.j != 0.0f || unit->throttle.k != 0.0f) {
            uint8_t hurt = (0.2f < unit->unknown_424);
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
            (ModelAnimationsAnimation *)((uint8_t *)*(void **)((uint8_t *)tag_instances[obj->animation_graph & 0xffff].data + 0x78) +
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

        if (0.0001f <= (float)fabs((double)dyaw)) {
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
            // The animation drives the body directly: the displacement becomes the velocity
            // and the solver is told not to limit the acceleration.
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

            player_info = *(GlobalsPlayerInformation **)(globals_tag_data + 0x174);

            if (some_globals_006f187c[9] != 0 || (biped_flags & 0x00001000) != 0) {
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
                // the per-player speed scale at player record + 0x6c
                player_speed_scale = *(float *)((uint8_t *)player_data->data +
                                                (unit->controlling_player & 0xffff) * 0x200 + 0x6c);
            }
            speed_scale = (1.0f - player_info->stun_movement_penalty * unit->unknown_424) *
                          player_speed_scale * speed_scale;

            stand_weight = 1.0f - biped->crouch_fraction;
            if (unit->throttle.i <= 0.0f) {
                forward_speed = stand_weight * player_info->run_backward +
                                player_info->sneak_backward * biped->crouch_fraction;
            } else {
                forward_speed = stand_weight * player_info->run_forward +
                                player_info->sneak_forward * biped->crouch_fraction;
            }
            solve.movement_delta.k = 0.0f;
            sideways_speed = stand_weight * player_info->run_sideways +
                             player_info->sneak_sideways * biped->crouch_fraction;

            if (unit->base_animation_state == _unit_base_animation_state_alert) {
                forward_speed = (unit->throttle.i <= 0.0f) ? 0.0f : player_info->walking_speed;
                sideways_speed = 0.0f;
            }

            solve.movement_delta.i = speed_scale * forward_speed * unit->throttle.i * 0.033333335f;
            solve.movement_delta.j = speed_scale * unit->throttle.j * sideways_speed * 0.033333335f;
            solve.maximum_acceleration = (stand_weight * player_info->run_acceleration +
                                          player_info->sneak_acceleration * biped->crouch_fraction) *
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
        float sideways_rate;

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

        sideways_rate = crouch_modifier * tag->max_sidestep_velocity * speed_scale;
        solve.movement_delta.i = crouch_modifier * tag->max_velocity * speed_scale *
                                 unit->throttle.i * 0.033333335f;
        solve.movement_delta.j = sideways_rate * unit->throttle.j * 0.033333335f;
        solve.movement_delta.k = sideways_rate * unit->throttle.k * 0.033333335f;
        solve.maximum_acceleration = ((1.0f - throttle_length) * tag->deceleration +
                                      throttle_length * tag->acceleration) *
                                     crouch_modifier * speed_scale * 0.033333335f;
        solve.airborne_acceleration = solve.maximum_acceleration;
    }

    // A grounded AI actor that has only just landed gets a much tighter step allowance.
    if ((biped->flags & 1) != 0 && biped->unknown_501 < 0x16 &&
        unit->actor_index != k_datum_index_none && actor_check_vehicle_mode_timeout(unit->actor_index) != 0) {
        solve.unknown_5c = 0.1f;
        solve.unknown_60 = 0.5f;
    }
    solve.unknown_48 = 0.0f;

step_crouch:
    // Step the crouch fraction toward or away from 1 at crouch_camera_velocity per tick.
    if (unit->base_animation_state == _unit_base_animation_state_crouch) {
        if (1.0f - biped->crouch_fraction <= tag->crouch_camera_velocity) {
            biped->crouch_fraction = 1.0f;
        } else {
            biped->crouch_fraction = biped->crouch_fraction + tag->crouch_camera_velocity;
        }
    } else if (-tag->crouch_camera_velocity <= -biped->crouch_fraction) {
        biped->crouch_fraction = 0.0f;
    } else {
        biped->crouch_fraction = biped->crouch_fraction - tag->crouch_camera_velocity;
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

        biped_flags = tag->biped_flags;
        if ((biped_flags & 0x00000004) != 0 && dead == 0) {
            solve.flags |= _biped_movement_solver_flying;
        }
        if ((biped_flags & 0x00000020) != 0) {
            solve.flags |= _biped_movement_solver_passes_through_bipeds;
        }
        if ((biped_flags & 0x00000040) != 0 && dead == 0) {
            solve.flags |= _biped_movement_solver_climbs_any_surface;
        }
    }

    biped_movement_solve(&solve);

    // The last supporting surface is remembered for 60 ticks after leaving the ground.
    if (solve.result_surface_index == k_datum_index_none) {
        if (biped->unknown_4d3 < 1) {
            biped->last_ground_object_index = k_datum_index_none;
        } else {
            biped->unknown_4d3 = biped->unknown_4d3 - 1;
        }
    } else {
        biped->unknown_4d3 = 0x3c;
        biped->last_ground_object_index = solve.result_surface_index;
    }

    if ((unit->flags & _unit_flag_unknown_1000000) != 0) {
        // Pinned in place: keep the starting position and drop all velocity.
        solve.result_velocity.i = global_origin3d_pointer->x;
        solve.result_velocity.j = global_origin3d_pointer->y;
        solve.result_velocity.k = global_origin3d_pointer->z;
        solve.result_flags = solve.result_flags & 0xfe;
        solve.result_position = solve.start_position;
    }

    if ((tag->biped_flags & 0x00000008) == 0) {   // "physics_pill_centered_at_origin"
        // undo the pill-radius lift unit_get_crouch_height_offset applied
        solve.result_position.z = solve.result_position.z - solve.pill_radius;
    }

    obj->position = solve.result_position;
    obj->velocity = solve.result_velocity;
    biped->ground_surface_index = solve.result_ground_surface_index;
    biped->unknown_4e0.x = solve.result_position.x;
    biped->unknown_4e0.y = solve.result_position.y;
    biped->unknown_4e0.z = solve.result_position.z;
    biped->unknown_4dc = k_datum_index_none;

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
    biped->unknown_520 = solve.ground_plane;

    if (0.0f < solve.result_impact_speed) {
        biped_update_animation_frame_trigger(solve.result_impact_speed, (uint8_t *)tag, obj);
    }

    if ((solve.flags & _biped_movement_solver_flying) == 0 && (biped->flags & 1) == 0) {
        obj->flags = obj->flags | 2;
    } else {
        obj->flags = obj->flags & 0xfffffffd;
    }

    if ((int16_t)(solve.flags & _biped_movement_solver_flying) != 0 || (biped->flags & 1) != 0 ||
        (solve.result_flags & _biped_movement_result_moving) != 0 ||
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
Original Ghidra decompilation (0x55bea0) -- see out/halo_decompiled.c for the unabridged text;
the body is reproduced verbatim below.

/* WARNING: Removing unreachable block (ram,0x0055c0d8) */

void FUN_0055bea0(uint param_1,uint *param_2,char *param_3)

{
  ... (locals elided for width; full text in out/halo_decompiled.c at "FUN_0055bea0 @ 0055bea0")

  if ((((param_2[4] & 0x20) != 0) && ((*(byte *)((int)param_2 + 0x106) & 4) != 0)) &&
     ((param_2[0xa6] & 4) != 0)) {
    return;
  }
  iVar5 = *(int *)((*param_2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_1dc = param_2[0x1d];
  local_1d8 = param_2[0x1e];
  local_1d4 = param_2[0x1f];
  local_1f0 = param_1;
  local_1ec = local_1ec & 0xffff0000;
  if ((*(uint *)(iVar5 + 0x17c) & 0x800) == 0) {
    iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    local_1d0 = *(uint *)(iVar14 + 0x23c);
    local_1cc = *(uint *)(iVar14 + 0x240);
    local_1c8 = *(uint *)(iVar14 + 0x244);
  }
  else {
    local_1d0 = param_2[0x1d];
    local_1cc = param_2[0x1e];
    local_1c8 = param_2[0x1f];
  }
  pfVar1 = (float *)(param_2 + 0x1a);
  local_1c4 = *pfVar1;
  local_1c0 = param_2[0x1b];
  local_1bc = param_2[0x1c];
  local_1b8 = 0;
  local_1a4 = 0.0053333333;
  local_1a0 = 0.0;
  FUN_0055a2e0(local_19c);
  local_184 = *(undefined4 *)(iVar5 + 0x4d8);
  local_18c = *(undefined4 *)(iVar5 + 0x4d0);
  local_188 = *(undefined4 *)(iVar5 + 0x4d4);
  local_178 = *(undefined4 *)(iVar5 + 0x4e0);
  local_180 = *(undefined4 *)(iVar5 + 0x364);
  local_17c = *(undefined4 *)(iVar5 + 0x4dc);
  local_11c = param_2 + 0x145;
  local_174 = *(undefined4 *)(iVar5 + 0x370);
  local_170 = *local_11c;
  local_16c = param_2[0x146];
  local_168 = param_2[0x147];
  local_164 = param_2[0x148];
  local_160 = param_2[0x136];
  local_194 = 0x7f7fffff;
  local_190 = 0;
  if ((short)param_2[0x142] == 1) {
    local_1b4 = 0.0;
    local_1b0 = 0.0;
    local_1ac = 0.0;
    local_1a8 = 0x3f800000;
    goto LAB_0055c91b;
  }
  uVar11 = *(uint *)(iVar5 + 0x2f4);
  local_208 = 1.0;
  if (((uVar11 & 0x800) != 0) && ((char)param_2[0xa2] == '\0')) {
    fVar19 = (float10)FUN_0046fe10(8);
    local_208 = (float)((float10)(param_1 % 0x89) * (float10)0.00729927 * fVar19 + (float10)1.0);
  }
  if (((uVar11 & 4) == 0) || ((*(byte *)((int)param_2 + 0x106) & 4) != 0)) {
    if (((float)param_2[0x9e] != 0.0) ||
       (((float)param_2[0x9f] != 0.0 || ((float)param_2[0xa0] != 0.0)))) {
      bVar12 = 0.2 < (float)param_2[0x109];
      if ((0.0 < *(float *)(iVar5 + 0x240)) && (*(float *)(iVar5 + 0x240) < (float)param_2[0x3e])) {
        bVar12 = true;
      }
      if (ABS((float)param_2[0x9f]) <= ABS((float)param_2[0x9e])) {
        if (0.0 <= (float)param_2[0x9e]) {
          cVar8 = bVar12 * '\x04' + '\x04';
          goto LAB_0055c20c;
        }
        *param_3 = bVar12 * '\x04' + '\x05';
      }
      else if (0.0 <= (float)param_2[0x9f]) {
        *param_3 = bVar12 * '\x04' + '\x06';
      }
      else {
        cVar8 = bVar12 * '\x04' + '\a';
LAB_0055c20c:
        *param_3 = cVar8;
      }
    }
  }
  else {
    *param_3 = '\0';
  }
  local_1b4 = *(float *)PTR_DAT_00696714;
  local_1b0 = *(float *)(PTR_DAT_00696714 + 4);
  local_1ac = *(float *)(PTR_DAT_00696714 + 8);
  if (((short)param_2[0x34] != -1) &&
     ((((*(byte *)((int)param_2 + 0x106) & 4) != 0 || ((*(byte *)(iVar5 + 0x2f4) & 4) == 0)) &&
      ((param_2[0xa6] & 4) == 0)))) {
    iVar9 = (short)param_2[0x34] * 0xb4;
    iVar14 = *(int *)(*(int *)((param_2[0x33] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x78);
    sVar4 = *(short *)(iVar9 + 0x26 + iVar14);
    iVar9 = iVar9 + iVar14;
    local_20c = 0.0;
    if (sVar4 == 1) {
      pfVar15 = (float *)(*(int *)(iVar9 + 0x54) + *(short *)((int)param_2 + 0xd2) * 8);
      local_1b4 = *pfVar15;
      local_1b0 = pfVar15[1];
    }
    else if (sVar4 == 2) {
      local_1b4 = *(float *)(*(int *)(iVar9 + 0x54) + *(short *)((int)param_2 + 0xd2) * 0xc);
      iVar14 = *(int *)(iVar9 + 0x54) + *(short *)((int)param_2 + 0xd2) * 0xc;
      local_20c = *(float *)(iVar14 + 8);
      local_1b0 = *(float *)(iVar14 + 4);
    }
    else if (sVar4 == 3) {
      iVar14 = *(short *)((int)param_2 + 0xd2) * 0x10;
      local_1ac = *(float *)(iVar14 + 8 + *(int *)(iVar9 + 0x54));
      pfVar15 = (float *)(iVar14 + *(int *)(iVar9 + 0x54));
      local_1b4 = *pfVar15;
      local_1b0 = pfVar15[1];
      local_20c = pfVar15[3];
    }
    local_1b4 = local_1b4 * local_208;
    local_1b0 = local_1b0 * local_208;
    local_1ac = local_1ac * local_208;
    if (0.0001 <= ABS(local_20c)) {
      fVar19 = (float10)fcos((float10)local_20c);
      pfVar15 = (float *)(param_2 + 0x1d);
      fVar16 = *pfVar15;
      uVar11 = param_2[0x1e];
      uVar10 = param_2[0x1f];
      fVar20 = (float10)fsin((float10)local_20c);
      vector3d_rotate_about_axis((float)fVar20,(float)fVar19);
      if ((((param_2[0x82] & 0x20) != 0) &&
          ((*(char *)((int)param_2 + 0x2a3) == '\x02' || (*(char *)((int)param_2 + 0x2a3) == '\x03')
           ))) && (0.5 < (float)param_2[0x89] * *pfVar15 +
                         (float)param_2[0x8a] * (float)param_2[0x1e] +
                         (float)param_2[0x8b] * (float)param_2[0x1f])) {
        vector3d_cross_product(param_2 + 0x89);
        vector3d_cross_product(param_2 + 0x89);
        fVar2 = (local_10c * (float)param_2[0x20] +
                local_104 * (float)param_2[0x22] + local_108 * (float)param_2[0x21]) *
                (local_118 * (float)param_2[0x20] +
                local_110 * (float)param_2[0x22] + local_114 * (float)param_2[0x21]);
        if (fVar2 < 0.0 != (fVar2 == 0.0)) {
          fVar16 = (float)param_2[0x89];
          uVar11 = param_2[0x8a];
          uVar10 = param_2[0x8b];
        }
      }
      *pfVar15 = fVar16;
      param_2[0x1e] = uVar11;
      param_2[0x1f] = uVar10;
      FUN_00560800();
    }
  }
  uVar11 = *(uint *)(iVar5 + 0x2f4);
  if (((uVar11 & 4) == 0) || ((*(byte *)((int)param_2 + 0x106) & 4) != 0)) {
    if (((uVar11 & 2) == 0) || (*(char *)((int)param_2 + 0x2a3) == '\x1c')) {
      if ((param_2[0x133] & 3) == 0) {
        *pfVar1 = local_1b4;
        param_2[0x1b] = (uint)local_1b0;
        local_1a4 = 3.4028235e+38;
      }
    }
    else {
      puVar17 = *(undefined4 **)(DAT_00746fa0 + 0x174);
      if ((*(char *)(DAT_006f187c + 9) != '\0') || ((uVar11 & 0x1000) != 0)) {
        puVar18 = local_100;
        for (iVar14 = 0x3d; iVar14 != 0; iVar14 = iVar14 + -1) {
          *puVar18 = *puVar17;
          puVar17 = puVar17 + 1;
          puVar18 = puVar18 + 1;
        }
        puVar17 = local_100;
        local_d4 = 0.512;
        local_cc = 2.25;
        local_c8 = 2.0;
        local_c4 = 2.0;
        local_c0 = 0x3ea3d70a;
      }
      if (DAT_006f1d20 != 0) {
        puVar18 = local_100;
        for (iVar14 = 0x3d; iVar14 != 0; iVar14 = iVar14 + -1) {
          *puVar18 = *puVar17;
          puVar17 = puVar17 + 1;
          puVar18 = puVar18 + 1;
        }
        puVar17 = local_100;
        local_d4 = local_a8 * local_d4 + local_d4;
        local_cc = local_a8 * local_cc + local_cc;
        local_c8 = local_a8 * local_c8 + local_c8;
        local_c4 = local_a8 * local_c4 + local_c4;
      }
      fVar16 = 1.0;
      if (param_2[0x86] != 0xffffffff) {
        fVar16 = *(float *)((param_2[0x86] & 0xffff) * 0x200 + 0x6c + *(int *)(DAT_0087a480 + 0x34))
        ;
      }
      local_208 = (1.0 - (float)puVar17[0x20] * (float)param_2[0x109]) * fVar16 * local_208;
      fVar16 = 1.0 - (float)param_2[0x143];
      if ((float)param_2[0x9e] <= 0.0) {
        fVar3 = (float)puVar17[0x12] * (float)param_2[0x143];
        fVar2 = (float)puVar17[0xe];
      }
      else {
        fVar3 = (float)puVar17[0x11] * (float)param_2[0x143];
        fVar2 = (float)puVar17[0xd];
      }
      fVar3 = fVar16 * fVar2 + fVar3;
      local_1ac = 0.0;
      local_1b0 = fVar16 * (float)puVar17[0xf] + (float)puVar17[0x13] * (float)param_2[0x143];
      if (*(char *)((int)param_2 + 0x2a7) == '\x01') {
        if ((float)param_2[0x9e] <= 0.0) {
          fVar3 = 0.0;
        }
        else {
          fVar3 = (float)puVar17[0xb];
        }
        local_1b0 = 0.0;
      }
      local_1b4 = local_208 * fVar3 * (float)param_2[0x9e] * 0.033333335;
      local_1b0 = local_208 * (float)param_2[0x9f] * local_1b0 * 0.033333335;
      local_1a4 = (fVar16 * (float)puVar17[0x10] + (float)puVar17[0x14] * (float)param_2[0x143]) *
                  0.033333335;
      local_1a0 = (float)puVar17[0x15] * 0.033333335;
      if ((param_2[0x82] & 0x100) == 0) {
        local_1dc = param_2[0x89];
        local_1d8 = param_2[0x8a];
        local_1d4 = param_2[0x8b];
      }
      if (*(char *)(DAT_006b0b80 + 2) != '\0') {
        local_1ac = (float)puVar17[0xc];
        local_1b4 = local_1b4 * local_1ac;
        local_1b0 = local_1b0 * local_1ac;
        local_1ac = local_1ac * 0.0;
      }
    }
  }
  else {
    if (1.0 <= SQRT((float)param_2[0xa0] * (float)param_2[0xa0] +
                    (float)param_2[0x9f] * (float)param_2[0x9f] +
                    (float)param_2[0x9e] * (float)param_2[0x9e])) {
      fVar16 = 1.0;
    }
    else {
      fVar16 = SQRT((float)param_2[0xa0] * (float)param_2[0xa0] +
                    (float)param_2[0x9f] * (float)param_2[0x9f] +
                    (float)param_2[0x9e] * (float)param_2[0x9e]);
    }
    fVar2 = 1.0;
    if (0.0 < *(float *)(iVar5 + 0x34c)) {
      if (param_2[0x143] == 0x3f800000) {
        fVar2 = *(float *)(iVar5 + 0x34c);
      }
      else {
        fVar2 = 1.0;
        if (0.0 < (float)param_2[0x143]) {
          fVar2 = (*(float *)(iVar5 + 0x34c) - 1.0) * (float)param_2[0x143] + 1.0;
        }
      }
    }
    fVar3 = fVar2 * *(float *)(iVar5 + 0x338) * local_208;
    local_1b4 = fVar2 * *(float *)(iVar5 + 0x334) * local_208 * (float)param_2[0x9e] * 0.033333335;
    local_1b0 = fVar3 * (float)param_2[0x9f] * 0.033333335;
    local_1ac = fVar3 * (float)param_2[0xa0] * 0.033333335;
    local_1a4 = ((1.0 - fVar16) * *(float *)(iVar5 + 0x340) +
                (1.0 - (1.0 - fVar16)) * *(float *)(iVar5 + 0x33c)) * fVar2 * local_208 *
                0.033333335;
    local_1a0 = local_1a4;
  }
  if (((((param_2[0x133] & 1) != 0) && (*(char *)((int)param_2 + 0x501) < '\x16')) &&
      (param_2[0x7d] != 0xffffffff)) && (cVar8 = FUN_00428270(), cVar8 != '\0')) {
    local_194 = 0x3dcccccd;
    local_190 = 0x3f000000;
  }
  local_1a8 = 0;
LAB_0055c91b:
  if (*(char *)((int)param_2 + 0x2a7) == '\x03') {
    if (1.0 - (float)param_2[0x143] <= *(float *)(iVar5 + 0x4cc)) {
      param_2[0x143] = 0x3f800000;
    }
    else {
      param_2[0x143] = (uint)((float)param_2[0x143] + *(float *)(iVar5 + 0x4cc));
    }
  }
  else if (-*(float *)(iVar5 + 0x4cc) <= -(float)param_2[0x143]) {
    param_2[0x143] = 0;
  }
  else {
    param_2[0x143] = (uint)((float)param_2[0x143] - *(float *)(iVar5 + 0x4cc));
  }
  if ((float)param_2[0x143] != 0.0) {
    uVar7 = local_1ec._1_3_;
    local_1ec = local_1ec | 4;
    if (param_3[1] == '\0') {
      local_1ec = CONCAT31(uVar7,(undefined1)local_1ec) | 8;
    }
  }
  uVar11 = param_2[0x133];
  if ((uVar11 & 1) != 0) {
    local_1ec = local_1ec | 1;
  }
  if ((uVar11 & 2) != 0) {
    local_1ec = local_1ec | 2;
  }
  if ((uVar11 & 4) != 0) {
    local_1ec = local_1ec | 0x20;
  }
  if ((uVar11 & 8) != 0) {
    local_1ec = local_1ec | 0x40;
  }
  uVar13 = *(ushort *)((int)param_2 + 0x106) & 4;
  if (uVar13 != 0) {
    local_1ec = local_1ec | 0x80;
  }
  uVar11 = *(uint *)(iVar5 + 0x2f4);
  if (((uVar11 & 4) != 0) && (uVar13 == 0)) {
    local_1ec = local_1ec | 0x10;
  }
  if ((uVar11 & 0x20) != 0) {
    local_1ec = local_1ec | 0x100;
  }
  if (((uVar11 & 0x40) != 0) && (uVar13 == 0)) {
    local_1ec = local_1ec | 0x200;
  }
  FUN_0055efd0(&local_1f0);
  if (local_154 == 0xffffffff) {
    if (*(char *)((int)param_2 + 0x4d3) < '\x01') {
      param_2[0x135] = 0xffffffff;
    }
    else {
      *(char *)((int)param_2 + 0x4d3) = *(char *)((int)param_2 + 0x4d3) + -1;
    }
  }
  else {
    *(undefined1 *)((int)param_2 + 0x4d3) = 0x3c;
    param_2[0x135] = local_154;
  }
  if ((param_2[0x81] & 0x1000000) != 0) {
    local_138 = *(float *)PTR_DAT_00696714;
    local_134 = *(uint *)(PTR_DAT_00696714 + 4);
    local_130 = *(uint *)(PTR_DAT_00696714 + 8);
    local_150 = local_150 & 0xfe;
    local_13c = local_1e0;
    local_144 = local_1e8;
    local_140 = local_1e4;
  }
  if ((*(byte *)(iVar5 + 0x2f4) & 8) == 0) {
    local_13c = local_13c - local_198;
  }
  param_2[0x17] = local_144;
  param_2[0x18] = local_140;
  param_2[0x19] = (uint)local_13c;
  *pfVar1 = local_138;
  param_2[0x1b] = local_134;
  param_2[0x1c] = local_130;
  param_2[0x136] = local_14c;
  param_2[0x138] = local_144;
  param_2[0x139] = local_140;
  param_2[0x13a] = (uint)local_13c;
  param_2[0x137] = 0xffffffff;
  if ((param_3[1] == '\0') && ((local_150 & 4) != 0)) {
    param_3[1] = '\x01';
  }
  if ((local_150 & 1) == 0) {
    uVar11 = param_2[0x133] & 0xfffffffe;
  }
  else {
    uVar11 = param_2[0x133] | 1;
  }
  param_2[0x133] = uVar11;
  if ((local_150 & 2) == 0) {
    uVar11 = uVar11 & 0xfffffffd;
  }
  else {
    uVar11 = uVar11 | 2;
  }
  param_2[0x133] = uVar11;
  if ((*(byte *)(iVar5 + 0x2f4) & 0x20) == 0) {
    uVar11 = param_2[0x133] & 0xffffffef;
  }
  else {
    uVar11 = param_2[0x133] | 0x10;
  }
  param_2[0x133] = uVar11;
  *local_11c = local_170;
  local_11c[1] = local_16c;
  local_11c[2] = local_168;
  local_11c[3] = local_164;
  if (0.0 < local_12c) {
    FUN_0055eaa0(local_12c);
  }
  if (((local_1ec & 0x10) == 0) && ((param_2[0x133] & 1) == 0)) {
    uVar11 = param_2[4] | 2;
  }
  else {
    uVar11 = param_2[4] & 0xfffffffd;
  }
  param_2[4] = uVar11;
  if (((((short)(local_1ec & 0x10) != 0) || ((param_2[0x133] & 1) != 0)) ||
      ((local_150 & 0x10) != 0)) ||
     (0.0001 <= (float)param_2[0x1c] * (float)param_2[0x1c] +
                (float)param_2[0x1b] * (float)param_2[0x1b] + *pfVar1 * *pfVar1)) {
    uVar11 = param_2[4] & 0xffffffdf;
  }
  else {
    uVar11 = uVar11 | 0x20;
  }
  param_2[4] = uVar11;
  puVar6 = PTR_DAT_00696714;
  if ((uVar11 & 2) != 0) {
    param_2[0x23] = *(uint *)PTR_DAT_00696714;
    param_2[0x24] = *(uint *)(puVar6 + 4);
    param_2[0x25] = *(uint *)(puVar6 + 8);
  }
  return;
}
#endif
