/**
 * Point physics for particles and massless movers, plus the scalar range helpers.
 */

#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "tags.h"
#include "math.h"
#include "physics.h"
#include "memory.h"
#include "cache.h"
#include "objects.h"
#include "projectiles.h"

#include "halo/physics/motion.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/core/libm.hpp"


namespace halo::physics {

/**
 * Advances *value one spring step in the direction of step's sign (using the matching
 * acceleration_positive/acceleration_negative scale, itself scaled by |step|, as a spring
 * toward the corresponding zero-crossing), then clamps the result to
 * +/-(|step| * maximum_positive/maximum_negative).
 *
 * @address 0x50b370
 */
void PhysicsMotion::clamp_value_to_spring_range(float *value, physics_scalar_rates *rates, float step)
{
    float magnitude = (float)halo::libm::fabs((double)step);
    float accel_positive = magnitude * rates->acceleration_positive;
    float accel_negative = magnitude * rates->acceleration_negative;

    if (step <= 0.0f) {
        if (0.0f <= step) {
            return;
        }

        float current = *value;
        float stepped;
        if (*value < accel_negative) {
            if (current > 0.0f) {
                stepped = (current / accel_negative - 1.0f) * accel_positive;
            } else {
                stepped = current - accel_positive;
            }
        } else {
            stepped = current - accel_negative;
        }
        *value = stepped;

        float lower_bound = -(magnitude * rates->maximum_negative);
        if (lower_bound <= *value) {
            lower_bound = *value;
        }
        *value = lower_bound;
        return;
    }

    float stepped;
    if (-accel_negative < *value) {
        if (*value < 0.0f) {
            stepped = (*value / accel_negative + 1.0f) * accel_positive;
        } else {
            stepped = accel_positive + *value;
        }
    } else {
        stepped = accel_negative + *value;
    }
    *value = stepped;

    float upper_bound = magnitude * rates->maximum_positive;
    if (*value <= upper_bound) {
        return;
    }
    *value = upper_bound;
}

}

namespace halo::physics {

/**
 * Resolves a global MaterialType_t for vertex_slot: when object_index names a real object,
 * indexes that object's ModelCollisionGeometry.materials (via its Object tag's collision_model
 * dependency, tag +0x7c); when object_index is -1 (the world), indexes the current structure
 * BSP's collision_materials instead. Returns -1 when vertex_slot itself is -1.
 *
 * @address 0x507a40
 */
int16_t PhysicsMotion::resolve_material_type(uint32_t object_index, int16_t vertex_slot)
{
    if (vertex_slot == -1) {
        return -1;
    }

    if (object_index != halo::k_dword_none) {
        object *obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
        void *object_tag_data = halo::cache::globals().tag_instances[obj->definition_tag & halo::k_slot_mask].data;
        int32_t collision_model_id = *(int32_t *)((uint8_t *)object_tag_data + 0x7c);
        ModelCollisionGeometry *geometry =
            (ModelCollisionGeometry *)halo::cache::globals().tag_instances[(uint16_t)collision_model_id].data;

        return ((ModelCollisionGeometryMaterial *)geometry->materials.pointer)[vertex_slot].material_type;
    }

    return ((ScenarioStructureBSPCollisionMaterial *)
        halo::scenario::globals().structure_bsp->collision_materials.pointer)[vertex_slot].material;
}

}

namespace halo::physics {

/**
 * Adds delta to *value, then keeps the result inside [range->lower, range->upper]: overshooting
 * the upper edge either wraps by the range's span (wrap != 0) or clamps to range->upper;
 * undershooting the lower edge does the mirror image against range->lower.
 *
 * @address 0x50b290
 */
void PhysicsMotion::scalar_advance_and_wrap(physics_scalar_range *range, float *value, uint8_t wrap, float delta)
{
    float new_value = *value + delta;
    *value = new_value;

    if (range->lower <= new_value) {
        if (range->upper < new_value) {
            if (wrap != 0) {
                *value = new_value - (range->upper - range->lower);
                return;
            }
            *value = range->upper;
        }
        return;
    }

    if (wrap != 0) {
        *value = (range->upper - range->lower) + new_value;
        return;
    }
    *value = range->lower;
}

}

namespace halo::physics {

/**
 * Returns +1.0 / -1.0 for the direction *value* should move to reach *target*, or 0.0 when it
 * is already there. When wrap is set and going directly would cross more than half the range's
 * span, the direction is flipped so the caller wraps the short way around instead.
 *
 * @address 0x50b4d0
 */
float PhysicsMotion::scalar_approach_direction(physics_scalar_range *range, float value, uint8_t wrap, float target)
{
    float delta = target - value;
    if (delta != 0.0f) {
        if (wrap != 0 && (range->upper - range->lower) * 0.5f < (float)halo::libm::fabs((double)delta)) {
            delta = -delta;
        }
        return (delta > 0.0f) ? 1.0f : -1.0f;
    }
    return delta;
}

}

