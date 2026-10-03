#include <string.h>
#include "halo/models/api.hpp"
#include "halo/units/unit.hpp"
#include "halo/core/collision_flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "projectiles.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern data_array *object_data;
extern double fabs(double x);
extern float fabsf(float x);
extern uint8_t *global_identity_quaternion_pointer;
extern void object_physics_tick(uint32_t object_index, void *powered_states, void *contact_points, real_vector3d *extra_force, real_vector3d *extra_torque);
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result);
extern void effect_new_with_color(uint32_t effect, uint32_t creator, void *velocity, int32_t count, char **names, real_point3d *points, real_vector3d *vectors, float a_scale, float b_scale, int32_t color, int32_t tint, int32_t force);
}

namespace halo::units {

namespace vehicle_blend_animations_local {

static double clamp_unit(double value)
{
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 1.0) {
        return 1.0;
    }
    return value;
}

static void blend_fraction(ModelAnimationsAnimation *animation, double fraction, real_orientation *orientations)
{
    int32_t last_frame = *(int16_t *)&((struct ModelAnimationsAnimation *)animation)->frame_count - 1;

    halo::models::animation_overlay_interpolated_frame_orientations(animation, (float)((double)last_frame * fraction), orientations);
}

}

/**
 * Engine function vehicle_blend_animations.
 *
 * @address 0x5718e0
 */
void VehicleView::blend_animations(real_orientation *orientations)
{
    using namespace vehicle_blend_animations_local;
    datum_index object_index = datum_handle;
    uint8_t *obj = *(uint8_t **)((uint8_t *)object_data->data + halo::datum_slot(object_index) * 0xc + 8);
    uint8_t *vehicle_tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data;
    datum_index graph_tag = *(datum_index *)&((struct Object *)vehicle_tag)->animation_graph.tag_id;
    uint8_t *graph;
    uint8_t *entry;
    uint8_t *animations;
    int32_t count;
    int16_t *indices;
    int16_t i;

    if (graph_tag == k_datum_index_none) {
        return;
    }
    graph = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(graph_tag)].data;
    if (*(int32_t *)&((ModelAnimations *)graph)->vehicles.count == 0) {
        return;
    }
    entry = *(uint8_t **)&((ModelAnimations *)graph)->vehicles.pointer;
    if (entry == 0) {
        return;
    }
    animations = *(uint8_t **)&((ModelAnimations *)graph)->animations.pointer;
    count = (int32_t)((struct ModelAnimationsAnimationGraphVehicleAnimations *)entry)->animations.count;
    indices = (int16_t *)((struct ModelAnimationsAnimationGraphVehicleAnimations *)entry)->animations.pointer;

    if (count > 0 && indices[0] != -1) {
        halo::models::animation_aiming_screen_blend((ModelAnimationsAnimation *)(animations + indices[0] * 0xb4),
            (animation_aiming_screen *)entry, ((struct vehicle_object *)obj)->vehicle.turning_velocity, 0.0f, orientations);
    }
    if (count > 1 && indices[1] != -1) {
        double speed = halo::math::vector3d_scalar_triple_product(*((real_vector3d *)&((struct object *)obj)->up), *((real_vector3d *)&((struct object *)obj)->forward),
            *((real_vector3d *)&((struct object *)obj)->velocity));

        speed = (speed / ((struct Vehicle *)vehicle_tag)->maximum_forward_speed + 1.0) * 0.5;
        blend_fraction((ModelAnimationsAnimation *)(animations + indices[1] * 0xb4), clamp_unit(speed), orientations);
    }
    if (count > 2 && indices[2] != -1) {
        float steering = ((struct vehicle_object *)obj)->vehicle.forward_velocity;
        double fraction;

        if (steering < 0.0f) {
            fraction = 0.5 - steering / ((struct Vehicle *)vehicle_tag)->maximum_reverse_speed * 0.5;
        } else {
            fraction = (steering / ((struct Vehicle *)vehicle_tag)->maximum_forward_speed + 1.0) * 0.5;
        }
        blend_fraction((ModelAnimationsAnimation *)(animations + indices[2] * 0xb4), fraction, orientations);
    }
    if (count > 3 && indices[3] != -1) {
        double forward_speed = (double)((unit_object *)obj)->base.velocity.k * ((unit_object *)obj)->base.forward.k +
            (double)((unit_object *)obj)->base.velocity.j * ((unit_object *)obj)->base.forward.j +
            (double)((unit_object *)obj)->base.velocity.i * ((unit_object *)obj)->base.forward.i;

        forward_speed = clamp_unit(clamp_unit(forward_speed) / fabs(((struct Vehicle *)vehicle_tag)->maximum_forward_speed));
        blend_fraction((ModelAnimationsAnimation *)(animations + indices[3] * 0xb4), forward_speed, orientations);
    }
    if (count > 5 && indices[5] != -1) {
        ModelAnimationsAnimation *animation = (ModelAnimationsAnimation *)(animations + indices[5] * 0xb4);
        double fraction = 0.0;
        int32_t frames = *(int16_t *)&((struct ModelAnimationsAnimation *)animation)->frame_count;

        if (((struct Vehicle *)vehicle_tag)->wheel_circumference > 0.0f) {
            fraction = ((struct vehicle_object *)obj)->vehicle.wheel_rotation / ((struct Vehicle *)vehicle_tag)->wheel_circumference;
        }
        halo::models::animation_overlay_interpolated_frame_orientations(animation, (float)((double)frames * fraction), orientations);
    }
    for (i = 0; i < (int32_t)((struct ModelAnimationsAnimationGraphVehicleAnimations *)entry)->suspension_animations.count; i++) {
        int16_t suspension = *(int16_t *)((uint8_t *)((struct ModelAnimationsAnimationGraphVehicleAnimations *)entry)->suspension_animations.pointer + i * 0x14 + 2);

        if (suspension != -1) {
            uint8_t compression = obj[0x4f4 + i];
            double fraction = compression == 0xff ? 1.0 : (double)compression * (double)0.0039215689f;

            blend_fraction((ModelAnimationsAnimation *)(animations + suspension * 0xb4), fraction, orientations);
        }
    }
}

