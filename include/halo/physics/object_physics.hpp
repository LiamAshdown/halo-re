#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::physics {

/**
 * Per-tick force and torque integration of an object carrying a physics tag, one mass point at a time.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct ObjectPhysics {
    static uint8_t add_mass_point_shapes(float x_offset, float y_offset, object_physics_context *context, int16_t *model_counts);
    static void blend_friction_axes(int16_t friction_type, float parallel_scale, float perpendicular_scale, float *friction, real_vector3d *forward, real_vector3d *up);
    static uint8_t check_impact_damage(uint32_t *self_object_index, uint32_t candidate_object_index);
    static void compute_mass_point_forces(object_physics_context *context, powered_mass_point_state *powered_states, uint32_t mass_points_address, real_vector3d *out_force, real_vector3d *out_torque);
    static uint8_t context_build(uint32_t object_index, object_physics_context *out_context);
    static void handle_nearby_object_impacts(uint32_t object_index);
    static void integrate_and_test_at_rest(object_physics_context *context, mass_point_state *mass_point_states, real_vector3d *torque, real_vector3d *force);
    static void mass_point_resolve_ground_contact(uint32_t exclude_object_index, mass_point_state *mass_point, PhysicsMassPoint *definition);
    static void mass_point_update_orientation(real_vector3d *axis, real_vector3d *up, real_vector3d *forward, real_vector3d *fallback_forward, real_vector3d *fallback_up);
    static uint8_t resolve_mass_point_overlap(object_physics_context *self, object_physics_context *other);
    static uint8_t test_point_against_mass_points(object_physics_context *context, real_point3d *world_point, int16_t *out_index);
    static uint8_t test_ray_against_mass_points(real_point3d *world_origin, real_vector3d *world_direction, object_physics_ray_result *out_result, object_physics_context *context);
    static void tick(uint32_t object_index, powered_mass_point_state *powered_states, uint32_t mass_points, real_vector3d *extra_force, real_vector3d *extra_torque);
    static void tick_single_pass(uint32_t object_index, powered_mass_point_state *powered_states, mass_point_state *mass_point_states, real_vector3d *extra_force, real_vector3d *extra_torque);
};

}