namespace halo::physics {

/**
 * Steps *value one tick toward target at the given rate, honoring range's wraparound the same
 * way physics_scalar_advance_and_wrap does. Returns 1 and snaps *value exactly to target once
 * the step reaches or passes it (or it was already there); returns 0 while still en route.
 *
 * @address 0x50b2f0
 */
uint8_t PhysicsMotion::scalar_move_toward_target(physics_scalar_range *range, float *value, uint8_t wrap, float target, float rate)
{
    float direction = halo::physics::physics_scalar_approach_direction(range, *value, wrap, target);
    if (direction != 0.0f) {
        halo::physics::physics_scalar_advance_and_wrap(range, value, wrap, direction * rate);
        if (halo::physics::physics_scalar_approach_direction(range, *value, wrap, target) == direction) {
            return 0;
        }
    }
    *value = target;
    return 1;
}

}

namespace halo::physics {

/**
 * Steps *value one spring tick toward target (up by step if below, down by step if above), then
 * reports whether that step reached or passed target. Snaps *value exactly to target on the
 * tick it is reached; leaves it at the stepped-and-clamped value otherwise.
 *
 * @address 0x50b460
 */
uint8_t PhysicsMotion::scalar_step_to_target_clamped(physics_scalar_rates *rates, float *value, float target, float step)
{
    if (*value <= target) {
        if (*value < target) {
            halo::physics::physics_clamp_value_to_spring_range(value, rates, step);
            if (*value < target) {
                return 0;
            }
            *value = target;
            return 1;
        }

    } else {
        halo::physics::physics_clamp_value_to_spring_range(value, rates, -step);
        if (*value > target) {
            return 0;
        }
        *value = target;
    }
    return 1;
}

}

namespace halo::physics {

/**
 * Linearly interpolates the tunable fields of two point_physics definitions.
 *
 * Original register convention: EAX -> out, ECX -> from, EDX -> to, stack -> fraction.
 *
 * @address 0x50b9e0
 */
void PhysicsMotion::interpolate(PointPhysics *out, const PointPhysics *from, const PointPhysics *to, float fraction)
{
    float inverse = 1.0f - fraction;

    out->flags = from->flags;
    out->density = inverse * from->density + fraction * to->density;
    out->water_gravity_scale = inverse * from->water_gravity_scale + fraction * to->water_gravity_scale;
    out->air_gravity_scale = inverse * from->air_gravity_scale + fraction * to->air_gravity_scale;
    out->mass_scale = inverse * from->mass_scale + fraction * to->mass_scale;
    out->air_friction = inverse * from->air_friction + fraction * to->air_friction;
    out->water_friction = inverse * from->water_friction + fraction * to->water_friction;
    out->surface_friction = inverse * from->surface_friction + fraction * to->surface_friction;
    out->elasticity = inverse * from->elasticity + fraction * to->elasticity;
}

}