/**
 * Evaluates the four ObjectFunctionIn selectors on the Vehicle tag (vehicle_a_in..d_in) against a table of
 * physics-derived control values (speed, turn rate, vertical motion, etc., each normalized to 0..1) and
 * writes the results into the object's function-output array (object+0x124), used to drive the unit's
 * procedural animation blending. FIXED (register inputs, objdump; one stack argument remains, so no ordering
 * question): the original never reads EAX; unit_index arrive(s) on the stack (1 stack argument(s)).
 *
 * @address 0x5756f0
 */
void VehicleView::calculate_animation_controls()
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    Vehicle *tag = (Vehicle *)halo::cache::globals().tag_instances[halo::datum_slot(*(uint32_t *)obj)].data;
    real_vector3d *velocity = (real_vector3d *)&((struct object *)obj)->velocity;
    real_vector3d *forward = (real_vector3d *)&((struct object *)obj)->forward;
    real_vector3d *up = (real_vector3d *)&((struct object *)obj)->up;
    float forward_velocity = ((struct vehicle_object *)obj)->vehicle.forward_velocity;
    float sideways_velocity = ((struct vehicle_object *)obj)->vehicle.sideways_velocity;
    float turning_velocity = ((struct vehicle_object *)obj)->vehicle.turning_velocity;
    float max_forward = fabsf(tag->maximum_forward_speed);
    float max_reverse = fabsf(tag->maximum_reverse_speed);
    float max_speed = (max_forward > max_reverse) ? max_forward : max_reverse;
    float max_left_slide = fabsf(tag->maximum_left_slide);
    float max_right_slide = fabsf(tag->maximum_right_slide);
    float max_slide = (max_left_slide > max_right_slide) ? max_left_slide : max_right_slide;
    float max_left_turn = fabsf(tag->maximum_left_turn);
    float max_right_turn = fabsf(tag->maximum_right_turn);
    float max_turn = (max_left_turn > max_right_turn) ? max_left_turn : max_right_turn;
    int16_t *selectors = (int16_t *)&((struct Vehicle *)tag)->vehicle_a_in;
    float *outputs = (float *)&((struct object *)obj)->function_in_values;
    int i;

    for (i = 0; i < 4; i++) {
        float value;

        if (selectors[i] == 0) {
            continue;
        }
        switch (selectors[i]) {
        case 1: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
            value = fabsf(forward_velocity) / max_speed;
            break;
        case 2:
            value = !(forward_velocity < 0.0f) ? forward_velocity / max_forward : 0.0f / max_forward;
            break;
        case 3:
            value = (forward_velocity > 0.0f) ? 0.0f / max_reverse : fabsf(forward_velocity) / max_reverse;
            break;
        case 4:
            value = fabsf(sideways_velocity) / max_slide;
            break;
        case 5:
            value = fabsf(sideways_velocity) / max_left_slide;
            break;
        case 6:
            value = fabsf(sideways_velocity) / max_right_slide;
            break;
        case 7: {
            float a = fabsf(forward_velocity) / max_speed;
            float b = fabsf(sideways_velocity) / max_slide;

            value = (a > b) ? a : b;
            break;
        }
        case 8:
            value = fabsf(turning_velocity) / max_turn;
            break;
        case 9:
            value = fabsf(turning_velocity) / max_left_turn;
            break;
        case 10:
            value = fabsf(turning_velocity) / max_right_turn;
            break;
        case 0xb:
            outputs[i] = ((uint8_t)((struct vehicle_object *)obj)->vehicle.flags & 4) ? 1.0f : 0.0f;
            continue;
        case 0xc:
            outputs[i] = ((uint8_t)((struct vehicle_object *)obj)->vehicle.flags & 8) ? 1.0f : 0.0f;
            continue;
        case 0xe:
            value = halo::math::vector3d_length(*velocity) / max_speed;
            break;
        case 0xf:
            if (!test_flag(((struct object *)obj)->flags, objects::object_flag::unknown_4 | objects::object_flag::unknown_8 | objects::object_flag::in_water)) {
                outputs[i] = 0.0f;
                continue;
            }
            value = halo::math::vector3d_length(*velocity) / max_speed;
            break;
        case 0x10:
            if (!test_flag(((struct object *)obj)->flags, objects::object_flag::unknown_2)) {
                outputs[i] = 0.0f;
                continue;
            }
            value = halo::math::vector3d_length(*velocity) / max_speed;
            break;
        case 0x11:
            value = fabsf(velocity->k * forward->k + velocity->j * forward->j + velocity->i * forward->i) / max_speed;
            break;
        case 0x12: case 0x13:
            value = fabsf(up->k * velocity->k + up->j * velocity->j + up->i * velocity->i) / max_speed;
            break;
        case 0x14:
            value = ((struct vehicle_object *)obj)->vehicle.left_wheel_rotation / tag->wheel_circumference;
            break;
        case 0x15:
            value = ((struct vehicle_object *)obj)->vehicle.right_wheel_rotation / tag->wheel_circumference;
            break;
        case 0x16:
            value = fabsf(forward_velocity - turning_velocity) / max_speed;
            break;
        case 0x17:
            value = fabsf(turning_velocity + forward_velocity) / max_speed;
            break;
        case 0x18: case 0x19: case 0x1a: case 0x1b:
            value = ((struct vehicle_object *)obj)->vehicle.wheel_rotation / tag->wheel_circumference;
            break;
        case 0x20: {
            real_vector3d parallel;
            real_vector3d perpendicular;
            float slide;

            halo::math::vector3d_project_onto_unit_axis(&parallel, *forward, *velocity, &perpendicular);
            slide = halo::math::vector3d_length(perpendicular) * 3.3333333f;
            value = slide * slide;
            break;
        }
        case 0x21:
            value = ((struct vehicle_object *)obj)->vehicle.ground_lean;
            break;
        case 0x22:
            value = ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction;
            break;
        case 0x23: {
            float lean = fabsf(forward->k * velocity->k + forward->j * velocity->j + forward->i * velocity->i) / max_speed;
            float speed = fabsf(forward_velocity) / max_forward;
            float blend = ((float)((struct vehicle_object *)obj)->vehicle.airborne_ticks * 0.2f + 1.0f) * 0.5f;

            if (blend < 0.0f) {
                blend = 0.0f;
            } else if (blend > 1.0f) {
                blend = 1.0f;
            }
            value = lean * (1.0f - blend) + blend * speed;
            break;
        }
        case 0x24:
            value = ((halo::math::vector3d_length(*velocity) / tag->maximum_forward_speed) * ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction - 0.05f) *
                1.1764706f;
            break;
        default:
            outputs[i] = 0.0f;
            continue;
        }

        if (value < 0.0f) {
            value = 0.0f;
        } else if (value > 1.0f) {
            value = 1.0f;
        }
        outputs[i] = value;
    }
}

