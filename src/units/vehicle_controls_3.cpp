#include "halo/units/unit.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"

extern "C" {
extern data_array *object_data;
extern real_point3d *global_origin3d_pointer;
extern real_vector3d *g_006966e4;
extern float scenario_location_water_surface_distance(void);
extern void vector3d_clamp_length(real_vector3d *v, real max_length);
extern void object_physics_tick(uint32_t unit_index, void *node_output, void *contact_points, void *extra_force, void *extra_torque);
extern double sqrt(double x);
extern double fabs(double x);
extern float fabsf(float x);
}

namespace halo::units {

/**
 * Computes per-marker flex/sway transforms (e.g. for wing or control-surface animation) on a flying
 * vehicle-type unit each tick, and updates its ground_lean toward the fraction of contact points reporting
 * partial contact. UNSURE: reproduced only partially; see the file header before trusting this file's math.
 *
 * @address 0x5734d0
 */
void VehicleView::calculate_wing_flex_controls(float angle, uint8_t *node_output, uint8_t *contact_points)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint8_t *physics_tag = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    int32_t node_count = *(int32_t *)(physics_tag + 0x68);
    float bank_lookup = scenario_location_water_surface_distance();
    real_vector3d push = *(real_vector3d *)global_origin3d_pointer;
    real_vector3d angular = *(real_vector3d *)global_origin3d_pointer;
    int32_t i;

    for (i = 0; i < node_count; i++) {
        uint8_t *entry = node_output + i * 0x60;
        *(float *)(entry + 0x18) = unit->driver_seat_power;
        *(uint32_t *)(entry + 0x1c) = 0;
        *(uint32_t *)(entry + 0x20) = 0;
        *(uint32_t *)(entry + 0x24) = 0;
        *(uint32_t *)(entry + 0x28) = 0x3f800000;
    }

    if (bank_lookup >= 0.5f || obj->up.k <= -0.2f) {
        halo::physics::object_physics_tick(unit_index, (powered_mass_point_state *)node_output, (uint32_t)contact_points, &push, &angular);
        goto ground_lean_update;
    }

