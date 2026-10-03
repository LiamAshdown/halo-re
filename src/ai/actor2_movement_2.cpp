#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/physics/api.hpp"

namespace halo::ai {

namespace actor_movement_choose_avoidance_direction_local {
static auto &global_structure_bsp = halo::link::ref<uint32_t>(halo::ai::vars().global_structure_bsp);
static auto &global_structure_collision_bsp = halo::link::ref<uint32_t>(halo::physics::vars().global_structure_collision_bsp);
static auto &global_origin3d_pointer = halo::link::ref<const real_vector3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &actor_avoidance_samples_a = halo::link::ref<float [16][7]>(halo::ai::vars().actor_avoidance_samples_a);
static auto &actor_avoidance_circle = halo::link::ref<float [8][3]>(halo::ai::vars().actor_avoidance_circle);
static auto &actor_avoidance_samples_b = halo::link::ref<float [9][7]>(halo::ai::vars().actor_avoidance_samples_b);
static auto &actor_avoidance_near_weights = halo::link::ref<const float [9][8]>(halo::ai::vars().actor_avoidance_near_weights);
static auto &actor_avoidance_ray_weights = halo::link::ref<const float [2]>(halo::ai::vars().actor_avoidance_ray_weights);
}

/**
 * The frame 0x4193d0 builds: F+0x20 result, F+0x3c closeness, F+0x58 the eight direction weights, F+0xa0 the
 * context handed to the obstacle helpers.
 *
 * @address 0x4193d0
 */
void ActorView::movement_choose_avoidance_direction(real_vector3d *desired, real_vector3d *out_direction, float *out_scale)
{
    using namespace actor_movement_choose_avoidance_direction_local;
    actor *act = halo::ai::actor_at(actor_index);
    const real_vector3d *zero = global_origin3d_pointer;
    real_vector3d result = *zero;
    float out = 0.0f;
    datum_index unit_index = act->active_unit_index;
    uint8_t *obj;
    actor_movement_context context;
    float weights[8];
    float closeness = 0.0f;
    real_vector3d elevation;
    real_point3d end_point;
    float distance;
    int16_t i;
    int16_t k;
    int16_t best;
    float best_weight;
    real_vector3d d;
    real_vector3d e;
    float forwardness;
    float along;
    float index_out;
    float delta;
    float scale;
    int16_t *best_saved = &act->avoidance_last_direction;
    int16_t *hold = &act->avoidance_turn_around_ticks;
    int16_t held;

    if (unit_index == k_datum_index_none) {
        unit_index = act->unit_index;
        if (unit_index == k_datum_index_none) {
            *out_direction = result;
            *out_scale = 0.0f;
            return;
        }
    }
    obj = (uint8_t *)halo::ai::object_at(unit_index);
    context.structure_bsp = global_structure_bsp;
    context.collision_bsp = global_structure_collision_bsp;
    context.unit_index = unit_index;
    halo::objects::object_get_position(&context.position, unit_index);
    context.forward = *(real_vector3d *)&((object *)obj)->forward.i;
    context.up = *(real_vector3d *)&((object *)obj)->up.i;
    context.left.i = context.forward.k * context.up.j - context.up.k * context.forward.j;
    context.left.j = context.up.k * context.forward.i - context.up.i * context.forward.k;
    context.left.k = context.up.i * context.forward.j - context.forward.i * context.up.j;
    context.search_radius = 12.0f;
    context.ray_scale = 1.0f;
    halo::ai::actor_movement_collect_obstacle_candidates(&context);

    for (i = 0; i < 8; i++) {
        weights[i] = 0.0f;
    }
    if (*best_saved >= 0 && *best_saved < 8) {
        int16_t b = *best_saved;

        weights[b] += 0.4f;
        weights[(b + 1) & 7] += 0.32f;
        weights[(b + 2) & 7] += 0.2f;
        weights[(b + 7) & 7] += 0.32f;
        weights[(b + 6) & 7] += 0.2f;
    }

    for (k = 0; k < 9; k++) {
        if (halo::ai::actor_movement_test_obstacle_ray(&elevation, actor_avoidance_samples_b[k], &end_point, &context, &distance,
                                             0) > 0) {
            float v = 1.0f - distance;
            float c = v + v;

            if (c > 1.0f) {
                c = 1.0f;
            }
            for (i = 0; i < 8; i++) {
                weights[i] += c * actor_avoidance_near_weights[k][i];
            }
            if (!(closeness > v)) {
                closeness = v;
            }
        }
    }

    for (k = 0; k < 8; k++) {
        int16_t hit[2];
        float ray_distance[2];
        float acc = 0.0f;
        uint8_t blocked = 0;
        int16_t j;

        for (j = 0; j < 2; j++) {
            hit[j] = halo::ai::actor_movement_test_obstacle_ray(&elevation, actor_avoidance_samples_a[k * 2 + j], &end_point,
                                                      &context, &ray_distance[j], (uint8_t *)act + 0x5c8 + k * 2 + j);
        }
        for (j = 1; j >= 0; j--) {
            float weight = actor_avoidance_ray_weights[j];

            if (hit[j] == 0) {
                if (blocked) {
                    acc += 1.0f * weight;
                } else {
                    uint8_t clear_ticks = ((uint8_t *)act)[0x5c8 + k * 2 + j];
                    float v = 0.0f;

                    if (clear_ticks >= 75) {
                        v = 1.0f - 75.0f / (float)clear_ticks;
                        if (!(v > 0.0f)) {
                            v = 0.0f;
                        } else if (v > 1.0f) {
                            v = 1.0f;
                        }
                    }
                    acc += v * weight;
                }
            } else {
                float v = (1.0f - ray_distance[j]) * 2.0f;

                if (v > 1.0f) {
                    v = 1.0f;
                }
                blocked = 1;
                acc -= v * weight;
            }
        }
        weights[k] += acc;
        weights[(k + 1) & 7] += acc * 0.8f;
        weights[(k + 2) & 7] += acc * 0.5f;
        weights[(k + 7) & 7] += acc * 0.8f;
        weights[(k + 6) & 7] += acc * 0.5f;
    }

    {
        float vx = ((object *)obj)->angular_velocity.i;
        float vy = ((object *)obj)->angular_velocity.j;
        float vz = ((object *)obj)->angular_velocity.k;
        float speed = (float)halo::libm::sqrt(vx * vx + vy * vy + vz * vz);

        if (speed > 0.02f) {
            float s = (speed - 0.02f) * 12.5f;
            real_vector3d motion;
            float length;

            if (s > 1.0f) {
                s = 1.0f;
            }
            s *= 0.8f;
            motion.i = 0.0f;
            motion.j = context.up.j * vy + context.up.k * vz + context.up.i * vx;
            motion.k = -(context.left.j * vy + context.left.k * vz + context.left.i * vx);
            length = (float)halo::libm::sqrt(motion.k * motion.k + motion.j * motion.j);
            if (halo::libm::fabs(length) >= 9.999999747378752e-05) {
                float inverse = 1.0f / length;
                float value;

                motion.i = 0.0f * inverse;
                motion.j *= inverse;
                motion.k *= inverse;
                if (length > 0.0f &&
                    halo::ai::actor_avoidance_interpolate_sample(&motion, (real_vector3d *)actor_avoidance_circle, 8, weights,
                                                       &index_out, &value) &&
                    value > 0.5f) {
                    for (i = 0; i < 8; i++) {
                        float dot = motion.k * actor_avoidance_circle[i][2] + motion.i * actor_avoidance_circle[i][0] +
                                    motion.j * actor_avoidance_circle[i][1];

                        if (dot < 0.0f) {
                            weights[i] += dot * s;
                        }
                    }
                }
            }
        }
    }

    best = -1;
    best_weight = -3.4028235e38f;
    for (i = 0; i < 8; i++) {
        if (weights[i] > best_weight) {
            best_weight = weights[i];
            best = i;
        }
    }

    d = *desired;
    e = *zero;
    forwardness = 1.0f;
    along = 0.0f;
    {
        float length = (float)halo::libm::sqrt(d.k * d.k + d.j * d.j + d.i * d.i);

        if (halo::libm::fabs(length) >= 9.999999747378752e-05) {
            float inverse = 1.0f / length;

            d.i *= inverse;
            d.j *= inverse;
            d.k *= inverse;
            if (length > 0.0f) {
                float length_2;

                e.i = 0.0f;
                forwardness = context.forward.k * d.k + context.forward.j * d.j + context.forward.i * d.i;
                e.j = d.j * context.left.j + d.k * context.left.k + d.i * context.left.i;
                e.k = d.j * context.up.j + d.k * context.up.k + d.i * context.up.i;
                length_2 = (float)halo::libm::sqrt(e.k * e.k + e.j * e.j);
                if (halo::libm::fabs(length_2) >= 9.999999747378752e-05) {
                    float inverse_2 = 1.0f / length_2;

                    e.i = 0.0f * inverse_2;
                    e.j *= inverse_2;
                    e.k *= inverse_2;
                    if (length_2 > 0.0f) {
                        halo::ai::actor_avoidance_interpolate_sample(&e, (real_vector3d *)actor_avoidance_circle, 8, weights,
                                                           &index_out, &along);
                    }
                }
            }
        }
    }
    delta = best_weight - along;
    if (closeness > 0.6f) {
        float t = (closeness - 0.6f) * 2.5f;

        scale = (1.0f > t ? t : 1.0f) + 1.0f;
    } else {
        float t = closeness * 3.3333333f;

        scale = 1.0f > t ? t : 1.0f;
    }

    held = *hold;
    if (forwardness < -0.2f) {
        uint8_t turn_around = 0;

        if (held != -1 && held < 90) {
            turn_around = 1;
        } else {
            float vx = ((object *)obj)->angular_velocity.i;
            float vy = ((object *)obj)->angular_velocity.j;
            float vz = ((object *)obj)->angular_velocity.k;

            if (vx * vx + vy * vy + vz * vz > 0.0025f) {
                turn_around = (uint8_t)(delta > 2.0f && best_weight > 2.0f);
            } else {
                turn_around = (uint8_t)(scale > 0.5f);
            }
        }
        if (turn_around) {
            real_vector3d *c = (real_vector3d *)actor_avoidance_circle[best];
            real_vector3d v;
            real_vector3d axis;
            float length;
            float t;

            *hold = held != -1 ? held + 1 : 0;
            v.i = context.forward.i * c->i + zero->i + context.left.i * c->j + context.up.i * c->k;
            v.j = context.forward.j * c->i + zero->j + context.left.j * c->j + context.up.j * c->k;
            v.k = context.forward.k * c->i + zero->k + context.left.k * c->j + context.up.k * c->k;
            axis.i = v.k * desired->j - v.j * desired->k;
            axis.j = v.i * desired->k - v.k * desired->i;
            axis.k = v.j * desired->i - v.i * desired->j;
            length = (float)halo::libm::sqrt(axis.k * axis.k + axis.j * axis.j + axis.i * axis.i);
            if (halo::libm::fabs(length) >= 9.999999747378752e-05) {
                float inverse = 1.0f / length;

                axis.i *= inverse;
                axis.j *= inverse;
                axis.k *= inverse;
                if (length > 0.0f) {
                    float angle = halo::math::vector3d_angle_between_4cd4f0(v, *desired);

                    result.i = axis.i * angle;
                    result.j = axis.j * angle;
                    result.k = axis.k * angle;
                }
            }
            t = (2.0f - along) * 0.5f - 0.5f;
            if (t < 0.0f) {
                t = 0.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
            }
            out = t > scale ? t : scale;
            *best_saved = best;
            goto done;
        }
    }

    *hold = -1;
    if (!(forwardness < 0.5f)) {
        real_vector3d *c;
        float length;

        if (!(closeness > 0.0f)) {
            goto reset;
        }
        c = (real_vector3d *)actor_avoidance_circle[best];
        result = *zero;
        result.i = context.left.i * -c->k + result.i;
        result.j = context.left.j * -c->k + result.j;
        result.k = context.left.k * -c->k + result.k;
        result.i = c->j * context.up.i + result.i;
        result.j = context.up.j * c->j + result.j;
        result.k = context.up.k * c->j + result.k;
        length = (float)halo::libm::sqrt(result.k * result.k + result.j * result.j + result.i * result.i);
        if (halo::libm::fabs(length) >= 9.999999747378752e-05) {
            float inverse = 1.0f / length;

            result.i *= inverse;
            result.j *= inverse;
            result.k *= inverse;
            if (length > 0.0f) {
                float k2 = scale * 1.0471976f;

                result.i *= k2;
                result.j *= k2;
                result.k *= k2;
            }
        }
        out = scale;
        *best_saved = best;
        goto done;
    }
    if (delta > 1.3f) {
        real_vector3d *c = (real_vector3d *)actor_avoidance_circle[best];

        if (e.k * c->k + e.j * c->j + e.i * c->i > 0.5f) {
            float t = delta * 0.7692308f - 0.5f;
            float w;

            if (t < 0.0f) {
                t = 0.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
            }
            out = t > scale ? t : scale;
            w = 1.0471976f * out;
            if (e.k * c->j - e.j * c->k > 0.0f) {
                w = -w;
            }
            *best_saved = best;
            result.i = context.forward.i * w;
            result.j = context.forward.j * w;
            result.k = context.forward.k * w;
            goto done;
        }
    }
reset:
    out = 0.0f;
    *best_saved = -1;
done:
    *out_direction = result;
    *out_scale = out;
}

namespace actor_movement_update_local {
static auto &global_origin3d_pointer = halo::link::ref<const real_vector3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &global_forward2d_pointer = halo::link::ref<const real_vector2d *>(halo::ai::vars().global_forward2d_pointer);
}

/**
 * Actor AI behaviour: movement update.
 *
 * @address 0x416790
 */
void ActorView::movement_update()
{
    using namespace actor_movement_update_local;
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & 0xffffu];
    uint8_t *actor_base = (uint8_t *)a;
    Actor *actor_def = halo::ai::tag_data<Actor>(a->actor_definition_tag);