/**
 * REWRITTEN from objdump. Physics tag +0x68 != 2: object_physics_tick(unit, 0, contacts, 0, 0). Otherwise:
 * force = mass * throttle * the gravity-biased clamped delta from the velocity toward forward * speed (clamp
 * lengths frac * tag +0x300 / +0x304, frac = speed / tag +0x2f8, or -speed / tag +0x2fc in reverse);
 *
 * @address 0x573f60
 */
void VehicleView::calculate_ground_contact_lean(void *out_record, void *out_transform)
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data;
    uint8_t *physics = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    uint8_t *powered = (uint8_t *)out_record;
    real speed = ((struct vehicle_object *)obj)->vehicle.forward_velocity;
    real mass = *(real *)(physics + 0x8);
    real throttle = ((struct vehicle_object *)obj)->unit.driver_seat_power;
    real_vector3d *velocity = (real_vector3d *)&((struct object *)obj)->velocity;
    real_vector3d *forward = (real_vector3d *)&((struct object *)obj)->forward;
    real_vector3d *object_up = (real_vector3d *)&((struct object *)obj)->up;
    real_vector3d *angular_velocity = (real_vector3d *)&((struct object *)obj)->angular_velocity;
    real_vector3d *world_up = halo::math::globals().global_up3d_pointer;
    real_point3d target_velocity;
    real_vector3d delta, force, torque, axis;
    real_vector3d basis[3];
    real_matrix3x3 current;
    real_matrix3x3 relative;
    real_quaternion rotation;
    real frac, angle, k, moment, spin_rate, lean, step;
    uint8_t *rider;

    if (*(int32_t *)(physics + 0x68) != 2) {
        halo::physics::object_physics_tick(unit_index, 0, (uint32_t)out_transform, 0, 0);
        return;
    }

    target_velocity.x = speed * forward->i;
    target_velocity.y = speed * forward->j;
    target_velocity.z = speed * forward->k;
    frac = speed > 0.0f ? speed / ((struct Vehicle *)tag)->maximum_forward_speed : -(speed / ((struct Vehicle *)tag)->maximum_reverse_speed);
    halo::math::vector3d_delta_toward_gravity_biased_clamp_length(*((real_point3d *)velocity), target_velocity, &delta,
        frac * ((struct Vehicle *)tag)->speed_acceleration, frac * ((struct Vehicle *)tag)->speed_deceleration);
    force.i = delta.i * mass * throttle;
    force.j = delta.j * mass * throttle;
    force.k = delta.k * mass * throttle;
    halo::math::matrix3x3_from_forward_up(*object_up, *forward, current);

    basis[0] = *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i;
    basis[2].i = -basis[0].k * basis[0].i + world_up->i;
    basis[2].j = -basis[0].k * basis[0].j + world_up->j;
    basis[2].k = -basis[0].k * basis[0].k + world_up->k;
    if (halo::math::vector3d_normalize_with_length(basis[2]) == 0.0f) {
        basis[2] = *halo::math::globals().global_forward3d_pointer;
    }
    rider = obj;
    if (((unit_object *)obj)->unit.driver_unit_index != k_datum_index_none) {
        rider = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(((unit_object *)obj)->unit.driver_unit_index)].data;
    }
    if (*(datum_index *)(rider + 0x1f4) == k_datum_index_none) {
        real pitch = ((struct Vehicle *)tag)->fixed_gun_pitch;
        halo::math::vector3d_rotate_pair_in_plane(basis[2], basis[0], (real)sin((double)pitch), (real)cos((double)pitch));
    }
    angle = (basis[0].i * velocity->j - basis[0].j * velocity->i) / ((struct Vehicle *)tag)->maximum_forward_speed * ((struct Vehicle *)tag)->maximum_left_turn;
    halo::math::vector3d_rotate_about_axis_perpendicular(basis[2], basis[0], (real)sin((double)angle), (real)cos((double)angle));
    basis[1].i = basis[2].j * basis[0].k - basis[2].k * basis[0].j;
    basis[1].j = basis[2].k * basis[0].i - basis[2].i * basis[0].k;
    basis[1].k = basis[0].j * basis[2].i - basis[2].j * basis[0].i;

    halo::math::matrix3x3_transpose(&current, &current);
    halo::math::matrix3x3_multiply(&relative, (real_matrix3x3 *)basis, &current);
    halo::math::quaternion_from_matrix3x3(&relative, &rotation);
    halo::math::quaternion_to_axis_angle(rotation, &axis, angle);

    k = -angle * ((struct Vehicle *)tag)->turn_rate * 0.31830987f;
    moment = (*(real *)(physics + 0x58) + *(real *)(physics + 0x54) + *(real *)(physics + 0x50)) * 0.33333334f;
    torque.i = (axis.i * k - angular_velocity->i) * moment * throttle;
    torque.j = (axis.j * k - angular_velocity->j) * moment * throttle;
    torque.k = (axis.k * k - angular_velocity->k) * moment * throttle;

    spin_rate = (real)sqrt((double)(angular_velocity->i * angular_velocity->i + angular_velocity->j * angular_velocity->j +
        angular_velocity->k * angular_velocity->k)) / ((struct Vehicle *)tag)->turn_rate;
    lean = ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction;
    if (spin_rate > lean) {
        step = (1.0f - lean) * (1.0f - lean) * 0.2f;
        if (!(step >= 0.01f)) {
            step = 0.01f;
        } else if (!(step <= 0.05f)) {
            step = 0.05f;
        }
        if (!(spin_rate - lean > step)) {
            step = spin_rate - lean;
        }
    } else {
        step = lean * lean * 0.05f;
        if (!(step > 0.005f)) {
            step = 0.005f;
        }
        step = -step;
        if (spin_rate - lean > step) {
            step = spin_rate - lean;
        }
    }
    ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction = step + lean;

    *(real *)(powered + 0x18) = throttle;
    memcpy(powered + 0x1c, global_identity_quaternion_pointer, 16);
    *(real *)(powered + 0x78) = throttle;
    memcpy(powered + 0x7c, global_identity_quaternion_pointer, 16);
    halo::physics::object_physics_tick(unit_index, (powered_mass_point_state *)out_record, (uint32_t)out_transform, &force, &torque);
}

