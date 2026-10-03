#include "halo/units/unit.hpp"
#include "halo/scenario/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/units/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/objects/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"
#include "halo/ai/api.hpp"
#include "halo/units/api.hpp"

static auto &global_origin3d_pointer = halo::link::ref<real_point3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &g_006966e4 = halo::link::ref<real_vector3d *>(halo::units::vars().g_006966e4);

namespace halo::units {

namespace {

constexpr uint32_t k_physics_mass_offset = 0x08;
constexpr uint32_t k_physics_turn_gain_offset = 0x50;
constexpr uint32_t k_physics_thrust_gain_offset = 0x54;
constexpr uint32_t k_physics_align_gain_offset = 0x58;
constexpr uint32_t k_physics_node_count_offset = 0x68;
constexpr uint32_t k_physics_contact_count_offset = 0x74;
constexpr uint32_t k_physics_contact_array_offset = 0x78;
constexpr uint32_t k_physics_contact_size = 0x80;
constexpr uint32_t k_physics_contact_marker_offset = 0x20;
constexpr uint32_t k_node_output_size = 0x60;
constexpr uint32_t k_contact_record_size = 0x130;
constexpr uint8_t k_contact_partial_bit = 0x10;

float physics_field(const uint8_t *physics_tag, uint32_t offset)
{
    return *(const float *)(physics_tag + offset);
}

float clamp_float(float value, float low, float high)
{
    if (value < low) {
        return low;
    }
    return value > high ? high : value;
}

/** Returns -1, 0 or 1 like the original's three-way float compare against zero. */
int32_t float_sign(float value)
{
    if (value == 0.0f) {
        return 0;
    }
    return value >= 0.0f ? 1 : -1;
}

}

/**
 * Per-tick drive of a hovering/flying vehicle: seeds each physics node with the seat power, and, when the
 * vehicle is not close to water or upside down, accumulates a force and a torque from the throttle, the
 * requested turn angle, the lean ground_lean and the hover/airborne state, scaled by the driver seat power.
 * The force and torque are handed to object_physics_tick together with the node and contact buffers.
 *
 * Afterwards ground_lean moves (at most 0.1 per tick) toward the fraction of active contact points that
 * report partial contact, and the hover thruster midpoint effects are created.
 *
 * @address 0x5734d0
 */
void VehicleView::calculate_wing_flex_controls(float angle, uint8_t *node_output, uint8_t *contact_points)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Vehicle *tag = (Vehicle *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint8_t *physics_tag = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    int32_t node_count = *(int32_t *)(physics_tag + k_physics_node_count_offset);
    bsp_leaf_reference bank_leaf = {obj->location_leaf_index, obj->location_cluster_index, 0};
    float water_distance = halo::scenario::location_view(&bank_leaf).water_surface_distance(&obj->position);
    real_vector3d force = *(real_vector3d *)global_origin3d_pointer;
    real_vector3d torque = *(real_vector3d *)global_origin3d_pointer;
    float seat_power = unit->driver_seat_power;
    const float mass = physics_field(physics_tag, k_physics_mass_offset);
    const float turn_gain = physics_field(physics_tag, k_physics_turn_gain_offset);
    const float thrust_gain = physics_field(physics_tag, k_physics_thrust_gain_offset);
    const float align_gain = physics_field(physics_tag, k_physics_align_gain_offset);
    const real_vector3d &up = obj->up;
    const real_vector3d &forward = obj->forward;
    const real_vector3d &angular_velocity = obj->angular_velocity;
    bool hovering = test_flag(vehicle->flags, units::vehicle_flag::hovering);
    int32_t i;

    for (i = 0; i < node_count; i++) {
        uint8_t *entry = node_output + i * k_node_output_size;
        *(float *)(entry + 0x18) = seat_power;
        *(uint32_t *)(entry + 0x1c) = 0;
        *(uint32_t *)(entry + 0x20) = 0;
        *(uint32_t *)(entry + 0x24) = 0;
        *(uint32_t *)(entry + 0x28) = 0x3f800000;
    }

    if (!(water_distance >= 0.5f) && up.k > -0.2f) {
        real_matrix4x3 basis;
        real_vector3d local_velocity;

        halo::math::matrix4x3_from_forward_up(obj->up, obj->forward, basis);
        basis.position = obj->position;
        halo::math::matrix4x3_inverse_transform_vector(local_velocity, obj->velocity, basis);

        if (vehicle->ground_lean > 0.0f) {
            float top_speed = tag->maximum_forward_speed;
            float acceleration = tag->speed_acceleration;
            real_vector3d desired;

            if (hovering) {
                top_speed *= 0.8f;
            }
            desired.i = top_speed * unit->throttle.i - local_velocity.i;
            desired.j = top_speed * unit->throttle.j - local_velocity.j;
            desired.k = 0.0f;

            if (vehicle->landing_ticks != 0 && halo::x87::fabsf(angle) > 0.7853982f) {
                float landing_fraction = clamp_float((float)vehicle->landing_ticks * 0.05f, -1.0e30f, 0.98f);
                acceleration = (1.0f - landing_fraction) * acceleration;
            }

            halo::game::vector3d_clamp_length(&desired, acceleration);
            halo::math::matrix4x3_transform_vector(desired, desired, basis);

            {
                float scale = mass * vehicle->ground_lean;
                force.i += desired.i * scale;
                force.j += desired.j * scale;
                force.k += desired.k * scale;
            }
        }

        if (vehicle->ground_lean > 0.0f) {
            float spin_about_up = up.k * angular_velocity.k + up.j * angular_velocity.j + up.i * angular_velocity.i;
            float target = (float)(halo::libm::sqrt(halo::libm::fabs((double)angle) * (double)0.0069813174f) * float_sign(angle));

            if (halo::x87::fabsf(target) > 0.0001f && angle / target < 2.0f) {
                target = angle * 0.5f;
            }
            target -= spin_about_up;
            target = clamp_float(target, -0.0034906587f, 0.0034906587f);
            target = target * align_gain * vehicle->ground_lean;
            torque.i += target * up.i;
            torque.j += target * up.j;
            torque.k += target * up.k;
        }

        if (vehicle->ground_lean < 1.0f) {
            const real_vector3d &origin = *(real_vector3d *)global_origin3d_pointer;
            real_vector3d right;
            float forward_flat_x = forward.i, forward_flat_y = forward.j;
            float right_flat_x, right_flat_y;
            float forward_flat_length, right_flat_length;
            float drive_x, drive_y;

            right.i = up.j * forward.k - up.k * forward.j;
            right.j = forward.i * up.k - forward.k * up.i;
            right.k = forward.j * up.i - forward.i * up.j;
            right_flat_x = right.i;
            right_flat_y = right.j;

            forward_flat_length = (float)halo::libm::sqrt((double)(forward_flat_x * forward_flat_x + forward_flat_y * forward_flat_y));
            if (halo::x87::fabsf(forward_flat_length) >= 0.0001f) {
                float inverse = 1.0f / forward_flat_length;
                forward_flat_x *= inverse;
                forward_flat_y *= inverse;
            }
            right_flat_length = (float)halo::libm::sqrt((double)(right_flat_y * right_flat_y + right_flat_x * right_flat_x));
            if (halo::x87::fabsf(right_flat_length) >= 0.0001f) {
                float inverse = 1.0f / right_flat_length;
                right_flat_y *= inverse;
                right_flat_x *= inverse;
            }

            if (!(up.k > 0.0f)) {
                drive_x = unit->throttle.i * 0.0015514038f + g_006966e4->i;
                drive_y = unit->throttle.j * 0.0015514038f + g_006966e4->j;
            } else {
                float forward_up = forward_flat_y * up.j + forward_flat_x * up.i;
                float right_up = right_flat_y * up.j + right_flat_x * up.i;
                float right_spin = right_flat_y * angular_velocity.j + right_flat_x * angular_velocity.i;
                float forward_spin = forward_flat_y * angular_velocity.j + forward_flat_x * angular_velocity.i;
                float error_x = (g_006966e4->i - forward_up) - right_spin * 15.0f;
                float error_y = (g_006966e4->j - right_up) - (-forward_spin) * 15.0f;
                float boost_x = clamp_float(halo::x87::fabsf(error_x) * (float)float_sign(error_x * unit->throttle.i) + 1.0f, 0.3f, 2.5f);
                float boost_y = clamp_float(halo::x87::fabsf(error_y) * (float)float_sign(error_y * unit->throttle.j) + 1.0f, 0.3f, 2.5f);
                float blend = (1.0f - vehicle->ground_lean) * 0.0038785094f;
                float base_x = boost_x * unit->throttle.i * 0.0015514038f + g_006966e4->i;
                float base_y = boost_y * unit->throttle.j * 0.0015514038f + g_006966e4->j;

                drive_x = base_x + blend * error_x;
                drive_y = blend * error_y + base_y;
            }

            {
                float thrust = drive_x * thrust_gain;
                float turn = -(drive_y * turn_gain);
                float fade = 1.0f - vehicle->ground_lean;
                real_vector3d lever;

                lever.i = right.i * thrust + origin.i;
                lever.j = right.j * thrust + origin.j;
                lever.k = right.k * thrust + origin.k;
                lever.i = forward.i * turn + lever.i;
                lever.j = forward.j * turn + lever.j;
                lever.k = forward.k * turn + lever.k;
                torque.i = lever.i * fade + torque.i;
                torque.j = lever.j * fade + torque.j;
                torque.k = lever.k * fade + torque.k;
            }
        }

        if (hovering) {
            const real_vector3d &world_up = *halo::math::globals().global_up3d_pointer;
            float along = (forward.k * obj->velocity.k + forward.j * obj->velocity.j + forward.i * obj->velocity.i) /
                tag->maximum_forward_speed;
            real_vector3d cross;

            along = clamp_float(along, 0.0f, 1.0f);
            cross.i = up.j * forward.k - forward.j * up.k;
            cross.j = forward.i * up.k - up.i * forward.k;
            cross.k = forward.j * up.i - up.j * forward.i;

            if (along > 0.0f) {
                float torque_scale = thrust_gain * vehicle->ground_lean * along * -0.005817764f;
                float lift_scale = mass * vehicle->ground_lean * along * 0.004f;

                torque.i = cross.i * torque_scale + torque.i;
                torque.j = cross.j * torque_scale + torque.j;
                torque.k = cross.k * torque_scale + torque.k;
                force.i = lift_scale * world_up.i + force.i;
                force.j = lift_scale * world_up.j + force.j;
                force.k = lift_scale * world_up.k + force.k;
            }

            if (vehicle->airborne_ticks != 0) {
                real_vector3d axis;

                halo::math::vector3d_cross_product(axis, world_up, cross);
                if (halo::math::vector3d_normalize_with_length(axis) > 0.0f) {
                    float air_fraction = clamp_float(1.0f - (float)vehicle->airborne_ticks * 0.033333335f, 0.0f, 1.0f);
                    float scale = air_fraction * ((1.0f - vehicle->ground_lean) * mass);
                    float side_scale = scale * 0.002f;
                    float up_scale = scale * 0.001f;

                    force.i = axis.i * side_scale + force.i;
                    force.j = axis.j * side_scale + force.j;
                    force.k = axis.k * side_scale + force.k;
                    force.i = up_scale * world_up.i + force.i;
                    force.j = up_scale * world_up.j + force.j;
                    force.k = up_scale * world_up.k + force.k;
                }
            }
        }

        force.i *= seat_power;
        force.j *= seat_power;
        force.k *= seat_power;
        torque.i *= seat_power;
        torque.j *= seat_power;
        torque.k *= seat_power;
    }

    halo::physics::object_physics_tick(unit_index, (powered_mass_point_state *)node_output, (uint32_t)contact_points, &force, &torque);

    {
        float base = (vehicle->ground_lean >= 0.4f) ? vehicle->ground_lean : 0.4f;
        int32_t count = *(int32_t *)(physics_tag + k_physics_contact_count_offset);
        const uint8_t *contacts = *(uint8_t **)(physics_tag + k_physics_contact_array_offset);
        int16_t active = 0;
        int16_t partial = 0;
        float fraction = 0.0f;
        float delta;
        int16_t j;

        for (j = 0; j < count; j++) {
            if (*(int16_t *)(contacts + j * k_physics_contact_size + k_physics_contact_marker_offset) != -1) {
                active++;
                if ((contact_points[j * k_contact_record_size] & k_contact_partial_bit) != 0) {
                    partial++;
                }
            }
        }
        if (active > 0) {
            fraction = (float)partial / (float)active;
        }

        fraction = clamp_float(base * fraction, 0.0f, 1.0f);
        delta = fraction - vehicle->ground_lean;
        if (delta > 0.1f) {
            fraction = vehicle->ground_lean + 0.1f;
        } else if (delta < -0.1f) {
            fraction = vehicle->ground_lean - 0.1f;
        }
        vehicle->ground_lean = fraction;
    }

    VehicleView(unit_index).create_hover_thruster_midpoint_effects();
}

}