    {
        real_matrix4x3 basis;
        real_vector3d local_velocity;

        halo::math::matrix4x3_from_forward_up(obj->up, obj->forward, basis);
        basis.position = obj->position;
        halo::math::matrix4x3_inverse_transform_vector(local_velocity, obj->velocity, basis);

        if (vehicle->ground_lean > 0.0f) {
            float accel = tag->maximum_forward_speed;
            real_vector3d desired;

            if ((vehicle->flags & 8) != 0) {
                accel *= 0.8f;
            }
            desired.i = accel * unit->throttle.i - local_velocity.i;
            desired.j = accel * unit->throttle.j - local_velocity.j;
            desired.k = 0.0f;

            if (vehicle->landing_ticks != 0 && fabsf(angle) > 0.7853982f) {
                float t = (float)vehicle->landing_ticks * 0.05f;
                if (t > 0.98f) t = 0.98f;
                accel = (1.0f - t) * tag->speed_acceleration;
            } else {
                accel = tag->speed_acceleration;
            }

            vector3d_clamp_length(&desired, accel);
            halo::math::matrix4x3_transform_vector(desired, desired, basis);

            {
                float scale = *(float *)(physics_tag + 8) * vehicle->ground_lean;
                push.i += desired.i * scale;
                push.j += desired.j * scale;
                push.k += desired.k * scale;
            }
        }

        if (vehicle->ground_lean > 0.0f) {
            float target = (float)sqrt(fabs((double)angle) * 0.0069813174) * ((angle >= 0.0f) ? 1.0f : -1.0f);
            if (fabsf(target) > 0.0001f && angle / target < 2.0f) {
                target = angle * 0.5f;
            }
            target -= (obj->angular_velocity.i * obj->up.i + obj->angular_velocity.j * obj->up.j +
                       obj->angular_velocity.k * obj->up.k);
            if (target < -0.0034906587f) target = -0.0034906587f;
            else if (target > 0.0034906587f) target = 0.0034906587f;
            target *= *(float *)(physics_tag + 0x58) * vehicle->ground_lean;
            angular.i += target * obj->up.i;
            angular.j += target * obj->up.j;
            angular.k += target * obj->up.k;
        }

        if (vehicle->ground_lean < 1.0f) {
            real_vector3d right;
            right.i = obj->up.j * obj->forward.k - obj->up.k * obj->forward.j;
            right.j = obj->forward.i * obj->up.k - obj->forward.k * obj->up.i;
            right.k = obj->forward.j * obj->up.i - obj->forward.i * obj->up.j;

            {
                float lateral_len = (float)sqrt((double)(obj->forward.j * obj->forward.j + obj->forward.i * obj->forward.i));
                float right_len = (float)sqrt((double)(right.i * right.i + right.j * right.j));
                float fx = obj->forward.i, fy = obj->forward.j;
                float rx = right.i, ry = right.j;

                if (fabsf(lateral_len) >= 0.0001f) {
                    fx *= 1.0f / lateral_len;
                    fy *= 1.0f / lateral_len;
                }
                if (fabsf(right_len) >= 0.0001f) {
                    rx *= 1.0f / right_len;
                    ry *= 1.0f / right_len;
                }

                {
                    float tx, ty;
                    if (obj->up.k <= 0.0f) {
                        tx = unit->throttle.i * 0.0015514038f + g_006966e4->i;
                        ty = unit->throttle.j * 0.0015514038f;
                    } else {
                        float f = ry * obj->forward.i;
                        tx = (g_006966e4->i - (fx * obj->forward.i + fy * obj->up.j)) -
                             (ry * obj->angular_velocity.i + rx * obj->angular_velocity.j) * 15.0f;
                        ty = (g_006966e4->j - (f + rx * obj->up.j)) -
                             -(fx * obj->angular_velocity.i + fy * obj->angular_velocity.j) * 15.0f;
                        {
                            float a = fabsf(tx * unit->throttle.i) + 1.0f;
                            float b = fabsf(ty * unit->throttle.j) + 1.0f;
                            if (a < 0.3f) a = 0.3f; else if (a > 2.5f) a = 2.5f;
                            if (b < 0.3f) b = 0.3f; else if (b > 2.5f) b = 2.5f;
                            {
                                float blend = (1.0f - obj->up.j) * 0.0038785094f;
                                float txx = blend * tx + a * unit->throttle.i * 0.0015514038f + g_006966e4->i;
                                float tyy = blend * ty;
                                tx = txx; ty = tyy;
                            }
                        }
                    }

                    {
                        float accel = tx * *(float *)(physics_tag + 0x54);
                        float turn = -((ty + 0.0f) * *(float *)(physics_tag + 0x50));
                        float fade = 1.0f - vehicle->ground_lean;

                        push.i += (turn * obj->forward.i + fx * accel + global_origin3d_pointer->x) * fade;
                        push.j += (turn * obj->forward.j + fy * accel + global_origin3d_pointer->y) * fade;
                        push.k += (turn * obj->forward.k + right.k * accel + global_origin3d_pointer->z) * fade;
                    }
                }
            }
        }

        if ((vehicle->flags & 8) != 0) {
            float along = (obj->forward.i * obj->velocity.i + obj->forward.j * obj->velocity.j +
                          obj->forward.k * obj->velocity.k) / tag->maximum_forward_speed;
            real_vector3d cross;
            if (along < 0.0f) along = 0.0f;
            else if (along > 1.0f) along = 1.0f;

            cross.i = obj->up.j * obj->forward.k - obj->forward.j * obj->up.k;
            cross.j = obj->forward.i * obj->up.k - obj->up.i * obj->forward.k;
            cross.k = obj->up.i * obj->forward.j - obj->up.j * obj->forward.i;

            if (along > 0.0f) {
                float f1 = *(float *)(physics_tag + 0x54) * vehicle->ground_lean * along * -0.005817764f;
                float f2 = *(float *)(physics_tag + 8) * vehicle->ground_lean * along * 0.004f;
                push.i += cross.i * f1;
                push.j += cross.j * f1;
                push.k += cross.k * f1;
                angular.i += f2 * halo::math::globals().global_up3d_pointer->i;
                angular.j += f2 * halo::math::globals().global_up3d_pointer->j;
                angular.k += f2 * halo::math::globals().global_up3d_pointer->k;
            }

            if (vehicle->airborne_ticks != 0) {
                real_vector3d axis = cross;
                real length;
                halo::math::vector3d_cross_product(axis, *halo::math::globals().global_up3d_pointer, cross);
                length = halo::math::vector3d_normalize_with_length(axis);
                if (length > 0.0f) {
                    float t = 1.0f - (float)vehicle->airborne_ticks * 0.033333335f;
                    float scale, s1, s2;
                    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
                    scale = (1.0f - vehicle->ground_lean) * *(float *)(physics_tag + 8) * t;
                    s1 = scale * 0.002f;
                    s2 = scale * 0.001f;
                    push.i += s2 * halo::math::globals().global_up3d_pointer->i + cross.i * s1;
                    push.j += s2 * halo::math::globals().global_up3d_pointer->j + cross.j * s1;
                    push.k += s2 * halo::math::globals().global_up3d_pointer->k + s1 * cross.k;
                }
            }
        }

        push.i *= unit->driver_seat_power; push.j *= unit->driver_seat_power; push.k *= unit->driver_seat_power;
        angular.i *= unit->driver_seat_power; angular.j *= unit->driver_seat_power; angular.k *= unit->driver_seat_power;
    }

    halo::physics::object_physics_tick(unit_index, (powered_mass_point_state *)node_output, (uint32_t)contact_points, &push, &angular);

ground_lean_update:
    {
        float base = (vehicle->ground_lean >= 0.4f) ? vehicle->ground_lean : 0.4f;
        int32_t count = *(int32_t *)(physics_tag + 0x74);
        int32_t active = 0;
        int32_t partial = 0;
        int32_t j;
        float fraction = 0.0f;
        float delta;

        for (j = 0; j < count; j++) {
            if (*(int16_t *)(*(uint8_t **)(physics_tag + 0x78) + j * 0x80 + 0x20) != -1) {
                active++;
                if ((contact_points[j * 0x130] & 0x10) != 0) {
                    partial++;
                }
            }
        }
        if (active > 0) {
            fraction = (float)partial / (float)active;
        }

        fraction *= base;
        if (fraction < 0.0f) fraction = 0.0f;
        else if (fraction > 1.0f) fraction = 1.0f;

        delta = fraction - vehicle->ground_lean;
        if (delta <= 0.1f) {
            if (delta < -0.1f) {
                fraction = vehicle->ground_lean - 0.1f;
            }
        } else {
            fraction = vehicle->ground_lean + 0.1f;
        }
        vehicle->ground_lean = fraction;
    }

    VehicleView(unit_index).create_hover_thruster_midpoint_effects();
}

}
