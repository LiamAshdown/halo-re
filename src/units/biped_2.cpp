#include "halo/math/constants.hpp"
#include "halo/units/animation_states.hpp"
#include "halo/core/bit_cast.hpp"
#include <string.h>
#include "halo/game/records.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/units/records.hpp"
#include "halo/units/unit.hpp"
#include "halo/core/collision_flags.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "physics.h"
#include "projectiles.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
static constexpr float k_flat_ground_k = 0.0001f;
static constexpr float k_ground_normal_offset = 0.0078125f;

static auto &global_origin3d_pointer = halo::link::ref<real_point3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &cinematic_globals_ptr = halo::link::ref<uint8_t *>(halo::game::vars().cinematic_globals_ptr);
static auto &object_update_gate_globals = halo::link::ref<uint8_t *>(halo::units::vars().object_update_gate_globals);
static auto &k_default_resting_plane = halo::link::ref<float [4]>(halo::units::vars().k_default_resting_plane);
static auto &global_structure_collision_bsp = halo::link::ref<ModelCollisionGeometryBSP *>(halo::physics::vars().global_structure_collision_bsp);

namespace halo::units {

/**
 * Integrates one tick of biped movement against a caller-supplied object record, without touching the BSP
 * cluster or applying any damage. The per-tick displacement comes from one of three sources: the current
 * animation's frame_info block (the default, which also yaws the body by the animation's dyaw), the
 * GlobalsPlayerInformation walk / run / sneak speeds (for a uses_player_physics biped), or the Biped tag's
 * own max_velocity / max_sidestep_velocity (for a flying biped).
 *
 * Original register convention: see file header.
 *
 * @address 0x55bea0
 */
void BipedView::integrate_movement(object *obj, int8_t *state)
{
    uint32_t object_index = datum_handle;
    unit_data *unit = halo::units::unit_data_of(obj);
    biped_data *biped = halo::units::biped_data_of(obj);
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    biped_movement_solver_data solve;
    GlobalsPlayerInformation player_info_copy;
    GlobalsPlayerInformation *player_info;
    float speed_scale;
    float dyaw;
    uint32_t biped_flags;

    if (test_flag(obj->flags, objects::object_flag::at_rest) && (obj->vitality_flags & _object_health_frozen_bit) != 0 &&
        (unit->animation_state_flags & _unit_animation_flag_unknown_4) != 0) {
        return;
    }

    solve.object_index = object_index;
    solve.facing = obj->forward;
    solve.flags = solve.flags & 0xffff0000;

    if (!test_flag(((Unit *)tag)->unit_flags, tags::unit_tag_flag::simple_creature)) {
        object *live = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
        solve.aiming = (halo::units::unit_data_of(live))->aiming_vector;
    } else {
        solve.aiming = obj->forward;
    }

    solve.velocity = obj->velocity;
    solve.height_change = 0.0f;
    solve.maximum_acceleration = 0.0053333333f;
    solve.airborne_acceleration = 0.0f;
    ::halo::units::unit_get_crouch_height_offset(&solve.start_position, object_index, &solve.pill_height, &solve.pill_radius);

    solve.cosine_maximum_slope_angle = tag->cosine_maximum_slope_angle;
    solve.negative_sine_downhill_falloff_angle = tag->negative_sine_downhill_falloff_angle;
    solve.negative_sine_downhill_cutoff_angle = tag->negative_sine_downhill_cutoff_angle;
    solve.downhill_velocity_scale = tag->downhill_velocity_scale;
    solve.sine_uphill_falloff_angle = tag->sine_uphill_falloff_angle;
    solve.sine_uphill_cutoff_angle = tag->sine_uphill_cutoff_angle;
    solve.uphill_velocity_scale = tag->uphill_velocity_scale;
    solve.ground_normal = biped->ground_normal;
    solve.ground_plane = biped->ground_plane_distance;
    solve.ground_surface_index = biped->ground_surface_index;
    solve.steep_landing_maximum_slide = 3.4028235e+38f;
    solve.steep_landing_minimum_penetration = 0.0f;

    if (biped->landing_type == 1) {
        solve.movement_delta.i = 0.0f;
        solve.movement_delta.j = 0.0f;
        solve.movement_delta.k = 0.0f;
        solve.frozen_fraction = 1.0f;
    } else {
        biped_flags = tag->biped_flags;
        speed_scale = 1.0f;
        if (test_flag(biped_flags, tags::biped_tag_flag::random_speed_increase) && unit->aiming_speed == 0) {
            speed_scale = (float)(object_index % 0x89) * 0.00729927f * halo::game::weapon_get_zoom_fov(8, halo::main::globals().game_globals->difficulty) + 1.0f;
        }

        if (!test_flag(biped_flags, tags::biped_tag_flag::flying) ||
            (obj->vitality_flags & _object_health_frozen_bit) != 0) {
            if (unit->throttle.i != 0.0f || unit->throttle.j != 0.0f || unit->throttle.k != 0.0f) {
                uint8_t hurt = (0.2f < unit->stun);
                if (0.0f < ((Unit *)tag)->stunned_movement_threshold &&
                    ((Unit *)tag)->stunned_movement_threshold < obj->recent_body_damage) {
                    hurt = 1;
                }
                if ((float)halo::libm::fabs((double)unit->throttle.j) <= (float)halo::libm::fabs((double)unit->throttle.i)) {
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

        if (obj->animation_index != -1 &&
            ((obj->vitality_flags & _object_health_frozen_bit) != 0 ||
             !test_flag(tag->biped_flags, tags::biped_tag_flag::flying)) &&
            (unit->animation_state_flags & _unit_animation_flag_unknown_4) == 0) {
            ModelAnimationsAnimation *animation = &halo::objects::block_element<ModelAnimationsAnimation>(
                halo::objects::tag_as<ModelAnimations>(obj->animation_graph)->animations, obj->animation_index);
            float *frame_info = reinterpret_cast<float *>(static_cast<uintptr_t>(animation->frame_info.pointer));

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

            if (!((float)halo::libm::fabs((double)dyaw) < 0.0001f)) {
                real_vector3d new_forward = obj->forward;
                float dyaw_cos = (float)halo::libm::cos((double)dyaw);
                float dyaw_sin = (float)halo::libm::sin((double)dyaw);

                halo::math::vector3d_rotate_about_axis(new_forward, obj->up, dyaw_sin, dyaw_cos);

                if ((unit->control_flags & _unit_control_flag_exact_facing) != 0 &&
                    (unit->animation_state == _unit_animation_state_unknown_02 ||
                     unit->animation_state == _unit_animation_state_unknown_03) &&
                    0.5f < unit->desired_facing_vector.i * obj->forward.i +
                               unit->desired_facing_vector.j * obj->forward.j +
                               unit->desired_facing_vector.k * obj->forward.k) {
                    real_vector3d before;
                    real_vector3d after;

                    halo::math::vector3d_cross_product(before, obj->forward, unit->desired_facing_vector);
                    halo::math::vector3d_cross_product(after, new_forward, unit->desired_facing_vector);
                    if ((before.i * obj->up.i + before.k * obj->up.k + before.j * obj->up.j) *
                            (after.i * obj->up.i + after.k * obj->up.k + after.j * obj->up.j) <= 0.0f) {
                        new_forward = unit->desired_facing_vector;
                    }
                }

                obj->forward = new_forward;
                ::halo::units::unit_update_up_vector((Biped *)tag, (::object *)obj);
            }
        }

        biped_flags = tag->biped_flags;
        if (!test_flag(biped_flags, tags::biped_tag_flag::flying) ||
            (obj->vitality_flags & _object_health_frozen_bit) != 0) {
            if (!test_flag(biped_flags, tags::biped_tag_flag::uses_player_physics) ||
                unit->animation_state == _unit_animation_state_custom_animation) {
                if (!test_flag(biped->flags, units::biped_flag::airborne | units::biped_flag::jumping)) {
                    obj->velocity.i = solve.movement_delta.i;
                    obj->velocity.j = solve.movement_delta.j;
                    solve.maximum_acceleration = 3.4028235e+38f;
                }
            } else {
                float stand_weight;
                float forward_speed;
                float sideways_speed;
                float player_speed_scale;

                player_info = (GlobalsPlayerInformation *)global_globals->player_information.pointer;

                if (cinematic_globals_ptr[9] != 0 || test_flag(biped_flags, tags::biped_tag_flag::unit_uses_old_ntsc_player_physics)) {
                    player_info_copy = *player_info;
                    player_info = &player_info_copy;
                    player_info->walking_speed = 0.512f;
                    player_info->run_forward = 2.25f;
                    player_info->run_backward = 2.0f;
                    player_info->run_sideways = 2.0f;
                    player_info->run_acceleration = 0.32f;
                }
                if (halo::game::globals().current_engine != 0) {
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
                    player_speed_scale = halo::game::player_at(unit->controlling_player)->speed;
                }
                speed_scale = (1.0f - player_info->stun_movement_penalty * unit->stun) *
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

                solve.movement_delta.i = speed_scale * forward_speed * unit->throttle.i * halo::math::k_seconds_per_tick;
                solve.movement_delta.j = speed_scale * unit->throttle.j * sideways_speed * halo::math::k_seconds_per_tick;
                solve.maximum_acceleration = (stand_weight * player_info->run_acceleration +
                                              player_info->sneak_acceleration * biped->crouch_fraction) *
                                             halo::math::k_seconds_per_tick;
                solve.airborne_acceleration = player_info->airborne_acceleration * halo::math::k_seconds_per_tick;

                if ((unit->control_flags & _unit_control_flag_look_dont_turn) == 0) {
                    solve.facing = unit->desired_facing_vector;
                }

                if (object_update_gate_globals[2] != 0) {
                    solve.movement_delta.k = player_info->double_speed_multiplier;
                    solve.movement_delta.i = solve.movement_delta.i * solve.movement_delta.k;
                    solve.movement_delta.j = solve.movement_delta.j * solve.movement_delta.k;
                    solve.movement_delta.k = solve.movement_delta.k * 0.0f;
                }
            }
        } else {
            float throttle_length;
            float crouch_modifier;
            float sideways_rate;

            throttle_length = (float)halo::libm::sqrt((double)(unit->throttle.k * unit->throttle.k +
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
                                     unit->throttle.i * halo::math::k_seconds_per_tick;
            solve.movement_delta.j = sideways_rate * unit->throttle.j * halo::math::k_seconds_per_tick;
            solve.movement_delta.k = sideways_rate * unit->throttle.k * halo::math::k_seconds_per_tick;
            solve.maximum_acceleration = ((1.0f - throttle_length) * tag->deceleration +
                                          throttle_length * tag->acceleration) *
                                         crouch_modifier * speed_scale * halo::math::k_seconds_per_tick;
            solve.airborne_acceleration = solve.maximum_acceleration;
        }

        if (test_flag(biped->flags, units::biped_flag::airborne) && biped->airborne_ticks < 0x16 &&
            unit->actor_index != k_datum_index_none && halo::ai::actor_check_vehicle_mode_timeout(unit->actor_index) != 0) {
            solve.steep_landing_maximum_slide = 0.1f;
            solve.steep_landing_minimum_penetration = 0.5f;
        }
        solve.frozen_fraction = 0.0f;
    }
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

        if (test_flag(biped_state, units::biped_flag::airborne)) solve.flags |= _biped_movement_solver_airborne;
        if (test_flag(biped_state, units::biped_flag::jumping)) solve.flags |= _biped_movement_solver_jumping;
        if (test_flag(biped_state, units::biped_flag::absolute_movement)) solve.flags |= _biped_movement_solver_absolute_movement;
        if (test_flag(biped_state, units::biped_flag::no_collision)) solve.flags |= _biped_movement_solver_no_collision;
        if (dead != 0) solve.flags |= _biped_movement_solver_dead;

        biped_flags = tag->biped_flags;
        if (test_flag(biped_flags, tags::biped_tag_flag::flying) && dead == 0) {
            solve.flags |= _biped_movement_solver_flying;
        }
        if (test_flag(biped_flags, tags::biped_tag_flag::passes_through_other_bipeds)) {
            solve.flags |= _biped_movement_solver_passes_through_bipeds;
        }
        if (test_flag(biped_flags, tags::biped_tag_flag::can_climb_any_surface) && dead == 0) {
            solve.flags |= _biped_movement_solver_climbs_any_surface;
        }
    }

    ::halo::units::biped_movement_solve(&solve);

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

    if ((unit->flags & _unit_flag_suspended) != 0) {
        solve.result_velocity.i = global_origin3d_pointer->x;
        solve.result_velocity.j = global_origin3d_pointer->y;
        solve.result_velocity.k = global_origin3d_pointer->z;
        solve.result_flags = solve.result_flags & 0xfe;
        solve.result_position = solve.start_position;
    }

    if (!test_flag(tag->biped_flags, tags::biped_tag_flag::physics_pill_centered_at_origin)) {
        solve.result_position.z = solve.result_position.z - solve.pill_radius;
    }

    obj->position = solve.result_position;
    obj->velocity = solve.result_velocity;
    biped->ground_surface_index = solve.result_ground_surface_index;
    biped->cached_ground_point.x = solve.result_position.x;
    biped->cached_ground_point.y = solve.result_position.y;
    biped->cached_ground_point.z = solve.result_position.z;
    biped->cached_ground_surface_index = k_datum_index_none;

    if (state[1] == 0 && (solve.result_flags & _biped_movement_result_landed) != 0) {
        state[1] = 1;
    }

    biped->flags = ((solve.result_flags & _biped_movement_result_airborne) == 0)
                       ? (biped->flags & ~halo::to_bits(units::biped_flag::airborne))
                       : (biped->flags | halo::to_bits(units::biped_flag::airborne));
    biped->flags = ((solve.result_flags & _biped_movement_result_jumping) == 0)
                       ? (biped->flags & ~halo::to_bits(units::biped_flag::jumping))
                       : (biped->flags | halo::to_bits(units::biped_flag::jumping));
    biped->flags = (!test_flag(tag->biped_flags, tags::biped_tag_flag::passes_through_other_bipeds))
                       ? (biped->flags & ~halo::to_bits(units::biped_flag::passes_through_bipeds))
                       : (biped->flags | halo::to_bits(units::biped_flag::passes_through_bipeds));

    biped->ground_normal = solve.ground_normal;
    biped->ground_plane_distance = solve.ground_plane;

    if (0.0f < solve.result_impact_speed) {
        ::halo::units::biped_update_animation_frame_trigger(solve.result_impact_speed, tag, obj);
    }

    if ((solve.flags & _biped_movement_solver_flying) == 0 && !test_flag(biped->flags, units::biped_flag::airborne)) {
        obj->flags = obj->flags | halo::to_bits(objects::object_flag::on_ground);
    } else {
        obj->flags = obj->flags & ~halo::to_bits(objects::object_flag::on_ground);
    }

    if ((int16_t)(solve.flags & _biped_movement_solver_flying) != 0 || test_flag(biped->flags, units::biped_flag::airborne) ||
        (solve.result_flags & _biped_movement_result_moving) != 0 ||
        0.0001f <= obj->velocity.k * obj->velocity.k + obj->velocity.j * obj->velocity.j +
                       obj->velocity.i * obj->velocity.i) {
        obj->flags = obj->flags & ~halo::to_bits(objects::object_flag::at_rest);
    } else {
        obj->flags = obj->flags | halo::to_bits(objects::object_flag::at_rest);
    }

    if (test_flag(obj->flags, objects::object_flag::on_ground)) {
        obj->angular_velocity.i = global_origin3d_pointer->x;
        obj->angular_velocity.j = global_origin3d_pointer->y;
        obj->angular_velocity.k = global_origin3d_pointer->z;
    }
}

/**
 * Integrates one tick of a live biped's movement, resolves it against the BSP, and applies everything that
 * falls out of the result. The displacement comes from the same three sources biped_integrate_movement
 * documents (the current animation's frame_info, the GlobalsPlayerInformation player-physics speeds, or the
 * Biped tag's own flight speeds), and the same biped_movement_solver_data goes to the object movement solver.
 *
 * Original register convention: see file header.
 *
 * @address 0x55cfd0
 */
void BipedView::integrate_movement_with_collision(int8_t *state)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    biped_data *biped = halo::units::biped_data_of(obj);
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    biped_movement_solver_data solve;
    GlobalsPlayerInformation player_info_copy;
    GlobalsPlayerInformation *player_info;
    float speed_scale;
    float dyaw;
    float crouch_step;
    uint32_t biped_flags;
    uint8_t result_flags;

    if (test_flag(obj->flags, objects::object_flag::at_rest) && (obj->vitality_flags & _object_health_frozen_bit) != 0 &&
        (unit->animation_state_flags & _unit_animation_flag_unknown_4) != 0) {
        return;
    }

    solve.object_index = object_index;
    solve.facing = obj->forward;
    solve.flags = solve.flags & 0xffff0000;

    if (!test_flag(((Unit *)tag)->unit_flags, tags::unit_tag_flag::simple_creature)) {
        solve.aiming = unit->aiming_vector;
    } else {
        solve.aiming = obj->forward;
    }

    solve.velocity = obj->velocity;
    solve.height_change = 0.0f;
    solve.maximum_acceleration = 0.0053333333f;
    solve.airborne_acceleration = 0.0f;
    ::halo::units::unit_get_crouch_height_offset(&solve.start_position, object_index, &solve.pill_height, &solve.pill_radius);

    solve.cosine_maximum_slope_angle = tag->cosine_maximum_slope_angle;
    solve.negative_sine_downhill_falloff_angle = tag->negative_sine_downhill_falloff_angle;
    solve.negative_sine_downhill_cutoff_angle = tag->negative_sine_downhill_cutoff_angle;
    solve.downhill_velocity_scale = tag->downhill_velocity_scale;
    solve.sine_uphill_falloff_angle = tag->sine_uphill_falloff_angle;
    solve.sine_uphill_cutoff_angle = tag->sine_uphill_cutoff_angle;
    solve.uphill_velocity_scale = tag->uphill_velocity_scale;
    solve.ground_normal = biped->ground_normal;
    solve.ground_plane = biped->ground_plane_distance;
    solve.ground_surface_index = biped->ground_surface_index;
    solve.steep_landing_maximum_slide = 3.4028235e+38f;
    solve.steep_landing_minimum_penetration = 0.0f;

    if (biped->landing_type == 1) {
        solve.movement_delta.i = 0.0f;
        solve.movement_delta.j = 0.0f;
        solve.movement_delta.k = 0.0f;
        solve.frozen_fraction = 1.0f;
    } else {
        biped_flags = tag->biped_flags;
        speed_scale = 1.0f;
        if (test_flag(biped_flags, tags::biped_tag_flag::random_speed_increase) && unit->aiming_speed == 0) {
            speed_scale = (float)(object_index % 0x89) * 0.00729927f * halo::game::weapon_get_zoom_fov(8, halo::main::globals().game_globals->difficulty) + 1.0f;
        }

        if (!test_flag(biped_flags, tags::biped_tag_flag::flying) ||
            (obj->vitality_flags & _object_health_frozen_bit) != 0) {
            if (unit->throttle.i != 0.0f || unit->throttle.j != 0.0f || unit->throttle.k != 0.0f) {
                uint8_t hurt = (0.2f < unit->stun);
                if (0.0f < ((Unit *)tag)->stunned_movement_threshold &&
                    ((Unit *)tag)->stunned_movement_threshold < obj->recent_body_damage) {
                    hurt = 1;
                }
                if ((float)halo::libm::fabs((double)unit->throttle.j) <= (float)halo::libm::fabs((double)unit->throttle.i)) {
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

        if (obj->animation_index != -1 &&
            ((obj->vitality_flags & _object_health_frozen_bit) != 0 ||
             !test_flag(tag->biped_flags, tags::biped_tag_flag::flying)) &&
            (unit->animation_state_flags & _unit_animation_flag_unknown_4) == 0) {
            ModelAnimationsAnimation *animation = &halo::objects::block_element<ModelAnimationsAnimation>(
                halo::objects::tag_as<ModelAnimations>(obj->animation_graph)->animations, obj->animation_index);
            float *frame_info = reinterpret_cast<float *>(static_cast<uintptr_t>(animation->frame_info.pointer));

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

            if (!((float)halo::libm::fabs((double)dyaw) < 0.0001f)) {
                real_vector3d new_forward = obj->forward;
                float dyaw_cos = (float)halo::libm::cos((double)dyaw);
                float dyaw_sin = (float)halo::libm::sin((double)dyaw);

                halo::math::vector3d_rotate_about_axis(new_forward, obj->up, dyaw_sin, dyaw_cos);

                if ((unit->control_flags & _unit_control_flag_exact_facing) != 0 &&
                    (unit->animation_state == _unit_animation_state_unknown_02 ||
                     unit->animation_state == _unit_animation_state_unknown_03) &&
                    0.5f < unit->desired_facing_vector.i * obj->forward.i +
                               unit->desired_facing_vector.j * obj->forward.j +
                               unit->desired_facing_vector.k * obj->forward.k) {
                    real_vector3d before;
                    real_vector3d after;

                    halo::math::vector3d_cross_product(before, obj->forward, unit->desired_facing_vector);
                    halo::math::vector3d_cross_product(after, new_forward, unit->desired_facing_vector);
                    if ((before.i * obj->up.i + before.k * obj->up.k + before.j * obj->up.j) *
                            (after.i * obj->up.i + after.k * obj->up.k + after.j * obj->up.j) <= 0.0f) {
                        new_forward = unit->desired_facing_vector;
                        UnitView(object_index).try_set_animation_state(animation_state_value(unit_animation_state_id::idle));
                    }
                }

                obj->forward = new_forward;
                ::halo::units::unit_update_up_vector((Biped *)tag, (::object *)obj);
            }
        }

        biped_flags = tag->biped_flags;
        if (!test_flag(biped_flags, tags::biped_tag_flag::flying) ||
            (obj->vitality_flags & _object_health_frozen_bit) != 0) {
            if (!test_flag(biped_flags, tags::biped_tag_flag::uses_player_physics) ||
                unit->animation_state == _unit_animation_state_custom_animation) {
                if (!test_flag(biped->flags, units::biped_flag::airborne | units::biped_flag::jumping)) {
                    obj->velocity.i = solve.movement_delta.i;
                    obj->velocity.j = solve.movement_delta.j;
                    solve.maximum_acceleration = 3.4028235e+38f;
                }
            } else {
                float stand_weight;
                float forward_speed;
                float sideways_speed;
                float player_speed_scale;

                player_info = (GlobalsPlayerInformation *)global_globals->player_information.pointer;

                if (cinematic_globals_ptr[9] != 0 || test_flag(biped_flags, tags::biped_tag_flag::unit_uses_old_ntsc_player_physics)) {
                    player_info_copy = *player_info;
                    player_info = &player_info_copy;
                    player_info->walking_speed = 0.512f;
                    player_info->run_forward = 2.25f;
                    player_info->run_backward = 2.0f;
                    player_info->run_sideways = 2.0f;
                    player_info->run_acceleration = 0.32f;
                }
                if (halo::game::globals().current_engine != 0) {
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
                    player_speed_scale = halo::game::player_at(unit->controlling_player)->speed;
                }
                speed_scale = (1.0f - player_info->stun_movement_penalty * unit->stun) *
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

                solve.movement_delta.i = speed_scale * unit->throttle.i * forward_speed * halo::math::k_seconds_per_tick;
                solve.movement_delta.j = speed_scale * unit->throttle.j * sideways_speed * halo::math::k_seconds_per_tick;
                solve.maximum_acceleration = (player_info->sneak_acceleration * biped->crouch_fraction +
                                              stand_weight * player_info->run_acceleration) *
                                             halo::math::k_seconds_per_tick;
                solve.airborne_acceleration = player_info->airborne_acceleration * halo::math::k_seconds_per_tick;

                if ((unit->control_flags & _unit_control_flag_look_dont_turn) == 0) {
                    solve.facing = unit->desired_facing_vector;
                }

                if (object_update_gate_globals[2] != 0) {
                    solve.movement_delta.k = player_info->double_speed_multiplier;
                    solve.movement_delta.i = solve.movement_delta.i * solve.movement_delta.k;
                    solve.movement_delta.j = solve.movement_delta.j * solve.movement_delta.k;
                    solve.movement_delta.k = solve.movement_delta.k * 0.0f;
                }
            }
        } else {
            float throttle_length;
            float crouch_modifier;

            throttle_length = (float)halo::libm::sqrt((double)(unit->throttle.k * unit->throttle.k +
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
                                     unit->throttle.i * halo::math::k_seconds_per_tick;
            solve.movement_delta.j = crouch_modifier * tag->max_sidestep_velocity * speed_scale *
                                     unit->throttle.j * halo::math::k_seconds_per_tick;
            solve.movement_delta.k = crouch_modifier * tag->max_sidestep_velocity * speed_scale *
                                     unit->throttle.k * halo::math::k_seconds_per_tick;
            solve.maximum_acceleration = ((1.0f - throttle_length) * tag->deceleration +
                                          throttle_length * tag->acceleration) *
                                         crouch_modifier * speed_scale * halo::math::k_seconds_per_tick;
            solve.airborne_acceleration = solve.maximum_acceleration;
        }

        if (test_flag(biped->flags, units::biped_flag::airborne) && biped->airborne_ticks < 0x16 &&
            unit->actor_index != k_datum_index_none && halo::ai::actor_check_vehicle_mode_timeout(unit->actor_index) != 0) {
            solve.steep_landing_maximum_slide = 0.1f;
            solve.steep_landing_minimum_penetration = 0.5f;
        }
        solve.frozen_fraction = 0.0f;
    }
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
    if (0.01f < (float)halo::libm::fabs((double)crouch_step) && test_flag(biped->flags, units::biped_flag::airborne)) {
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

        if (test_flag(biped_state, units::biped_flag::airborne)) solve.flags |= _biped_movement_solver_airborne;
        if (test_flag(biped_state, units::biped_flag::jumping)) solve.flags |= _biped_movement_solver_jumping;
        if (test_flag(biped_state, units::biped_flag::absolute_movement)) solve.flags |= _biped_movement_solver_absolute_movement;
        if (test_flag(biped_state, units::biped_flag::no_collision)) solve.flags |= _biped_movement_solver_no_collision;
        if (dead != 0) solve.flags |= _biped_movement_solver_dead;

        if (test_flag(tag->biped_flags, tags::biped_tag_flag::flying) && dead == 0) {
            solve.flags |= _biped_movement_solver_flying;
        }
        if (test_flag(tag->biped_flags, tags::biped_tag_flag::passes_through_other_bipeds)) {
            solve.flags |= _biped_movement_solver_passes_through_bipeds;
        }
        if (test_flag(tag->biped_flags, tags::biped_tag_flag::can_climb_any_surface) && dead == 0) {
            solve.flags |= _biped_movement_solver_climbs_any_surface;
        }
    }

    ::halo::units::biped_movement_solve(&solve);

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

    if ((unit->flags & _unit_flag_suspended) != 0) {
        solve.velocity.i = global_origin3d_pointer->x;
        solve.velocity.j = global_origin3d_pointer->y;
        solve.velocity.k = global_origin3d_pointer->z;
        solve.result_flags = solve.result_flags & 0xfe;
        solve.result_position = solve.start_position;
        solve.result_velocity = solve.velocity;
    }

    {
        real_point3d final_position = solve.result_position;

        if (!test_flag(tag->biped_flags, tags::biped_tag_flag::physics_pill_centered_at_origin)) {
            final_position.z = solve.result_position.z - solve.pill_radius;
        }

        halo::objects::object_unlink_cluster_or_notify_parent(object_index);
        obj->position = final_position;
        halo::objects::object_set_cluster_and_parent(object_index, 0);

        result_flags = solve.result_flags;
        obj->velocity = solve.result_velocity;
        biped->cached_ground_point.x = final_position.x;
        biped->ground_surface_index = solve.result_ground_surface_index;
        biped->cached_ground_point.y = final_position.y;
        biped->cached_ground_surface_index = k_datum_index_none;
        biped->cached_ground_point.z = final_position.z;
    }

    if (state[1] == 0 && (solve.result_flags & _biped_movement_result_landed) != 0) {
        state[1] = 1;
    }

    biped->flags = ((solve.result_flags & _biped_movement_result_airborne) == 0)
                       ? (biped->flags & ~halo::to_bits(units::biped_flag::airborne))
                       : (biped->flags | halo::to_bits(units::biped_flag::airborne));
    biped->flags = ((solve.result_flags & _biped_movement_result_jumping) == 0)
                       ? (biped->flags & ~halo::to_bits(units::biped_flag::jumping))
                       : (biped->flags | halo::to_bits(units::biped_flag::jumping));
    biped->flags = (!test_flag(tag->biped_flags, tags::biped_tag_flag::passes_through_other_bipeds))
                       ? (biped->flags & ~halo::to_bits(units::biped_flag::passes_through_bipeds))
                       : (biped->flags | halo::to_bits(units::biped_flag::passes_through_bipeds));

    biped->ground_normal = solve.ground_normal;
    biped->ground_plane_distance = solve.ground_plane;

    if (0.0f < solve.result_impact_speed) {
        ::halo::units::biped_update_animation_frame_trigger(solve.result_impact_speed, tag, obj);
    }
    if ((result_flags & _biped_movement_result_landed) == 0) {
        UnitView(object_index).track_target_lock_timeout();
    }

    if (unit->melee_state == _unit_melee_state_unknown_3 &&
        biped->melee_target_index != k_datum_index_none) {
        datum_index target_index = biped->melee_target_index;
        object *target = reinterpret_cast<object *>(halo::objects::object_record_bytes(target_index));
        real_vector3d lunge;
        object_collision_context context;
        object_node_collision_result node_hit;
        collision_result structure_hit;

        lunge.i = solve.result_position.x - solve.start_position.x;
        lunge.j = solve.result_position.y - solve.start_position.y;
        lunge.k = solve.result_position.z - solve.start_position.z;
        if (halo::math::ray_intersects_sphere_test(solve.start_position, target->bounding_center, lunge,
                                       target->bounding_radius) &&
            halo::physics::object_collision_context_build(target_index, &context) &&
            halo::physics::object_collision_context_test_segment(&context, 3, &solve.start_position, &lunge, &node_hit) &&
            !halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::structure_bsp | halo::collision_test_flag::nearby_objects | halo::collision_test_flag::object_vehicle | halo::collision_test_flag::object_scenery | halo::collision_test_flag::object_machine), &solve.start_position, &lunge, object_index, &structure_hit)) {
            real_point3d contact_point;
            real_plane3d contact_plane;

            contact_point.x = lunge.i * node_hit.segment.t + solve.start_position.x;
            contact_point.y = lunge.j * node_hit.segment.t + solve.start_position.y;
            contact_point.z = lunge.k * node_hit.segment.t + solve.start_position.z;
            halo::math::matrix4x3_transform_plane(contact_plane,
                                      static_cast<real_matrix4x3 *>(context.nodes)[node_hit.node_index],
                                      *static_cast<real_plane3d *>(node_hit.segment.plane));
            if (node_hit.segment.plane_index < 0) {
                contact_plane.normal.i = -contact_plane.normal.i;
                contact_plane.normal.j = -contact_plane.normal.j;
                contact_plane.normal.k = -contact_plane.normal.k;
                contact_plane.d = -contact_plane.d;
            }
            ::halo::units::unit_process_melee_special_interaction(object_index, target_index, node_hit.node_index, node_hit.region_index, node_hit.segment.material_index, &contact_point, &contact_plane, &structure_hit.leaf);
        }
    }

    ::halo::units::biped_update_target_lock_timer(solve.fastest_contact_object, object_index);
    UnitView(object_index).apply_fall_damage(solve.result_impact_speed);

    if ((solve.flags & _biped_movement_solver_flying) == 0 && !test_flag(biped->flags, units::biped_flag::airborne)) {
        obj->flags = obj->flags | halo::to_bits(objects::object_flag::on_ground);
    } else {
        obj->flags = obj->flags & ~halo::to_bits(objects::object_flag::on_ground);
    }

    if ((int16_t)(solve.flags & _biped_movement_solver_flying) != 0 || test_flag(biped->flags, units::biped_flag::airborne) ||
        (result_flags & _biped_movement_result_moving) != 0 ||
        0.0001f <= obj->velocity.k * obj->velocity.k + obj->velocity.j * obj->velocity.j +
                       obj->velocity.i * obj->velocity.i) {
        obj->flags = obj->flags & ~halo::to_bits(objects::object_flag::at_rest);
    } else {
        obj->flags = obj->flags | halo::to_bits(objects::object_flag::at_rest);
    }

    if (test_flag(obj->flags, objects::object_flag::on_ground)) {
        obj->angular_velocity.i = global_origin3d_pointer->x;
        obj->angular_velocity.j = global_origin3d_pointer->y;
        obj->angular_velocity.k = global_origin3d_pointer->z;
    }
}

/**
 * Engine function biped_movement_solve.
 *
 * @address 0x55efd0
 */
void biped_movement_solve(biped_movement_solver_data *solve)
{
    uint16_t flags = (uint16_t)solve->flags;
    uint8_t &result_flags = solve->result_flags;
    uint8_t climbs_any_surface = (uint8_t)((flags >> 9) & 1);
    float lateral_x = 0.0f;
    float lateral_y = 0.0f;
    real_vector3d a;
    real_vector3d b;
    real_vector3d c;
    real_vector3d direction;
    real_vector3d e;
    float speed;
    float one_minus_frozen;
    int16_t contact_count;
    real_point3d swept_position;
    real_vector3d swept_velocity;
    physics_model_contact contacts[16];
    physics_model probe_model;

    result_flags = 0;

    if (test_flag(flags, units::biped_movement_solver_flag::flying)) {
        real_vector3d world;
        float length;

        halo::math::real_matrix4x3_rotation_from_forward(&solve->facing, &a, &b);
        world.i = a.i * solve->movement_delta.j + solve->facing.i * solve->movement_delta.i +
                  b.i * solve->movement_delta.k;
        world.j = solve->facing.j * solve->movement_delta.i + a.j * solve->movement_delta.j +
                  b.j * solve->movement_delta.k;
        world.k = solve->facing.k * solve->movement_delta.i + a.k * solve->movement_delta.j +
                  b.k * solve->movement_delta.k;
        one_minus_frozen = 1.0f - solve->frozen_fraction;
        a.i = one_minus_frozen * world.i - solve->velocity.i;
        a.j = one_minus_frozen * world.j - solve->velocity.j;
        a.k = one_minus_frozen * world.k - solve->velocity.k;
        b = a;
        length = halo::math::vector3d_normalize_with_length(b);
        if (length > solve->maximum_acceleration) {
            a.i = b.i * solve->maximum_acceleration;
            a.j = b.j * solve->maximum_acceleration;
            a.k = b.k * solve->maximum_acceleration;
        }
        solve->result_velocity.i = a.i + solve->velocity.i;
        solve->result_velocity.j = a.j + solve->velocity.j;
        solve->result_velocity.k = a.k + solve->velocity.k;
        result_flags = static_cast<uint8_t>((result_flags & 0xfd) | 1);
    } else if (test_flag(flags, units::biped_movement_solver_flag::absolute_movement)) {
        solve->result_velocity.k = solve->movement_delta.k;
        lateral_x = solve->facing.i * solve->movement_delta.i - solve->movement_delta.j * solve->facing.j;
        solve->result_velocity.i = lateral_x;
        lateral_y = solve->movement_delta.j * solve->facing.i + solve->movement_delta.i * solve->facing.j;
        solve->result_velocity.j = lateral_y;
    } else if (test_flag(flags, units::biped_movement_solver_flag::airborne)) {
        real_vector2d delta2;
        float rotated_x = solve->facing.i * solve->movement_delta.i - solve->movement_delta.j * solve->facing.j;
        float rotated_y = solve->movement_delta.j * solve->facing.i + solve->movement_delta.i * solve->facing.j;
        float dx, dy, length;

        one_minus_frozen = 1.0f - solve->frozen_fraction;
        dx = one_minus_frozen * rotated_x - solve->velocity.i;
        dy = one_minus_frozen * rotated_y - solve->velocity.j;
        delta2.i = dx;
        delta2.j = dy;
        length = halo::math::vector2d_normalize_with_length(delta2);
        if (length > solve->airborne_acceleration) {
            dx = delta2.i * solve->airborne_acceleration;
            dy = solve->airborne_acceleration * delta2.j;
        }
        direction.i = delta2.i;
        direction.j = delta2.j;
        result_flags = static_cast<uint8_t>(flags & 2);
        solve->result_velocity.i = dx + solve->velocity.i;
        solve->result_velocity.j = dy + solve->velocity.j;
        solve->result_velocity.k = solve->velocity.k - halo::physics::globals().gravity;
    } else {
        real_vector3d *ground_normal = &solve->ground_normal;
        uint8_t jumping = 0;
        real_vector3d delta;
        float length;
        float scaled;

        speed = (float)halo::libm::sqrt((double)(solve->movement_delta.i * solve->movement_delta.i +
                                     solve->movement_delta.j * solve->movement_delta.j +
                                     solve->movement_delta.k * solve->movement_delta.k));
        if (test_flag(flags, units::biped_movement_solver_flag::climbs_any_surface)) {
            b = solve->aiming;
            halo::math::vector3d_cross_product(c, b, *ground_normal);
            if (halo::math::vector3d_normalize_with_length(c) == 0.0f) {
                halo::math::vector3d_cross_product(c, *halo::math::globals().global_up3d_pointer, *ground_normal);
                if (halo::math::vector3d_normalize_with_length(c) == 0.0f) {
                    halo::math::vector3d_cross_product(c, *halo::math::globals().global_forward3d_pointer, *ground_normal);
                    halo::math::vector3d_normalize_with_length(c);
                }
            }
            halo::math::vector3d_cross_product(b, *ground_normal, c);
            halo::math::vector3d_normalize_with_length(b);
            direction.i = c.i * solve->movement_delta.j + b.i * solve->movement_delta.i;
            direction.j = c.j * solve->movement_delta.j + b.j * solve->movement_delta.i;
            direction.k = c.k * solve->movement_delta.j + b.k * solve->movement_delta.i + solve->movement_delta.k;
        } else if (ground_normal->k > k_flat_ground_k) {
            lateral_x = solve->facing.i * solve->movement_delta.i - solve->movement_delta.j * solve->facing.j;
            direction.i = lateral_x;
            lateral_y = solve->movement_delta.j * solve->facing.i + solve->movement_delta.i * solve->facing.j;
            direction.j = lateral_y;
            direction.k = solve->movement_delta.k -
                          (lateral_y * ground_normal->j + lateral_x * ground_normal->i) / ground_normal->k;
        } else {
            c = solve->aiming;
            halo::math::vector3d_cross_product(e, solve->aiming, *halo::math::globals().global_up3d_pointer);
            halo::math::vector3d_normalize_with_length(e);
            halo::math::point3d_add_scaled(*((real_point3d *)&c), *ground_normal, *((real_point3d *)&c),
                -(c.k * ground_normal->k + c.j * ground_normal->j + c.i * ground_normal->i));
            halo::math::point3d_add_scaled(*((real_point3d *)&e), *ground_normal, *((real_point3d *)&e),
                -(e.k * ground_normal->k + e.j * ground_normal->j + e.i * ground_normal->i));
            lateral_x = solve->facing.i * solve->movement_delta.i - solve->movement_delta.j * solve->facing.j;
            lateral_y = solve->movement_delta.j * solve->facing.i + solve->movement_delta.i * solve->facing.j;
            direction.i = e.i * solve->movement_delta.j + c.i * solve->movement_delta.i;
            direction.j = c.j * solve->movement_delta.i + e.j * solve->movement_delta.j;
            direction.k = c.k * solve->movement_delta.i + e.k * solve->movement_delta.j + solve->movement_delta.k;
            if (climbs_any_surface == 0) {
                direction.k = direction.k * 5.0f;
            }
        }
        halo::math::vector3d_normalize_with_length(direction);

        scaled = speed;
        if (!test_flag(flags, units::biped_movement_solver_flag::climbs_any_surface)) {
            float z = direction.k;
            if (z <= solve->negative_sine_downhill_cutoff_angle) {
                scaled = speed * solve->downhill_velocity_scale;
            } else if (z < solve->negative_sine_downhill_falloff_angle) {
                scaled = ((solve->downhill_velocity_scale - 1.0f) * (z - solve->negative_sine_downhill_falloff_angle)) /
                         (solve->negative_sine_downhill_cutoff_angle - solve->negative_sine_downhill_falloff_angle) + 1.0f;
                scaled = scaled * speed;
            } else if (z >= solve->sine_uphill_cutoff_angle) {
                scaled = speed * solve->uphill_velocity_scale;
            } else if (z > solve->sine_uphill_falloff_angle) {
                scaled = ((solve->uphill_velocity_scale - 1.0f) * (z - solve->sine_uphill_falloff_angle)) /
                         (solve->sine_uphill_cutoff_angle - solve->sine_uphill_falloff_angle) + 1.0f;
                scaled = scaled * speed;
            }
        }
        scaled = (1.0f - solve->frozen_fraction) * scaled;

        delta.i = direction.i * scaled - solve->velocity.i;
        delta.j = direction.j * scaled - solve->velocity.j;
        delta.k = direction.k * scaled - solve->velocity.k;
        b = delta;
        length = halo::math::vector3d_normalize_with_length(b);
        if (length > solve->maximum_acceleration) {
            if (!test_flag(flags, units::biped_movement_solver_flag::climbs_any_surface)) {
                jumping = (uint8_t)((flags >> 1) & 1);
            }
            c.i = b.i * solve->maximum_acceleration;
            c.j = b.j * solve->maximum_acceleration;
            c.k = b.k * solve->maximum_acceleration;
        } else {
            c = delta;
        }
        result_flags = jumping ? 2 : 0;
        solve->result_velocity.i = (c.i - ground_normal->i * k_ground_normal_offset) + solve->velocity.i;
        solve->result_velocity.j = (c.j - ground_normal->j * k_ground_normal_offset) + solve->velocity.j;
        solve->result_velocity.k = (c.k - ground_normal->k * k_ground_normal_offset) + solve->velocity.k;
        if ((result_flags & 2) != 0) {
            solve->result_velocity.k -= halo::physics::globals().gravity;
        }
    }

    {
        uint32_t model_flags;
        real_vector3d delta;

        if (test_flag(flags, units::biped_movement_solver_flag::no_collision)) {
            model_flags = 0;
        } else if (test_flag(flags, units::biped_movement_solver_flag::dead)) {
            model_flags = k_dead_unit_collision_flags;
        } else if (test_flag(flags, units::biped_movement_solver_flag::passes_through_bipeds)) {
            model_flags = k_pass_through_bipeds_collision_flags;
        } else {
            model_flags = k_unit_collision_flags;
        }
        a = *(real_vector3d *)&solve->start_position;
        delta = solve->result_velocity;
        delta.k = delta.k + solve->height_change;
        e = delta;
        contact_count = halo::physics::physics_sweep_capsule_step((real_point3d *)&a, &e, &swept_velocity, solve->object_index,
            model_flags, solve->pill_height, solve->pill_radius, &swept_position, 16, contacts);
        if (contact_count < 16) {
            clear_flag(solve->result_flags, units::biped_movement_result_flag::unknown_8);
        } else {
            set_flag(solve->result_flags, units::biped_movement_result_flag::unknown_8);
        }
    }
    solve->snapped_ground_surface_index = k_datum_index_none;
    if (contact_count == 0 && solve->ground_surface_index != k_datum_index_none) {
        ModelCollisionGeometryBSP *bsp = global_structure_collision_bsp;
        int32_t surface_index = (int32_t)solve->ground_surface_index;
        int32_t best_surface = -1;
        float best_distance_squared = 3.4028235e+38f;
        float best_dot = 0.0f;
        real_plane3d best_plane;

        if (surface_index >= 0 && surface_index < (int32_t)bsp->surfaces.count) {
            ModelCollisionGeometryBSPSurface *surfaces = (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
            ModelCollisionGeometryBSPEdge *edges = (ModelCollisionGeometryBSPEdge *)bsp->edges.pointer;
            uint32_t start_edge = surfaces[surface_index].first_edge;
            uint32_t edge_index = start_edge;
            real_plane3d plane;
            float along;

            halo::structures::structure_bsp_plane_fetch_signed(&plane, bsp, (int32_t)surfaces[surface_index].plane);
            along = -((plane.normal.i * swept_position.x + plane.normal.k * swept_position.z +
                       plane.normal.j * swept_position.y) - plane.d);
            a.i = plane.normal.i * along + swept_position.x;
            a.j = plane.normal.j * along + swept_position.y;
            a.k = plane.normal.k * along + swept_position.z;

            do {
                ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
                uint8_t ours_on_right = solve->ground_surface_index == edge->right_surface;
                uint32_t other = ours_on_right ? edge->left_surface : edge->right_surface;

                if (other != k_datum_index_none &&
                    (test_flag(flags, units::biped_movement_solver_flag::climbs_any_surface) || (surfaces[other].flags & 4) != 0)) {
                    float dot;

                    halo::structures::structure_bsp_plane_fetch_signed(&plane, bsp, (int32_t)surfaces[other].plane);
                    dot = plane.normal.i * swept_velocity.i + plane.normal.j * swept_velocity.j +
                          plane.normal.k * swept_velocity.k;
                    if (dot > 0.0f &&
                        solve->pill_radius * -0.5f <
                            (plane.normal.i * swept_position.x + plane.normal.k * swept_position.z +
                             plane.normal.j * swept_position.y) - plane.d) {
                        ModelCollisionGeometryBSPVertex *vertices = (ModelCollisionGeometryBSPVertex *)bsp->vertices.pointer;
                        real_point3d *v0 = (real_point3d *)&vertices[edge->start_vertex].point;
                        real_point3d *v1 = (real_point3d *)&vertices[edge->end_vertex].point;
                        float t;
                        float dx, dy, dz, distance_squared;

                        c.i = v1->x - v0->x;
                        c.j = v1->y - v0->y;
                        c.k = v1->z - v0->z;
                        t = ((a.i - v0->x) * c.i + (a.k - v0->z) * c.k + (a.j - v0->y) * c.j) /
                            (c.i * c.i + c.k * c.k + c.j * c.j);
                        if (t < 0.0f) {
                            b = *(real_vector3d *)v0;
                        } else if (t > 1.0f) {
                            b = *(real_vector3d *)v1;
                        } else {
                            halo::math::point3d_add_scaled(*((real_point3d *)&b), c, *v0, t);
                        }
                        dx = b.i - a.i;
                        dy = b.j - a.j;
                        dz = b.k - a.k;
                        distance_squared = dx * dx + dy * dy + dz * dz;
                        if (distance_squared < best_distance_squared) {
                            best_distance_squared = distance_squared;
                            best_plane = plane;
                            best_surface = (int32_t)other;
                            best_dot = dot;
                        }
                    }
                }
                edge_index = ours_on_right ? edge->reverse_edge : edge->forward_edge;
            } while (edge_index != start_edge);
        }

        if (best_surface != -1) {
            float reach = solve->pill_radius + solve->pill_radius;
            if (!(best_distance_squared > reach * reach) && !(best_dot > 0.053333335f)) {
                float height = (swept_position.x * best_plane.normal.i + best_plane.normal.k * swept_position.z +
                                best_plane.normal.j * swept_position.y) - (best_plane.d + solve->pill_radius);
                if (!(solve->pill_radius * 0.5f < (float)halo::libm::fabs((double)height))) {
                    physics_model_contact *contact = &contacts[0];
                    float into = swept_velocity.i * best_plane.normal.i + best_plane.normal.j * swept_velocity.j +
                                 best_plane.normal.k * swept_velocity.k;

                    swept_position.x = best_plane.normal.i * -height + swept_position.x;
                    swept_position.y = best_plane.normal.j * -height + swept_position.y;
                    swept_position.z = best_plane.normal.k * -height + swept_position.z;
                    if (into > -halo::math::k_seconds_per_tick) {
                        float push = -(into + halo::math::k_seconds_per_tick);
                        swept_velocity.i = best_plane.normal.i * push + swept_velocity.i;
                        swept_velocity.j = best_plane.normal.j * push + swept_velocity.j;
                        swept_velocity.k = best_plane.normal.k * push + swept_velocity.k;
                    }
                    contact->t = 0.0f;
                    contact->point_x = best_plane.normal.i * -solve->pill_radius + swept_position.x;
                    contact->point_y = best_plane.normal.j * -solve->pill_radius + swept_position.y;
                    contact->point_z = best_plane.normal.k * -solve->pill_radius + swept_position.z;
                    contact->plane_i = best_plane.normal.i;
                    contact->plane_j = best_plane.normal.j;
                    contact->plane_k = best_plane.normal.k;
                    contact->plane_d = best_plane.d;
                    contact->object_index = k_datum_index_none;
                    contact->surface_index = best_surface;
                    contact->surface_flags = 0;
                    contact->breakable_surface_index = 0;
                    contact->material_type = -1;
                    contact_count = 1;
                    solve->snapped_ground_surface_index = (uint32_t)best_surface;
                }
            }
        }
    }

    {
        float lateral_squared = lateral_y * lateral_y + lateral_x * lateral_x;
        int16_t best = -1;
        uint8_t best_walkable = 0;
        uint8_t best_is_snap_surface = 0;
        float best_k = -3.4028235e+38f;
        float best_height = -3.4028235e+38f;
        uint8_t landed = 0;

        if (lateral_squared > 9.999999e-09f) {
            float inverse = (float)(1.0 / halo::libm::sqrt((double)lateral_squared));
            lateral_x = lateral_x * inverse;
            lateral_y = inverse * lateral_y;
        }

        if (!test_flag(flags, units::biped_movement_solver_flag::flying) && contact_count > 0) {
            uint8_t dead = test_flag(flags, units::biped_movement_solver_flag::dead);
            int16_t i;

            for (i = 0; i < contact_count; i++) {
                physics_model_contact *contact = &contacts[i];
                uint8_t walkable = !dead && (climbs_any_surface || (contact->surface_flags & 4) != 0);
                uint8_t is_snap_surface = solve->snapped_ground_surface_index != k_datum_index_none && best >= 0 &&
                                          (uint32_t)contacts[best].surface_index == solve->snapped_ground_surface_index;
                float height = -(solve->result_velocity.i * contact->plane_i + contact->plane_j * solve->result_velocity.j +
                                 contact->plane_k * solve->result_velocity.k);
                uint8_t take;

                if (walkable) {
                    take = (climbs_any_surface || !(lateral_y * contact->plane_j + lateral_x * contact->plane_i > 0.5f)) &&
                           (!best_walkable || is_snap_surface || (!best_is_snap_surface && height > best_height));
                } else {
                    take = !best_walkable && contact->plane_k > best_k;
                }
                if (take) {
                    best_walkable = walkable;
                    best = i;
                    best_is_snap_surface = is_snap_surface;
                    best_k = contact->plane_k;
                    best_height = height;
                }

                if ((result_flags & 0x10) == 0) {
                    uint8_t dynamic = (contact->surface_flags & 8) != 0;
                    if (!dynamic && contact->object_index != k_datum_index_none) {
                        uint8_t type = halo::objects::object_header_of(contact->object_index).type;
                        dynamic = ((halo::objects::object_type_mask_of(type)) & 0x40) == 0;
                    }
                    if (dynamic) {
                        result_flags = static_cast<uint8_t>(result_flags | 0x10);
                    }
                }
            }

            if (best != -1) {
                physics_model_contact *ground = &contacts[best];
                real_plane3d plane;
                float penetration;

                plane.normal.i = ground->plane_i;
                plane.normal.j = ground->plane_j;
                plane.normal.k = ground->plane_k;
                plane.d = ground->plane_d;
                penetration = -(plane.normal.j * e.j + plane.normal.k * e.k + plane.normal.i * e.i);
                landed = 1;
                if (!best_walkable && !best_is_snap_surface) {
                    if (!(best_k >= solve->cosine_maximum_slope_angle)) {
                        landed = 0;
                    } else if (test_flag(flags, units::biped_movement_solver_flag::airborne) && solve->steep_landing_maximum_slide < 3.4028235e+38f) {
                        real_vector3d projected;
                        float r = solve->steep_landing_maximum_slide;
                        projected.i = plane.normal.i * penetration + e.i;
                        projected.j = plane.normal.j * penetration + e.j;
                        projected.k = plane.normal.k * penetration + e.k;
                        if (r * r < projected.i * projected.i + projected.j * projected.j + projected.k * projected.k &&
                            penetration / halo::math::vector3d_length(e) < solve->steep_landing_minimum_penetration) {
                            landed = 0;
                        }
                    }
                }
                if (landed) {
                    uint32_t surface = (uint32_t)ground->surface_index;
                    clear_flag(solve->result_flags, units::biped_movement_result_flag::airborne);
                    solve->ground_normal = plane.normal;
                    solve->ground_plane = halo::bit_cast<uint32_t>(plane.d);
                    solve->result_ground_surface_index = surface;
                    if (surface != k_datum_index_none && surface == solve->snapped_ground_surface_index) {
                        solve->result_impact_speed = 0.0f;
                    } else {
                        solve->result_impact_speed = -(e.j * solve->ground_normal.j + e.k * solve->ground_normal.k +
                                                       e.i * solve->ground_normal.i);
                    }
                }
            }
        }
        if (!landed) {
            set_flag(solve->result_flags, units::biped_movement_result_flag::airborne);
            solve->ground_normal.i = k_default_resting_plane[0];
            solve->ground_normal.j = k_default_resting_plane[1];
            solve->ground_normal.k = k_default_resting_plane[2];
            solve->ground_plane = halo::bit_cast<uint32_t>(k_default_resting_plane[3]);
            solve->result_ground_surface_index = k_datum_index_none;
            solve->result_impact_speed = 0.0f;
        }
    }

    {
        uint32_t best_object = k_datum_index_none;
        int16_t best_type = 0;
        float best_relative_speed_squared = 0.0f;
        int16_t i;

        for (i = 0; i < contact_count; i++) {
            uint32_t object_index = contacts[i].object_index;
            object *obj;
            float dx, dy, dz, relative_speed_squared;
            uint8_t take;

            if (object_index == k_datum_index_none) {
                continue;
            }
            obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
            dx = obj->velocity.i - swept_velocity.i;
            dy = obj->velocity.j - swept_velocity.j;
            dz = obj->velocity.k - swept_velocity.k;
            relative_speed_squared = dx * dx + dy * dy + dz * dz;
            if (best_object == k_datum_index_none) {
                take = 1;
            } else if (best_type != _object_type_vehicle) {
                take = obj->type == _object_type_vehicle || relative_speed_squared > best_relative_speed_squared;
            } else {
                take = obj->type == _object_type_vehicle && relative_speed_squared > best_relative_speed_squared;
            }
            if (take) {
                best_type = obj->type;
                best_relative_speed_squared = relative_speed_squared;
                best_object = object_index;
            }
        }
        solve->fastest_contact_object = best_object;
        solve->result_surface_index = k_datum_index_none;

        for (i = 0; i < contact_count; i++) {
            uint32_t object_index = contacts[i].object_index;
            object_header *header = 0;
            int16_t index = (int16_t)object_index;
            int16_t salt = (int16_t)(object_index >> 16);

            if (object_index == k_datum_index_none) {
                continue;
            }
            if (index >= 0 && index < halo::objects::globals().object_data->maximum_count) {
                object_header *candidate = &halo::objects::object_header_of(static_cast<uint32_t>(index));
                if (candidate->identifier != 0 && (salt == 0 || candidate->identifier == salt)) {
                    header = candidate;
                }
            }
            if (header != 0 && (int8_t)(halo::objects::object_type_mask_of(header->type)) < 0 && header->data != 0) {
                Unit *tag = halo::objects::tag_as<Unit>(header->data->definition_tag);
                if (((uint8_t)(tag->melee_damage.path_size >> 16) & 4) != 0 && halo::raw_at<int16_t>(tag, 0x2ea) != -1) {
                    solve->result_surface_index = object_index;
                }
            }
        }

        solve->result_position = swept_position;
        solve->result_velocity = swept_velocity;
        {
            float dx = swept_velocity.i - e.i;
            float dy = swept_velocity.j - e.j;
            float dz = swept_velocity.k - e.k;
            solve->result_blocked_distance = (float)halo::libm::sqrt((double)(dx * dx + dy * dy + dz * dz));
        }
        solve->result_velocity.k = solve->result_velocity.k - solve->height_change;

        if (test_flag(flags, units::biped_movement_solver_flag::crouching) && test_flag(flags, units::biped_movement_solver_flag::crouch_began)) {
            object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(solve->object_index)].data;
            unit_data *unit = halo::units::unit_data_of(obj);

            if (unit->controlling_player != k_datum_index_none) {
                Biped *tag = halo::objects::tag_as<Biped>(obj->definition_tag);
                if (!test_flag(tag->biped_flags, halo::tags::biped_tag_flag::physics_pill_centered_at_origin | halo::tags::biped_tag_flag::spherical)) {
                    float half_height = tag->standing_collision_height * 0.5f;
                    uint32_t query_flags = test_flag(flags, units::biped_movement_solver_flag::dead) ? k_dead_unit_collision_flags : k_unit_collision_flags;
                    real_point3d center;

                    center.x = solve->result_position.x;
                    center.y = solve->result_position.y;
                    center.z = solve->result_position.z + half_height;
                    if (halo::physics::physics_model_build_from_sphere_query(query_flags, &center, half_height, 0.0f,
                                                              tag->collision_radius, solve->object_index, &probe_model)) {
                        float reach = tag->standing_collision_height - (tag->collision_radius + tag->collision_radius);
                        real_vector3d up_ray;
                        physics_model_contact probe_contact;

                        up_ray.i = reach * halo::math::globals().global_up3d_pointer->i;
                        up_ray.j = reach * halo::math::globals().global_up3d_pointer->j;
                        up_ray.k = reach * halo::math::globals().global_up3d_pointer->k;
                        if (halo::physics::physics_shape_test_ray(&probe_model, &solve->result_position, &up_ray, &probe_contact)) {
                            set_flag(solve->result_flags, units::biped_movement_result_flag::landed);
                        }
                    }
                }
            }
        }
    }
}

/**
 * object_type_definition "biped" row, +0x50 column. Clears the whole biped_data extension to zero, reseeds
 * its ground_normal/unknown_520 from the shared default resting plane, and marks unknown_4f8's rate-limit
 * stamp invalid.
 *
 * @address 0x559f10
 */
void BipedView::reset_state()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    biped_data *biped = halo::units::biped_data_of(obj);

    memset(biped, 0, 0x21 * sizeof(uint32_t));

    biped->ground_normal.i = k_default_resting_plane[0];
    biped->ground_normal.j = k_default_resting_plane[1];
    biped->ground_normal.k = k_default_resting_plane[2];
    biped->ground_plane_distance = halo::bit_cast<uint32_t>(k_default_resting_plane[3]);
    biped->last_falling_reaction_tick = -1;
}

}