/**
 * REWRITTEN from objdump. Physics tag +0x68 != 2: object_physics_tick(unit, 0, contacts, 0, 0). Otherwise the
 * same hover solve as vehicle_calculate_ground_lean_controls without the lean state: desired basis from the
 * facing (+0x224) with world up minus its facing component, rotated by the side-slip angle; the force is
 * forward * (speed - v.forward) * mass * 0.05 plus up * |v.forward / max| * 0.0035651792 * mass * 1.05;
 *
 * @address 0x574460
 */
void VehicleView::calculate_ground_contact_lean_alt(void *out_record, void *out_transform)
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data;
    uint8_t *physics = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    uint8_t *powered = (uint8_t *)out_record;
    real max_speed = ((struct Vehicle *)tag)->maximum_forward_speed;
    real speed = ((struct vehicle_object *)obj)->vehicle.forward_velocity;
    real throttle;
    real_vector3d *velocity = (real_vector3d *)&((struct object *)obj)->velocity;
    real_vector3d *forward = (real_vector3d *)&((struct object *)obj)->forward;
    real_vector3d *object_up = (real_vector3d *)&((struct object *)obj)->up;
    real_vector3d *angular_velocity = (real_vector3d *)&((struct object *)obj)->angular_velocity;
    real_vector3d facing, up, force, torque, axis;
    real_matrix4x3 current, desired, relative;
    real_quaternion rotation;
    real dot, x_force, y_force, angle, per_tick, torque_scale;

    if (*(int32_t *)(physics + 0x68) != 2) {
        halo::physics::object_physics_tick(unit_index, 0, (uint32_t)out_transform, 0, 0);
        return;
    }

    facing = *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i;
    up.i = -(facing.i * facing.k);
    up.j = -(facing.j * facing.k);
    up.k = 1.0f - facing.k * facing.k;
    if (halo::math::vector3d_normalize_with_length(up) == 0.0f) {
        up.i = 1.0f;
        up.j = 0.0f;
        up.k = 0.0f;
    }

    dot = velocity->i * forward->i + velocity->j * forward->j + velocity->k * forward->k;
    x_force = (speed - dot) * *(real *)(physics + 0x8) * 0.05f;
    y_force = (real)fabs((double)(dot / max_speed)) * 0.0035651792f * *(real *)(physics + 0x8) * 1.05f;
    force.i = object_up->i * y_force + forward->i * x_force;
    force.j = object_up->j * y_force + forward->j * x_force;
    force.k = forward->k * x_force + object_up->k * y_force;

    angle = (velocity->j * facing.i - facing.j * velocity->i) * 1.5707964f / (real)fabs((double)max_speed);
    halo::math::vector3d_rotate_about_axis_perpendicular(up, facing, (real)sin((double)angle), (real)cos((double)angle));
    halo::math::matrix4x3_from_forward_up(*object_up, *forward, current);
    halo::math::matrix4x3_from_forward_up(up, facing, desired);
    halo::math::matrix4x3_inverse(&desired, desired);
    halo::math::matrix4x3_multiply(&current, &desired, &relative);
    halo::math::quaternion_from_matrix4x3(&relative, rotation);
    halo::math::quaternion_to_axis_angle(rotation, &axis, angle);

    per_tick = angle * 0.13333334f;
    torque_scale = *(real *)(physics + 0x0) * *(real *)(physics + 0x0) * *(real *)(physics + 0x8) * 0.05f;
    torque.i = (axis.i * per_tick - angular_velocity->i) * torque_scale;
    torque.j = (axis.j * per_tick - angular_velocity->j) * torque_scale;
    torque.k = (axis.k * per_tick - angular_velocity->k) * torque_scale;

    throttle = ((struct vehicle_object *)obj)->unit.driver_seat_power;
    *(real *)(powered + 0x18) = throttle;
    *(real *)(powered + 0x28) = 1.0f;
    *(real *)(powered + 0x1c) = 0.0f;
    *(real *)(powered + 0x20) = 0.0f;
    *(real *)(powered + 0x24) = 0.0f;
    *(real *)(powered + 0x78) = throttle;
    *(real *)(powered + 0x88) = 1.0f;
    *(real *)(powered + 0x7c) = 0.0f;
    *(real *)(powered + 0x80) = 0.0f;
    *(real *)(powered + 0x84) = 0.0f;

    force.i = throttle * force.i;
    force.j = throttle * force.j;
    force.k = throttle * force.k;
    torque.i = throttle * torque.i;
    torque.j = throttle * torque.j;
    torque.k = throttle * torque.k;
    halo::physics::object_physics_tick(unit_index, (powered_mass_point_state *)out_record, (uint32_t)out_transform, &force, &torque);
}