    uint8_t sidestep_mode = 0;
    uint8_t face_along_heading = 0;
    uint8_t vehicle_stuck = 0;
    uint8_t clear_recognition = 0;
    float avoid_threshold = 0.0f;
    uint8_t want_avoid_check = 0;
    int16_t cached_axis;
    float throttle_maximum = 1.0f;
    float avoidance_scale = 0.0f;
    float oversteer_max = 0.0f;
    float oversteer_min = 0.0f;
    float steering_maximum = 0.0f;
    uint8_t order_failed = 0;

    uint8_t movement_mode;
    int16_t movement_style;
    int16_t context;

    a->desired_facing_vector = *(const real_point3d *)&a->facing;

    a->turn_required = 0;
    actor_base[0x58d] = 1;
    actor_base[0x58e] = 1;

    if (a->move_in_direction != 0) {
        a->desired_movement_vector = *(const real_point3d *)&a->move_direction;
        a->moving = 1;
        actor_base[0x58d] = 0;
        a->avoidance_direction = *global_origin3d_pointer;
        a->avoidance_scale = 0.0f;
        a->avoidance_emergency = 0.0f;
    } else if (a->vehicle_driving_type == 4) {
        const real_vector3d *desired;
        real_vector3d probe;
        real_vector3d sampled;
        float sampled_scale = 0.0f;
        float blend;
        float keep;

        if (a->moving == 0) {
            probe.i = a->facing.i * 3.0f;
            probe.j = a->facing.j * 3.0f;
            probe.k = a->facing.k * 3.0f;
            desired = &probe;
        } else {
            desired = (const real_vector3d *)&a->desired_movement_vector;
        }
        halo::ai::actor_movement_choose_avoidance_direction(actor_index, const_cast<real_vector3d *>(desired), &sampled, &sampled_scale);

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
        a->avoidance_emergency = sampled_scale;
        a->avoidance_scale = blend * sampled_scale + keep * a->avoidance_scale;
        if (a->avoidance_scale < 0.001f) {
            a->avoidance_scale = 0.0f;
        }
        if (a->moving != 0) {
            real_vector3d turn = a->avoidance_direction;
            float length_squared = turn.j * turn.j + turn.k * turn.k + turn.i * turn.i;
            if (0.0001f < length_squared) {
                double length = halo::libm::sqrt((double)length_squared);
                float inverse = (float)(1.0 / length);
                turn.i *= inverse;
                turn.j *= inverse;
                turn.k *= inverse;
                halo::math::vector3d_rotate_about_axis(*((real_vector3d *)&a->desired_movement_vector), turn, (real)halo::libm::sin(length), (real)halo::libm::cos(length));
            }
            avoidance_scale = a->avoidance_scale;
        }
    }

