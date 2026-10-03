#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

namespace halo::ai {

namespace actor_movement_choose_avoidance_direction_local {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern uint32_t global_structure_bsp;
extern uint32_t global_structure_collision_bsp;
extern const real_vector3d *global_origin3d_pointer;
extern double sqrt(double x);
extern double fabs(double x);
extern void actor_movement_collect_obstacle_candidates(actor_movement_context *context);
extern int16_t actor_movement_test_obstacle_ray(real_vector3d *out_elevation, const float *sample,
    real_point3d *out_end_point, actor_movement_context *context, float *out_distance,
    uint8_t *out_clear_counter);
extern uint8_t actor_avoidance_interpolate_sample(const real_vector3d *direction, const real_vector3d *samples,
    int16_t count, const float *values, float *out_index, float *out_value);
extern float actor_avoidance_samples_a[16][7];
extern float actor_avoidance_circle[8][3];
extern float actor_avoidance_samples_b[9][7];
extern const float actor_avoidance_near_weights[9][8];
extern const float actor_avoidance_ray_weights[2];
}
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
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    const real_vector3d *zero = global_origin3d_pointer;
    real_vector3d result = *zero;
    float out = 0.0f;
    datum_index unit_index = ((actor *)act)->active_unit_index;
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
    int16_t *best_saved = (int16_t *)(act + 0x5d8);
    int16_t *hold = (int16_t *)(act + 0x5f0);
    int16_t held;