/**
 * REWRITTEN from objdump. Raw object offsets: +0x68 velocity, +0x74 forward, +0x80 up, +0x8c angular
 * velocity, +0x224 the desired facing, +0x338 the throttle scale, +0x4cc vehicle flags, +0x4d4 forward speed,
 * +0x4ec lean, +0x4f0 lean output. Vehicle tag +0x2f8 is the maximum forward speed; physics tag +0x00 and
 * +0x08 are the radius and mass. Flag bit 1 (disabled) clears the contact buffer (physics +0x74 entries of
 * 0x130) and only spawns the thruster effects.
 *
 * @address 0x573100
 */
void VehicleView::calculate_ground_lean_controls(uint8_t *out_transform)
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data;
    uint8_t *physics = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    uint16_t flags = ((struct vehicle_object *)obj)->vehicle.flags;
    real max_speed = ((struct Vehicle *)tag)->maximum_forward_speed;
    real speed = ((struct vehicle_object *)obj)->vehicle.forward_velocity;
    real throttle = ((struct vehicle_object *)obj)->unit.driver_seat_power;
    real clamped, f2, k, delta, lean_scale, dot, x_force, y_force, angle, per_tick, torque_scale;
    real_vector3d facing, up, force, torque;
    real_vector3d *velocity = (real_vector3d *)&((struct object *)obj)->velocity;
    real_vector3d *forward = (real_vector3d *)&((struct object *)obj)->forward;
    real_vector3d *object_up = (real_vector3d *)&((struct object *)obj)->up;
    real_vector3d *angular_velocity = (real_vector3d *)&((struct object *)obj)->angular_velocity;
    real_matrix4x3 current, desired, relative;
    real_quaternion rotation;
    real_vector3d axis;
    int32_t i;

    if (flags & 2) {
        int32_t bytes = *(int32_t *)(physics + 0x74) * 0x130;
        for (i = 0; i < bytes; i++) {
            out_transform[i] = 0;
        }
        VehicleView(unit_index).create_hover_thruster_effects();
        return;
    }

    clamped = !(speed >= 0.0f) ? 0.0f : (speed <= max_speed ? speed : max_speed);
    f2 = (clamped / max_speed) * (clamped / max_speed);
    k = (flags & 4) ? 0.25f : ((flags & 8) ? 1.0f : 0.75f);
    delta = k * ((1.0f - f2) * throttle) - ((struct vehicle_object *)obj)->vehicle.ground_lean;
    if (!(delta >= -0.05f)) {
        delta = -0.05f;
    } else if (!(delta <= 0.05f)) {
        delta = 0.05f;
    }
    ((struct vehicle_object *)obj)->vehicle.ground_lean = delta + ((struct vehicle_object *)obj)->vehicle.ground_lean;
    lean_scale = f2 * throttle;
    ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction = lean_scale;

    facing = *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i;
    up.i = -(facing.i * facing.k);
    up.j = -(facing.j * facing.k);
    up.k = 1.0f - facing.k * facing.k;
    if (halo::math::vector3d_normalize_with_length(up) == 0.0f) {
        up.i = 1.0f;
        up.j = 0.0f;
        up.k = 0.0f;
    }

    dot = velocity->i * forward->i + velocity->j * forward->j + velocity->k * forward->k;
    x_force = (speed - dot) * lean_scale * *(real *)(physics + 0x8) * 0.05f;
    y_force = ((real)fabs((double)(dot / max_speed)) * 1.05f + ((struct vehicle_object *)obj)->vehicle.ground_lean * 1.3f) *
        *(real *)(physics + 0x8) * 0.0035651792f;
    force.i = object_up->i * y_force + forward->i * x_force;
    force.j = object_up->j * y_force + forward->j * x_force;
    force.k = forward->k * x_force + object_up->k * y_force;

    angle = (velocity->j * facing.i - facing.j * velocity->i) * 1.5707964f / (real)fabs((double)max_speed);
    halo::math::vector3d_rotate_about_axis_perpendicular(up, facing, (real)sin((double)angle), (real)cos((double)angle));
    halo::math::matrix4x3_from_forward_up(*object_up, *forward, current);
    halo::math::matrix4x3_from_forward_up(up, facing, desired);
    halo::math::matrix4x3_inverse(&desired, desired);
    halo::math::matrix4x3_multiply(&current, &desired, &relative);
    halo::math::quaternion_from_matrix4x3(&relative, rotation);
    halo::math::quaternion_to_axis_angle(rotation, &axis, angle);

    per_tick = angle * 0.033333335f;
    torque_scale = *(real *)(physics + 0x0) * *(real *)(physics + 0x0) * *(real *)(physics + 0x8) * 0.05f;
    torque.i = (axis.i * per_tick - angular_velocity->i) * torque_scale;
    torque.j = (axis.j * per_tick - angular_velocity->j) * torque_scale;
    torque.k = (axis.k * per_tick - angular_velocity->k) * torque_scale;

    force.i = throttle * force.i;
    force.j = throttle * force.j;
    force.k = throttle * force.k;
    torque.i = throttle * torque.i;
    torque.j = throttle * torque.j;
    torque.k = throttle * torque.k;
    halo::physics::object_physics_tick(unit_index, 0, (uint32_t)out_transform, &force, &torque);
    VehicleView(unit_index).create_hover_thruster_effects();
}

