#include "halo/units/records.hpp"
#include "halo/units/unit.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/tags/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/libm.hpp"


namespace halo::units {

/**
 * Turns a biped's body toward its desired facing once per tick. A walking biped (or any biped whose health is
 * frozen) takes the first path: the desired facing is projected into the turning plane -- the XY plane
 * normally, or the plane perpendicular to object.up for a can_climb_any_surface biped -- and the sign of its
 * cross product with the current forward picks the turn direction, with a tie broken by the current turn
 * animation when the two are nearly opposite.
 *
 * Original register convention: see file header.
 *
 * @address 0x55b7c0
 */
void BipedView::update_facing(int8_t *out_animation_state)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    biped_data *biped = halo::units::biped_data_of(obj);
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    real_vector3d target;
    real_vector3d scratch;
    float turn_cross;
    float facing_dot;
    float threshold;
    uint8_t turn_right;

    if (!test_flag(tag->biped_flags, tags::biped_tag_flag::flying) ||
        (obj->vitality_flags & _object_health_frozen_bit) != 0) {
        int8_t base_state = unit->base_animation_state;
        uint32_t climbs;

        if (base_state == _unit_base_animation_state_asleep) {
            return;
        }

        climbs = tag->biped_flags & 0x00000040;
        if (climbs == 0) {
            target.i = unit->desired_facing_vector.i;
            target.j = unit->desired_facing_vector.j;
            target.k = 0.0f;
            if (halo::math::vector2d_normalize_with_length(*((real_vector2d *)&target)) == 0.0f) {
                target = obj->forward;
            }
            turn_cross = target.i * obj->forward.j - target.j * obj->forward.i;
            facing_dot = target.j * obj->forward.j;
        } else {
            halo::math::vector3d_cross_product(scratch, unit->desired_facing_vector, obj->up);
            halo::math::vector3d_cross_product(target, obj->up, scratch);
            if (halo::math::vector3d_normalize_with_length(target) == 0.0f) {
                target = obj->forward;
            }
            halo::math::vector3d_cross_product(scratch, obj->forward, target);
            turn_cross = scratch.i * obj->up.i + scratch.k * obj->up.k + scratch.j * obj->up.j;
            facing_dot = target.k * obj->forward.k + target.j * obj->forward.j;
        }
        facing_dot = target.i * obj->forward.i + facing_dot;

        turn_right = (0.0f < turn_cross);
        if (facing_dot < -0.9f) {
            if (unit->animation_state == _unit_animation_state_unknown_03) {
                turn_right = 1;
            } else if (unit->animation_state == _unit_animation_state_unknown_02) {
                turn_right = 0;
            }
        }

        if (biped->movement_state == 1 ||
            test_flag(tag->biped_flags, tags::biped_tag_flag::turns_without_animating)) {
            float turn_sin;
            float turn_cos;
            double angle;

            if ((unit->control_flags & _unit_control_flag_look_dont_turn) != 0) {
                return;
            }

            angle = (double)tag->moving_turning_speed * 0.033333335;
            turn_cos = (float)halo::libm::cos(angle);
            turn_sin = (float)halo::libm::sin(angle);
            if (turn_right) {
                turn_sin = -turn_sin;
            }

            if (climbs == 0) {
                float forward_i = obj->forward.i;
                obj->forward.i = turn_cos * forward_i - turn_sin * obj->forward.j;
                obj->forward.j = turn_cos * obj->forward.j + turn_sin * forward_i;
                turn_cross = target.i * obj->forward.j - target.j * obj->forward.i;
            } else {
                halo::math::vector3d_rotate_about_axis(obj->forward, obj->up, turn_sin, turn_cos);
                halo::math::vector3d_cross_product(scratch, obj->forward, target);
                turn_cross = scratch.i * obj->up.i + scratch.k * obj->up.k + scratch.j * obj->up.j;
            }

            if (turn_right) {
                if (0.0f <= turn_cross) {
                    return;
                }
            } else if (turn_cross <= 0.0f) {
                return;
            }

            if (!test_flag(tag->biped_flags, tags::biped_tag_flag::can_climb_any_surface)) {
                obj->forward.i = target.i;
                obj->forward.j = target.j;
                obj->forward.k = 0.0f;
                obj->up = *halo::math::globals().global_up3d_pointer;
            } else {
                halo::math::vector3d_cross_product(scratch, target, obj->up);
                if (0.0f < halo::math::vector3d_normalize_with_length(scratch)) {
                    halo::math::vector3d_cross_product(obj->forward, obj->up, scratch);
                    halo::math::vector3d_normalize_with_length(obj->forward);
                    return;
                }
            }
            halo::math::vector3d_normalize_with_length(obj->forward);
            return;
        }

        if (biped->movement_state == 0 &&
            base_state != _unit_base_animation_state_flaming &&
            (unit->flags & _unit_flag_aim_without_turning) == 0 &&
            (unit->control_flags & _unit_control_flag_look_dont_turn) == 0) {
            threshold = ((unit->control_flags & _unit_control_flag_exact_facing) != 0)
                            ? 0.99f
                            : tag->cosine_stationary_turning_threshold;
            if (facing_dot < threshold &&
                !test_flag(((Unit *)tag)->unit_flags, tags::unit_tag_flag::special_cinematic_unit)) {
                *out_animation_state = (int8_t)(turn_right + 2);
            }
        }
        return;
    }

    {
        float pitch;
        float bank_target;
        float bank_blend;
        float bank_time;
        float bounds[4];
        float servo_acceleration;
        bool facing_reached = false;

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
                target = obj->forward;
                facing_reached = true;
            }
        }

        if (!facing_reached) {
            pitch = tag->pitch_ratio * unit->throttle.k;
            target = unit->desired_facing_vector;
            if (pitch != 0.0f) {
                target.k = target.k + pitch;
                if (halo::math::vector3d_normalize_with_length(target) == 0.0f) {
                    target = unit->desired_facing_vector;
                }
            }
        }

        halo::math::vector3d_cross_product(scratch, obj->up, obj->forward);
        bank_target = (scratch.i * unit->desired_facing_vector.i +
                       scratch.k * unit->desired_facing_vector.k +
                       scratch.j * unit->desired_facing_vector.j) *
                          3.3333333f * unit->throttle.i -
                      unit->throttle.j;
        if (1.5f < bank_target) {
            bank_target = 1.5f;
        }
        bank_target = bank_target * tag->bank_angle;

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

        bounds[0] = -3.1415927f;
        bounds[1] = 3.1415927f;
        bounds[2] = -1.5707964f;
        bounds[3] = 1.5707964f;

        servo_acceleration = tag->angular_acceleration_maximum * 0.0011111111f;
        if (servo_acceleration != 0.0f) {
            halo::math::vector3d_rotate_toward_bounded(&obj->forward, &obj->angular_velocity, bounds,
                         tag->angular_velocity_maximum * 0.033333335f, servo_acceleration, target, 0);
        } else {
            obj->forward = target;
        }
        ::halo::units::unit_update_up_vector((Biped *)tag, (::object *)obj);
    }
}

}