    if (unit_index == k_datum_index_none) {
        unit_index = ((actor *)act)->unit_index;
        if (unit_index == k_datum_index_none) {
            *out_direction = result;
            *out_scale = 0.0f;
            return;
        }
    }
    obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
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
    actor_movement_collect_obstacle_candidates(&context);

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
        if (actor_movement_test_obstacle_ray(&elevation, actor_avoidance_samples_b[k], &end_point, &context, &distance,
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
            hit[j] = actor_movement_test_obstacle_ray(&elevation, actor_avoidance_samples_a[k * 2 + j], &end_point,
                                                      &context, &ray_distance[j], act + 0x5c8 + k * 2 + j);
        }
        for (j = 1; j >= 0; j--) {
            float weight = actor_avoidance_ray_weights[j];

            if (hit[j] == 0) {
                if (blocked) {
                    acc += 1.0f * weight;
                } else {
                    uint8_t clear_ticks = act[0x5c8 + k * 2 + j];
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
        float speed = (float)sqrt(vx * vx + vy * vy + vz * vz);

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
            length = (float)sqrt(motion.k * motion.k + motion.j * motion.j);
            if (fabs(length) >= 9.999999747378752e-05) {
                float inverse = 1.0f / length;
                float value;

                motion.i = 0.0f * inverse;
                motion.j *= inverse;
                motion.k *= inverse;
                if (length > 0.0f &&
                    actor_avoidance_interpolate_sample(&motion, (real_vector3d *)actor_avoidance_circle, 8, weights,
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
        float length = (float)sqrt(d.k * d.k + d.j * d.j + d.i * d.i);

        if (fabs(length) >= 9.999999747378752e-05) {
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
                length_2 = (float)sqrt(e.k * e.k + e.j * e.j);
                if (fabs(length_2) >= 9.999999747378752e-05) {
                    float inverse_2 = 1.0f / length_2;

                    e.i = 0.0f * inverse_2;
                    e.j *= inverse_2;
                    e.k *= inverse_2;
                    if (length_2 > 0.0f) {
                        actor_avoidance_interpolate_sample(&e, (real_vector3d *)actor_avoidance_circle, 8, weights,
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
            length = (float)sqrt(axis.k * axis.k + axis.j * axis.j + axis.i * axis.i);
            if (fabs(length) >= 9.999999747378752e-05) {
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
        length = (float)sqrt(result.k * result.k + result.j * result.j + result.i * result.i);
        if (fabs(length) >= 9.999999747378752e-05) {
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
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern data_array *prop_data;
extern const real_vector3d *global_origin3d_pointer;
extern const real_vector2d *global_forward2d_pointer;
extern double sin(double x);
extern double cos(double x);
extern double sqrt(double x);
extern void actor_movement_choose_avoidance_direction(datum_index actor_index,
                                                      const real_vector3d *desired_direction,
                                                      real_vector3d *out_direction,
                                                      float *out_scale);
extern void actor_movement_apply_steering(
    int16_t cached_axis, uint8_t keep_z,
    datum_index actor_index, uint8_t want_avoid_check, float avoid_threshold, uint8_t order_failed,
    float steering_maximum, float oversteer_min, float oversteer_max, float avoidance_scale,
    float throttle_maximum,
    real_vector3d *desired_direction, real_vector3d *out_direction, int16_t *out_axis,
    real_vector3d *out_heading, uint8_t *out_flag_507, uint8_t *out_flag_506);
extern void actor_clear_recognition_history(datum_index actor_index, uint8_t keep_when_typed);
extern uint8_t actor_queue_secondary_action(datum_index actor_index, int16_t action,
                                            uint32_t payload[2]);
extern uint8_t actor_action_has_queued_secondary(datum_index actor_index);
extern void actor_set_flag_bit1(datum_index actor_index);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index,
                                       datum_index object_a, int32_t param_d,
                                       datum_index object_b, datum_index object_c,
                                       uint32_t *param_g);
}
}

/**
 * Actor AI behaviour: movement update.
 *
 * @address 0x416790
 */
void ActorView::movement_update()
{
    using namespace actor_movement_update_local;
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffffu];
    uint8_t *actor_base = (uint8_t *)a;
    Actor *actor_def = (Actor *)halo::cache::globals().tag_instances[a->actor_definition_tag & 0xffff].data;

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
        actor_movement_choose_avoidance_direction(actor_index, desired, &sampled, &sampled_scale);

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
                double length = sqrt((double)length_squared);
                float inverse = (float)(1.0 / length);
                turn.i *= inverse;
                turn.j *= inverse;
                turn.k *= inverse;
                halo::math::vector3d_rotate_about_axis(*((real_vector3d *)&a->desired_movement_vector), turn, (real)sin(length), (real)cos(length));
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
        actor_def->stationary_movement_dist <= *(float *)&a->movement_timer) {
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
        } else if (a->unknown_360 >= 1) {
            a->moving = 0;
            actor_base[0x58d] = 1;
            movement_mode = (uint8_t)((actor_def->flags >> 0x1e) & 1);
        } else {
            clear_recognition = 1;
            if (movement_style == 2 &&
                ((movement_mode == 0 && (actor_def->flags & 0x4000) == 0) ||
                 (movement_mode != 0 && (int8_t)(actor_def->flags >> 8) >= 0))) {
            } else {
                a->forced_aim = 0;
            }
            if (movement_style == 4) {
                face_along_heading = 1;
            }
            if ((actor_def->flags & 0x200000) != 0) {
                avoid_threshold = actor_def->free_flying_sidestep * actor_def->free_flying_sidestep;
                sidestep_mode = 1;
                want_avoid_check = 1;
                if (a->forced_aim != 0) {
                    avoid_threshold *= 4.0f;
                }
            }
        }
    } else {
        object *unit_object = ((object_header *)object_data->data)[a->active_unit_index & 0xffff].data;
        Vehicle *vehicle_def = (Vehicle *)halo::cache::globals().tag_instances[unit_object->definition_tag & 0xffff].data;
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
        actor_movement_apply_steering(
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
        actor_clear_recognition_history(actor_index, 1);
    }

    if (a->moving != 0 && (actor_def->flags & 0x10000000) != 0) {
        movement_mode = 0;
    }
    actor_base[0x58f] = 0;
    if (movement_mode != 0 && (actor_def->flags & 0x20000000) != 0) {
        actor_base[0x58f] = 1;
    }
    a->crouching = movement_mode;
    if (movement_mode != 0) {
        a->control_flags |= 1u;
    } else {
        a->control_flags &= ~1u;
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
            prop *target_prop = &((prop *)prop_data->data)[a->target_unit_index & 0xffff];
            target_object = target_prop->object_index;
            facing.i = target_prop->direction.x;
            facing.j = target_prop->direction.y;
            if (halo::math::vector2d_normalize_with_length(facing) == 0.0f) {
                facing.i = a->facing.i;
                facing.j = a->facing.j;
            }
        }
        actor_queue_secondary_action(actor_index, 0, (uint32_t *)&facing);
        ai_communication_broadcast(0x2a, a->unit_index, target_object, 3,
                                   (datum_index)k_datum_index_none,
                                   (datum_index)k_datum_index_none, 0);
        a->berserk_announced = 1;
    }

    if (vehicle_stuck) {
        a->control_flags |= 2u;
    } else if (a->airborne != 0 || a->active_unit_index != (datum_index)k_datum_index_none) {
        a->jump_velocity_request[0] = 0;
    } else if (actor_action_has_queued_secondary(actor_index) == 0 && a->jump_requested != 0) {
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
        if (a->jump_parameters_valid != 0) {
            *(float *)&a->jump_velocity_request[4]  = a->jump_facing.i;
            a->jump_velocity_request[0] = 1;
            *(float *)&a->jump_velocity_request[8]  = a->jump_facing.j;
            *(float *)&a->jump_velocity_request[12] = a->jump_horizontal_velocity;
            *(float *)&a->jump_velocity_request[16] = a->jump_vertical_velocity;
        }
    }

    *(uint32_t *)&((struct actor *)actor_base)->control_animation_impulse = *(uint32_t *)&((struct actor *)actor_base)->secondary_action;
    *(uint32_t *)(actor_base + 0x6f0) = *(uint32_t *)(actor_base + 0x41c);
    *(uint32_t *)(actor_base + 0x6f4) = *(uint32_t *)(actor_base + 0x420);
}

}