/**
 * Not callable: see the header. Kept only so the address stays listed in the symbol tables.
 *
 * @address 0x5739a0
 */
void halo::units::vehicle_calculate_hover_lift_toward_target(void)
{
}

/**
 * Not callable: see the header. Kept only so the address stays listed in the symbol tables.
 *
 * @address 0x5738b0
 */
void halo::units::vehicle_calculate_hover_turn_controls(void)
{
}

/**
 * REWRITTEN from objdump. Physics tag +0x68 != 3: object_physics_tick(unit, 0, contacts, 0, 0). Otherwise the
 * ESI powered-mass-point buffer (vehicle_update [esp+0x88]) gets the drive: +0x04 forward speed, +0x0c/+0x6c
 * 0.003, +0x24/+0x28 sin/cos of turn * 0.5 * (1 - min(|speed| * 2.5, 1)), +0x88/+0xe8 1.0, +0xcc 0.005, the
 * rest 0.
 *
 * @address 0x572df0
 */
void VehicleView::calculate_lean_controls(void *mass_points, float *powered_states)
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data;
    uint8_t *physics = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    real_vector3d *velocity = (real_vector3d *)&((struct object *)obj)->velocity;
    real_vector3d *forward = (real_vector3d *)&((struct object *)obj)->forward;
    real_vector3d *up = (real_vector3d *)&((struct object *)obj)->up;
    real_vector3d *angular_velocity = (real_vector3d *)&((struct object *)obj)->angular_velocity;
    real_vector3d *world_up = halo::math::globals().global_up3d_pointer;
    uint8_t *ps = (uint8_t *)powered_states;
    real_vector3d zero_force;
    real_vector3d torque;
    real speed_factor, half_turn, steer;

    if (*(int32_t *)(physics + 0x68) != 3) {
        halo::physics::object_physics_tick(unit_index, 0, (uint32_t)mass_points, 0, 0);
        return;
    }

    speed_factor = (real)fabs((double)((real)sqrt((double)(velocity->i * velocity->i + velocity->j * velocity->j +
        velocity->k * velocity->k)) * 2.5f));
    half_turn = ((struct vehicle_object *)obj)->vehicle.turning_velocity * 0.5f;
    if (!(speed_factor <= 1.0f)) {
        speed_factor = 1.0f;
    }
    steer = (1.0f - speed_factor) * half_turn;
    *(real *)(ps + 0x04) = ((struct vehicle_object *)obj)->vehicle.forward_velocity;
    *(uint32_t *)(ps + 0x0c) = 0x3b449ba6;
    *(real *)(ps + 0x1c) = 0.0f;
    *(real *)(ps + 0x20) = 0.0f;
    *(real *)(ps + 0x24) = (real)sin((double)steer);
    *(real *)(ps + 0x28) = (real)cos((double)steer);
    *(uint32_t *)(ps + 0x6c) = 0x3b449ba6;
    *(real *)(ps + 0x7c) = 0.0f;
    *(real *)(ps + 0x80) = 0.0f;
    *(real *)(ps + 0x84) = 0.0f;
    *(real *)(ps + 0x88) = 1.0f;
    *(uint32_t *)(ps + 0xcc) = 0x3ba3d70a;
    *(real *)(ps + 0xe8) = 1.0f;
    *(real *)(ps + 0xdc) = 0.0f;
    *(real *)(ps + 0xe0) = 0.0f;
    *(real *)(ps + 0xe4) = 0.0f;
    zero_force.i = 0.0f;
    zero_force.j = 0.0f;
    zero_force.k = 0.0f;

    torque.i = -forward->k * forward->i + world_up->i;
    torque.j = -forward->k * forward->j + world_up->j;
    torque.k = -forward->k * forward->k + world_up->k;
    if (halo::math::vector3d_normalize_with_length(torque) == 0.0f) {
        torque.i = 0.0f;
        torque.j = 0.0f;
        torque.k = 0.0f;
    } else {
        real_vector3d side;
        real_vector3d slip;
        real angle, spin, w;
        int32_t sign;

        halo::math::vector3d_cross_product(side, *forward, *up);
        halo::math::vector3d_cross_product(slip, *velocity, *forward);
        angle = (slip.i * world_up->i + slip.j * world_up->j + slip.k * world_up->k) * 6.2831855f;
        halo::math::vector3d_rotate_about_axis(torque, *forward, (real)sin((double)angle), (real)cos((double)angle));
        angle = halo::math::vector3d_angle_between_4cd4f0(*up, torque);
        if (side.k * torque.k + side.j * torque.j + side.i * torque.i > 0.0f) {
            angle = -angle;
        }
        spin = forward->k * angular_velocity->k + forward->j * angular_velocity->j + forward->i * angular_velocity->i;
        sign = (angle == 0.0f) ? 0 : (angle >= 0.0f ? 1 : -1);
        w = (real)sqrt(fabs((double)angle) * 0.027925269678235054) * (real)sign - spin;
        if (!(w >= -0.013962635f)) {
            w = -0.013962635f;
        } else if (!(w <= 0.013962635f)) {
            w = 0.013962635f;
        }
        w = w * *(real *)(physics + 0x50);
        torque.i = w * forward->i;
        torque.j = w * forward->j;
        torque.k = w * forward->k;
    }
    halo::physics::object_physics_tick(unit_index, (powered_mass_point_state *)powered_states, (uint32_t)mass_points, &zero_force, &torque);
}