static auto &k_water_density = halo::link::ref<float>(halo::game::vars().k_water_density);
static auto &k_air_density = halo::link::ref<float>(halo::game::vars().k_air_density);
namespace halo::physics {

/**
 * One tick of a massless point mover driven by a PointPhysics tag: probes wind (full or "cheap"
 * depending on flags_arg bit 0), applies medium-dependent gravity and drag, nudges velocity
 * toward the wind vector, then resolves up to k_physics_collision_iterations bounces against the
 * world -- each bounce scales the tangential velocity by (1 - surface_friction) and the normal
 * component by -elasticity, and reports the closest hit's leaf/normal/material_type through the
 * optional out parameters. Returns a point_physics_result_flags bit field.
 *
 * @address 0x50b530
 */
uint32_t PhysicsMotion::tick(real_vector3d *velocity, uint32_t flags_arg, PointPhysics *definition, bsp_leaf_reference *out_leaf, uint32_t unused_param_4, real_point3d *position, real_vector3d *wind, real_vector3d *out_normal, int16_t *out_material_type, real radius, real dt)
{
    uint32_t result_flags;
    real gravity_scale, friction, density, radius_cubed;
    real_vector3d probed_wind;
    uint8_t in_water;
    uint32_t collision_flags;
    int16_t bounce;

    if (dt == 0.0f) {
        return 0;
    }

    radius_cubed = radius * radius * radius;

    {
        uint32_t wind_mode_mask = ((definition->flags & 0xff) >> 3 & 1);
        if ((definition->flags & 0x10) != 0) {
            wind_mode_mask |= 2;
        }

        if ((flags_arg & 1) == 0) {
            in_water = halo::effects::ambient_color_marker_visible(out_leaf, position, &probed_wind, wind_mode_mask);
        } else {
            in_water = (uint8_t)(flags_arg >> 1) & 1;

            halo::effects::ambient_color_for_marker((int16_t)unused_param_4, position, (uint8_t)wind_mode_mask, &probed_wind);
        }
    }

    if (in_water == 0) {
        result_flags = _point_physics_in_air_bit;
        gravity_scale = definition->air_gravity_scale;
        friction = definition->air_friction;
        density = k_air_density;
    } else {
        result_flags = _point_physics_in_water_bit;
        gravity_scale = definition->water_gravity_scale;
        friction = definition->water_friction;
        density = k_water_density;
    }

    density += definition->mass_scale;
    friction = radius * radius * friction;
    density *= radius_cubed;

    {
        real wind_nudge_fraction = dt / density;

        if ((definition->flags & 0x20) != 0) {
            gravity_scale = 0.0f;
        }

        if (wind != (real_vector3d *)0 && density != 0.0f) {
            velocity->i += wind_nudge_fraction * wind->i;
            velocity->j += wind_nudge_fraction * wind->j;
            velocity->k += wind_nudge_fraction * wind->k;
        }

        velocity->k += k_physics_gravity * 30.0f * 30.0f * gravity_scale * dt;

        {
            real drag_lerp;
            if (density == 0.0f) {
                drag_lerp = (friction == 0.0f) ? 0.0f : 1.0f;
            } else {
                drag_lerp = wind_nudge_fraction * friction;
                if (drag_lerp < 0.0f) {
                    drag_lerp = 0.0f;
                } else if (drag_lerp > 1.0f) {
                    drag_lerp = 1.0f;
                }
            }
            velocity->i += (probed_wind.i - velocity->i) * drag_lerp;
            velocity->j += (probed_wind.j - velocity->j) * drag_lerp;
            velocity->k += (probed_wind.k - velocity->k) * drag_lerp;
        }
    }

    {
        uint32_t flags = definition->flags;
        if ((flags & 4) == 0) {
            collision_flags = 1;
        } else if ((flags_arg & 4) != 0) {
            collision_flags = 1;
        } else {
            collision_flags = 0x40 | 1;
        }
        if ((flags & 2) != 0 && (flags_arg & 4) == 0) {
            collision_flags |= 0x20;
        }
        if ((flags & 1) != 0) {
            collision_flags |= 0x80 | 0x4200;
        }
    }

    for (bounce = 0; bounce <= k_physics_collision_iterations - 1; bounce++) {
        real_vector3d delta;
        collision_result hit;
        uint8_t collided;

        delta.i = dt * velocity->i;
        delta.j = dt * velocity->j;
        delta.k = dt * velocity->k;

        collided = halo::physics::collision_test_movement_segment(collision_flags, position, &delta, 0xffffffff, &hit);
        if (collided == 0) {
            if (hit.leaf.leaf_index != -1) {
                *out_leaf = hit.leaf;
            }
            position->x = hit.point.x;
            position->y = hit.point.y;
            position->z = hit.point.z;
            return result_flags;
        }

        {
            real step_dt = (radius < 0.005f) ? radius : 0.005f;
            real dot_nv, tangential_j, tangential_k, normal_j_term, normal_k_term;

            if (hit.type == 0) {
                result_flags |= _point_physics_hit_water_surface_bit;
            } else if (hit.type == 2 || (hit.type == 3 && (definition->flags & 1) != 0)) {
                result_flags |= _point_physics_collided_bit;
            }

            if (out_normal != (real_vector3d *)0) {
                *out_normal = hit.plane.normal;
            }
            if (out_material_type != (int16_t *)0) {
                *out_material_type = hit.material_type;
            }

            dot_nv = hit.plane.normal.j * velocity->j + hit.plane.normal.k * velocity->k + hit.plane.normal.i * velocity->i;
            normal_j_term = dot_nv * hit.plane.normal.j;
            normal_k_term = dot_nv * hit.plane.normal.k;
            tangential_j = velocity->j - normal_j_term;
            tangential_k = velocity->k - normal_k_term;

            velocity->i = (1.0f - definition->surface_friction) * (velocity->i - hit.plane.normal.i * dot_nv) -
                hit.plane.normal.i * dot_nv * definition->elasticity;
            velocity->j = (1.0f - definition->surface_friction) * tangential_j - normal_j_term * definition->elasticity;
            velocity->k = (1.0f - definition->surface_friction) * tangential_k - normal_k_term * definition->elasticity;

            if (hit.leaf.leaf_index != -1) {
                *out_leaf = hit.leaf;
            }

            position->x = hit.plane.normal.i * step_dt + hit.point.x;
            position->y = hit.plane.normal.j * step_dt + hit.point.y;
            position->z = hit.plane.normal.k * step_dt + hit.point.z;

            dt -= hit.t * dt;
            if (dt == 0.0f) {
                return result_flags;
            }
        }
    }

    return result_flags;
}

}

namespace halo::physics {

/**
 * Projects v onto the (not necessarily unit) axis.
 *
 * Original register convention: ECX -> out, EAX -> axis, EDX -> v.
 *
 * @address 0x506760
 */
void PhysicsMotion::vector3d_project_onto_direction(real_vector3d *out, const real_vector3d *axis, const real_vector3d *v)
{
    float scale = (v->k * axis->k + v->j * axis->j + v->i * axis->i) /
                  (axis->i * axis->i + axis->j * axis->j + axis->k * axis->k);

    out->i = scale * axis->i;
    out->j = scale * axis->j;
    out->k = scale * axis->k;
}

}
