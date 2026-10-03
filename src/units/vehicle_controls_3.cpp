#include "halo/objects/record_access.hpp"
#include "halo/units/records.hpp"
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
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Vehicle *tag = (Vehicle *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    vehicle_data *vehicle = halo::units::vehicle_data_of(obj);
    unit_data *unit = halo::units::unit_data_of(obj);
    Physics *physics_tag = halo::objects::tag_as<Physics>(halo::objects::tag_handle(((Unit *)tag)->base.physics));
    int32_t node_count = physics_tag->powered_mass_points.count;
    bsp_leaf_reference bank_leaf = {obj->location_leaf_index, obj->location_cluster_index, 0};
    float bank_lookup = halo::scenario::location_view(&bank_leaf).water_surface_distance(&obj->position);
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
    } else {
        real_matrix4x3 basis;
        real_vector3d local_velocity;

        halo::math::matrix4x3_from_forward_up(obj->up, obj->forward, basis);
        basis.position = obj->position;
        halo::math::matrix4x3_inverse_transform_vector(local_velocity, obj->velocity, basis);

        if (vehicle->ground_lean > 0.0f) {
            float accel = tag->maximum_forward_speed;
            real_vector3d desired;

            if (test_flag(vehicle->flags, units::vehicle_flag::hovering)) {
                accel *= 0.8f;
            }
            desired.i = accel * unit->throttle.i - local_velocity.i;
            desired.j = accel * unit->throttle.j - local_velocity.j;
            desired.k = 0.0f;

            if (vehicle->landing_ticks != 0 && halo::x87::fabsf(angle) > 0.7853982f) {
                float t = (float)vehicle->landing_ticks * 0.05f;
                if (t > 0.98f) t = 0.98f;
                accel = (1.0f - t) * tag->speed_acceleration;
            } else {
                accel = tag->speed_acceleration;
            }

            halo::game::vector3d_clamp_length(&desired, accel);
            halo::math::matrix4x3_transform_vector(desired, desired, basis);

            {
                float scale = physics_tag->mass * vehicle->ground_lean;
                push.i += desired.i * scale;
                push.j += desired.j * scale;
                push.k += desired.k * scale;
            }
        }

        if (vehicle->ground_lean > 0.0f) {
            float target = (float)halo::libm::sqrt(halo::libm::fabs((double)angle) * 0.0069813174) * ((angle >= 0.0f) ? 1.0f : -1.0f);
            if (halo::x87::fabsf(target) > 0.0001f && angle / target < 2.0f) {
                target = angle * 0.5f;
            }
            target -= (obj->angular_velocity.i * obj->up.i + obj->angular_velocity.j * obj->up.j +
                       obj->angular_velocity.k * obj->up.k);
            if (target < -0.0034906587f) target = -0.0034906587f;
            else if (target > 0.0034906587f) target = 0.0034906587f;
            target *= physics_tag->zz_moment * vehicle->ground_lean;
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
                float lateral_len = (float)halo::libm::sqrt((double)(obj->forward.j * obj->forward.j + obj->forward.i * obj->forward.i));
                float right_len = (float)halo::libm::sqrt((double)(right.i * right.i + right.j * right.j));
                float fx = obj->forward.i, fy = obj->forward.j;
                float rx = right.i, ry = right.j;

                if (halo::x87::fabsf(lateral_len) >= 0.0001f) {
                    fx *= 1.0f / lateral_len;
                    fy *= 1.0f / lateral_len;
                }
                if (halo::x87::fabsf(right_len) >= 0.0001f) {
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
                            float a = halo::x87::fabsf(tx * unit->throttle.i) + 1.0f;
                            float b = halo::x87::fabsf(ty * unit->throttle.j) + 1.0f;
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
                        float accel = tx * physics_tag->yy_moment;
                        float turn = -((ty + 0.0f) * physics_tag->xx_moment);
                        float fade = 1.0f - vehicle->ground_lean;

                        push.i += (turn * obj->forward.i + fx * accel + global_origin3d_pointer->x) * fade;
                        push.j += (turn * obj->forward.j + fy * accel + global_origin3d_pointer->y) * fade;
                        push.k += (turn * obj->forward.k + right.k * accel + global_origin3d_pointer->z) * fade;
                    }
                }
            }
        }

        if (test_flag(vehicle->flags, units::vehicle_flag::hovering)) {
            float along = (obj->forward.i * obj->velocity.i + obj->forward.j * obj->velocity.j +
                          obj->forward.k * obj->velocity.k) / tag->maximum_forward_speed;
            real_vector3d cross;
            if (along < 0.0f) along = 0.0f;
            else if (along > 1.0f) along = 1.0f;

            cross.i = obj->up.j * obj->forward.k - obj->forward.j * obj->up.k;
            cross.j = obj->forward.i * obj->up.k - obj->up.i * obj->forward.k;
            cross.k = obj->up.i * obj->forward.j - obj->up.j * obj->forward.i;

            if (along > 0.0f) {
                float f1 = physics_tag->yy_moment * vehicle->ground_lean * along * -0.005817764f;
                float f2 = physics_tag->mass * vehicle->ground_lean * along * 0.004f;
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
                    scale = (1.0f - vehicle->ground_lean) * physics_tag->mass * t;
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

        halo::physics::object_physics_tick(unit_index, (powered_mass_point_state *)node_output, (uint32_t)contact_points, &push, &angular);
    }

    {
        float base = (vehicle->ground_lean >= 0.4f) ? vehicle->ground_lean : 0.4f;
        int32_t count = physics_tag->mass_points.count;
        int32_t active = 0;
        int32_t partial = 0;
        int32_t j;
        float fraction = 0.0f;
        float delta;

        for (j = 0; j < count; j++) {
            if (*(int16_t *)(&halo::objects::block_element<PhysicsMassPoint>(physics_tag->mass_points, j).powered_mass_point) != -1) {
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