/**
 * Selects between two ground-contact lean calculations for a mounted/turret-style vehicle unit based on the
 * sign of its Physics tag's first field, and always triggers the hover-thruster midpoint effect afterward.
 *
 * @address 0x573ee0
 */
void VehicleView::calculate_mounted_controls_dispatch(void *out_transform, void *out_record)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    Vehicle *tag = (Vehicle *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    uint8_t *physics_tag = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;

    if (*(float *)physics_tag > 0.0f) {
        VehicleView(unit_index).calculate_ground_contact_lean_alt(out_record, out_transform);
    } else {
        VehicleView(unit_index).calculate_ground_contact_lean(out_record, out_transform);
    }
    VehicleView(unit_index).create_hover_thruster_midpoint_effects();
}

/**
 * REWRITTEN from objdump. Up to 15 "hover thrusters" markers then 16 - n "jet thrusters" markers (marker
 * stride 0x6c: +0x3c forward, +0x60 position). For each one the forward is randomized by up to 0.2618 rad
 * (seed 0x719cd4) and cast (flags 0x61, excluding the unit) for (lean * 6 + 2) world units, where lean is
 * +0x4ec for hover markers and +0x4f0 for jets.
 *
 * @address 0x574900
 */
void VehicleView::create_hover_thruster_effects()
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data;
    uint8_t markers[16 * 0x6c];
    int16_t hover_count;
    int16_t total;
    int16_t i;
    static char *names[3] = { (char *)"incident", (char *)"normal", (char *)"reflected" };

    if (*(int32_t *)&((struct Vehicle *)tag)->effect.tag_id == -1) {
        return;
    }
    hover_count = (int16_t)halo::objects::object_get_node_local_transform(unit_index, (char *)"hover thrusters", (object_marker *)markers, 0xf);
    total = (int16_t)(hover_count + (int16_t)halo::objects::object_get_node_local_transform(unit_index, (char *)"jet thrusters",
        (object_marker *)(markers + hover_count * 0x6c), 0x10 - hover_count));

    for (i = 0; i < total; i++) {
        uint8_t *marker = markers + (int32_t)i * 0x6c;
        real_vector3d direction;
        real_vector3d delta;
        collision_result result;
        real length;

        halo::math::vector3d_randomize_direction(*(real_point3d *)(marker + 0x3c), &direction, halo::math::globals().effect_random_seed, 0.0f,
            0.2617994f);
        length = (i < hover_count ? ((struct vehicle_object *)obj)->vehicle.ground_lean : ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction) * 6.0f + 2.0f;
        delta.i = direction.i * length;
        delta.j = direction.j * length;
        delta.k = direction.k * length;
        if (halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::structure_bsp | halo::collision_test_flag::water_surface), (real_point3d *)(marker + 0x60), &delta, unit_index, &result)) {
            real_point3d points[3];
            real_vector3d vectors[3];
            real twice_dot;
            float scale;

            points[0] = result.point;
            points[1] = result.point;
            points[2] = result.point;
            vectors[0].i = -direction.i;
            vectors[0].j = -direction.j;
            vectors[0].k = -direction.k;
            vectors[1] = result.plane.normal;
            twice_dot = result.plane.normal.i * direction.i + result.plane.normal.j * direction.j +
                result.plane.normal.k * direction.k;
            twice_dot = twice_dot + twice_dot;
            vectors[2].i = direction.i - result.plane.normal.i * twice_dot;
            vectors[2].j = direction.j - result.plane.normal.j * twice_dot;
            vectors[2].k = direction.k - result.plane.normal.k * twice_dot;
            scale = 1.0f - result.t;
            halo::effects::effect_new_with_color(*(uint32_t *)&((struct Vehicle *)tag)->effect.tag_id, k_datum_index_none, 0, 3, (uint32_t)(uintptr_t)names, points, (uint32_t)(uintptr_t)vectors,
                scale, scale, 0, 0, 1);
        }
    }
}

