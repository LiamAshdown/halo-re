#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::physics {

/**
 * Point physics for particles and massless movers, plus the scalar range helpers.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct PhysicsMotion {
    static void clamp_value_to_spring_range(float *value, physics_scalar_rates *rates, float step);
    static int16_t resolve_material_type(uint32_t object_index, int16_t vertex_slot);
    static void scalar_advance_and_wrap(physics_scalar_range *range, float *value, uint8_t wrap, float delta);
    static float scalar_approach_direction(physics_scalar_range *range, float value, uint8_t wrap, float target);
    static uint8_t scalar_move_toward_target(physics_scalar_range *range, float *value, uint8_t wrap, float target, float rate);
    static uint8_t scalar_step_to_target_clamped(physics_scalar_rates *rates, float *value, float target, float step);
    static void interpolate(PointPhysics *out, const PointPhysics *from, const PointPhysics *to, float fraction);
    static uint32_t tick(real_vector3d *velocity, uint32_t flags_arg, PointPhysics *definition, bsp_leaf_reference *out_leaf, uint32_t unused_param_4, real_point3d *position, real_vector3d *wind, real_vector3d *out_normal, int16_t *out_material_type, real radius, real dt);
    static void vector3d_project_onto_direction(real_vector3d *out, const real_vector3d *axis, const real_vector3d *v);
};

}
