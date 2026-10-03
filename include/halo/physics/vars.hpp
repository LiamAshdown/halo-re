/**
 * @file include/halo/physics/vars.hpp
 * Addresses of the engine variables the physics module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/physics_vars.hpp.
 */
#pragma once

namespace halo::physics {

/** Address table of the engine variables owned by the physics module. */
struct Vars {
    void *breakable_surface_state;
    void *breakable_surfaces_enabled;
    void *collideable_object_references;
    void *global_collision_bsp;
    void *global_structure_collision_bsp;
    void *k_impact_damage_scale_table;
    void *k_physics_collision_damping;
    void *k_physics_displacement_directions;
    void *k_physics_gravity;
    void *material_table_bad_index;
    void *material_table_fallback;
    void *material_table_warning_issued;
    void *object_cluster_stamp;
    void *physics_disable_integration;
    void *sphere_point_table;
    void *sphere_point_table_count;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::physics