/**
 * REWRITTEN from objdump. Runs only with tag +0x3ec set and the throttle (+0x338) above 0. For each of up to
 * 15 "hover thrusters" markers (stride 0x6c: +0x3c forward, +0x60 position) the forward is randomized (lo 0,
 * hi 15, seed 0x719cd4) and cast one unit from the marker (flags 0x61, excluding the unit). On a hit, v =
 * -forward.k * (1 - t) * throttle, capped at 1 and skipped unless it is above 0.
 *
 * @address 0x574bc0
 */
void VehicleView::create_hover_thruster_midpoint_effects()
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data;
    uint8_t markers[15 * 0x6c];
    int16_t count;
    int16_t i;
    static char *names[4] = { (char *)"incident", (char *)"normal", (char *)"reflected", (char *)"midpoint" };

    if (*(int32_t *)&((struct Vehicle *)tag)->effect.tag_id == -1 || !(((struct vehicle_object *)obj)->unit.driver_seat_power > 0.0f)) {
        return;
    }
    count = (int16_t)halo::objects::object_get_node_local_transform(unit_index, (char *)"hover thrusters", (object_marker *)markers, 0xf);
    for (i = 0; i < count; i++) {
        uint8_t *marker = markers + (int32_t)i * 0x6c;
        real_point3d *marker_position = (real_point3d *)(marker + 0x60);
        real_vector3d direction;
        real_vector3d delta;
        collision_result result;
        real v;

        halo::math::vector3d_randomize_direction(*(real_point3d *)(marker + 0x3c), &direction, halo::math::globals().effect_random_seed, 0.0f, 15.0f);
        delta = direction;
        if (!halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::structure_bsp | halo::collision_test_flag::water_surface), marker_position, &delta, unit_index, &result)) {
            continue;
        }
        v = -*(real *)(marker + 0x44) * (1.0f - result.t) * ((struct vehicle_object *)obj)->unit.driver_seat_power;
        if (v < 0.0f) {
            continue;
        }
        if (v > 1.0f) {
            v = 1.0f;
        } else if (!(v > 0.0f)) {
            continue;
        }
        {
            real_point3d points[4];
            real_vector3d vectors[4];
            real twice_dot;

            points[0] = result.point;
            points[1] = result.point;
            points[2] = result.point;
            points[3].x = (result.point.x + marker_position->x) * 0.5f;
            points[3].y = (result.point.y + marker_position->y) * 0.5f;
            points[3].z = (result.point.z + marker_position->z) * 0.5f;
            vectors[0].i = -direction.i;
            vectors[0].j = -direction.j;
            vectors[0].k = -direction.k;
            vectors[1] = result.plane.normal;
            twice_dot = result.plane.normal.k * direction.k + result.plane.normal.j * direction.j +
                result.plane.normal.i * direction.i;
            twice_dot = twice_dot + twice_dot;
            vectors[2].i = direction.i - result.plane.normal.i * twice_dot;
            vectors[2].j = direction.j - result.plane.normal.j * twice_dot;
            vectors[2].k = direction.k - result.plane.normal.k * twice_dot;
            vectors[3] = vectors[2];
            halo::effects::effect_new_with_color(*(uint32_t *)&((struct Vehicle *)tag)->effect.tag_id, k_datum_index_none, 0, 4, (uint32_t)(uintptr_t)names, points, (uint32_t)(uintptr_t)vectors, v, v,
                0, 0, 1);
        }
    }
}

}