    movement_style = a->movement_style_override;
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
    a->control_animation_mode = movement_style;
    cached_axis = a->strafe_axis_override;

    if (a->movement_action_complete != 0 &&
        actor_def->stationary_movement_dist <= a->movement_timer) {
        movement_mode = actor_base[0x427];
    } else {
        movement_mode = actor_base[0x426];
    }

    context = a->vehicle_driving_type;
    if (context < 1) {
        if (a->order_committed != 0) {
            a->moving = 0;
            a->moving_facing_direction = 0;
            actor_base[0x58d] = (uint8_t)((a->type == 0xf || a->vehicle_gunner != 0) ? 1 : 0);
            actor_base[0x58e] = 0;
            movement_mode = 0;
        } else if (a->secondary_action != -1) {
            a->moving = 0;
            actor_base[0x58d] = 0;
            actor_base[0x58e] = 0;
            movement_mode = 0;
        } else if (a->control_animation_mode == 1) {
            a->moving = 0;
            actor_base[0x58d] = 0;
            actor_base[0x58e] = 0;
            face_along_heading = 1;
            movement_mode = 0;
        } else if (a->airborne != 0 && a->flying == 0) {
            a->moving = 0;
            actor_base[0x58d] = 1;
            movement_mode = 0;
        } else if (a->grenade_throw_pending != 0) {
            real_vector3d away;
            away.i = a->grenade_impact_point.x - a->body_position.x;
            away.j = a->grenade_impact_point.y - a->body_position.y;
            away.k = a->grenade_impact_point.z - a->body_position.z;
            a->moving = 0;
            movement_mode = 0;
            if (halo::math::vector3d_normalize_with_length(away) == 0.0f) {
                actor_base[0x58d] = 1;
            } else {
                a->desired_facing_vector.x = away.i;
                a->desired_facing_vector.y = away.j;
                a->desired_facing_vector.z = away.k;
                actor_base[0x58d] = 0;
                actor_base[0x58e] = 0;
                a->turn_required = 1;
            }
        } else if (a->incoming_fire_ticks >= 1) {
            a->moving = 0;
            actor_base[0x58d] = 1;
            movement_mode = (uint8_t)((actor_def->flags >> 0x1e) & 1);
        } else {
            clear_recognition = 1;
            if (movement_style == 2 &&
                ((movement_mode == 0 && !halo::has(static_cast<halo::tags::actor_tag_flag>(actor_def->flags), halo::tags::actor_tag_flag::standing_must_move_forward)) ||
                 (movement_mode != 0 && (int8_t)(actor_def->flags >> 8) >= 0))) {
            } else {
                a->forced_aim = 0;
            }
            if (movement_style == 4) {
                face_along_heading = 1;
            }
            if (halo::has(static_cast<halo::tags::actor_tag_flag>(actor_def->flags), halo::tags::actor_tag_flag::flying)) {
                avoid_threshold = actor_def->free_flying_sidestep * actor_def->free_flying_sidestep;
                sidestep_mode = 1;
                want_avoid_check = 1;
                if (a->forced_aim != 0) {
                    avoid_threshold *= 4.0f;
                }
            }
        }
    } else {
        object *unit_object = halo::ai::object_at(a->active_unit_index);
        Vehicle *vehicle_def = halo::ai::tag_data<Vehicle>(unit_object->definition_tag);
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
                a->moving = 0;
                actor_base[0x58d] = 1;
                movement_mode = 0;
            } else if (0.7f <= unit_vehicle->ground_lean) {
                take_sideslip = 1;
            } else {
                vehicle_stuck = 1;
                if (0.8f <= unit_object->up.k) {
                    take_sideslip = 1;
                } else {
                    real_vector3d righting;
                    righting.i = unit_object->up.i;
                    righting.j = unit_object->up.j;
                    righting.k = 0.0f;
                    movement_mode = 0;
                    if (halo::math::vector3d_normalize_with_length(righting) <= 0.0f) {
                        a->moving = 0;
                    } else {
                        a->moving = 1;
                        a->desired_movement_vector.x = righting.i * 3.0f;
                        a->desired_movement_vector.y = righting.j * 3.0f;
                        a->desired_movement_vector.z = righting.k * 3.0f;
                    }
                }
            }
        } else if (movement_style == 3) {
            take_sideslip = 1;
        } else if (movement_style == 4) {
            real_vector3d direction;
            if (halo::units::unit_get_average_active_marker_direction(a->active_unit_index, &direction) == 0) {
                cached_axis = 0;
                sidestep_mode = 1;
                order_failed = 1;
                movement_mode = 0;
            } else {
                a->moving = 0;
                actor_base[0x58d] = 0;
                actor_base[0x58e] = 0;
                a->desired_facing_vector.x = -direction.i;
                movement_mode = 0;
                a->desired_facing_vector.y = -direction.j;
                a->desired_facing_vector.z = -direction.k;
            }
        } else {
            a->moving = 0;
            a->moving_facing_direction = 0;
            actor_base[0x58d] = (uint8_t)((a->type == 0xf || a->vehicle_gunner != 0) ? 1 : 0);
            movement_mode = 0;
        }

        if (take_sideslip) {
            want_avoid_check = 1;
            movement_mode = 0;
            avoid_threshold = vehicle_def->ai_sideslip_distance * vehicle_def->ai_sideslip_distance;
        }
    }

    if (a->moving != 0 && a->waypoint_reached == 0) {
        halo::ai::actor_movement_apply_steering(
            cached_axis, sidestep_mode,
            actor_index, want_avoid_check, avoid_threshold, order_failed,
            steering_maximum, oversteer_min, oversteer_max, avoidance_scale, throttle_maximum,
            (real_vector3d *)&a->desired_movement_vector, (real_vector3d *)&a->desired_facing_vector,
            &a->moving_facing_direction, &a->throttle, &a->movement_thwarted, &a->waypoint_reached);
        if (a->waypoint_reached != 0) {
            a->moving = 0;
        }
    }

    if (a->moving != 0) {
        actor_base[0x58e] = 0;
        actor_base[0x58d] = 0;
    } else if (face_along_heading) {
        a->desired_facing_vector = *(const real_point3d *)&a->facing;
        actor_base[0x58e] = 0;
        a->moving_facing_direction = 0;
        actor_base[0x58d] = 0;
    } else if (actor_base[0x590] != 0) {
        a->desired_facing_vector.x = a->oversteer_angle[1];
        a->desired_facing_vector.y = a->oversteer_angle[2];
        a->desired_facing_vector.z = a->oversteer_angle[3];
        actor_base[0x58e] = 1;
        a->moving_facing_direction = 0;
        actor_base[0x58d] = 0;
    }

    if (clear_recognition && a->moving == 0) {
        halo::ai::actor_clear_recognition_history(actor_index, 1);
    }

    if (a->moving != 0 && halo::has(static_cast<halo::tags::actor_tag_flag>(actor_def->flags), halo::tags::actor_tag_flag::cannot_move_while_crouching)) {
        movement_mode = 0;
    }
    actor_base[0x58f] = 0;
    if (movement_mode != 0 && halo::has(static_cast<halo::tags::actor_tag_flag>(actor_def->flags), halo::tags::actor_tag_flag::fixed_crouch_facing)) {
        actor_base[0x58f] = 1;
    }
    a->crouching = movement_mode;
    if (movement_mode != 0) {
        a->control_flags |= halo::units::to_bits(halo::units::unit_control_flag::crouch);
    } else {
        a->control_flags &= ~halo::units::to_bits(halo::units::unit_control_flag::crouch);
    }

    if (a->secondary_action == -1 &&
        (a->unit_index == (datum_index)k_datum_index_none || halo::units::unit_is_in_busy_animation_state(a->unit_index) == 0) &&
        a->active_unit_index == (datum_index)k_datum_index_none &&
        a->airborne == 0 && a->berserking != 0 && a->berserk_announced == 0) {
        real_vector2d facing;
        datum_index target_object = (datum_index)k_datum_index_none;

        facing.i = a->facing.i;
        facing.j = a->facing.j;
        if (a->target_unit_index != (datum_index)k_datum_index_none) {
            prop *target_prop = &((prop *)halo::ai::globals().prop_data->data)[a->target_unit_index & halo::k_slot_mask];
            target_object = target_prop->object_index;
            facing.i = target_prop->direction.x;
            facing.j = target_prop->direction.y;
            if (halo::math::vector2d_normalize_with_length(facing) == 0.0f) {
                facing.i = a->facing.i;
                facing.j = a->facing.j;
            }
        }
        halo::ai::actor_queue_secondary_action(actor_index, 0, &facing);
        halo::ai::ai_communication_broadcast(0x2a, a->unit_index, target_object, 3,
                                   (datum_index)k_datum_index_none,
                                   (datum_index)k_datum_index_none, 0);
        a->berserk_announced = 1;
    }

    if (vehicle_stuck) {
        a->control_flags |= halo::units::to_bits(halo::units::unit_control_flag::jump);
    } else if (a->airborne != 0 || a->active_unit_index != (datum_index)k_datum_index_none) {
        a->jump_velocity_request.valid = 0;
    } else if (halo::ai::actor_action_has_queued_secondary(actor_index) == 0 && a->jump_requested != 0) {
        uint8_t handled = 0;
        if (a->jump_is_leap != 0) {
            real_vector2d facing;
            if (a->jump_parameters_valid != 0) {
                facing = a->jump_facing;
            } else {
                facing.i = a->facing.i;
                facing.j = a->facing.j;
                if (halo::math::vector2d_normalize_with_length(facing) == 0.0f) {
                    facing = *global_forward2d_pointer;
                }
            }
            if (halo::units::unit_try_ready_weapon_variant(a->unit_index, &facing) != 0) {
                halo::ai::ai_communication_broadcast(0x2f, a->unit_index,
                                           (datum_index)k_datum_index_none, -1,
                                           (datum_index)k_datum_index_none,
                                           (datum_index)k_datum_index_none, 0);
                handled = 1;
            }
        }
        if (!handled) {
            halo::ai::actor_set_flag_bit1(actor_index);
        }
        if (a->jump_parameters_valid != 0) {
            a->jump_velocity_request.direction.i = a->jump_facing.i;
            a->jump_velocity_request.valid = 1;
            a->jump_velocity_request.direction.j = a->jump_facing.j;
            a->jump_velocity_request.horizontal_speed = a->jump_horizontal_velocity;
            a->jump_velocity_request.vertical_speed = a->jump_vertical_velocity;
        }
    }

    *(uint32_t *)&((struct actor *)actor_base)->control_animation_impulse = *(uint32_t *)&((struct actor *)actor_base)->secondary_action;
    *(uint32_t *)(actor_base + 0x6f0) = *(uint32_t *)(actor_base + 0x41c);
    *(uint32_t *)(actor_base + 0x6f4) = *(uint32_t *)(actor_base + 0x420);
}

}
