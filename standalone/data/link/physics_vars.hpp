/**
 * @file standalone/data/link/physics_vars.hpp
 * Link names of the engine variables owned by the physics module (halo::physics::vars()). The data image defines them under these C
 * names; only src/physics/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char breakable_surface_state[];
extern char breakable_surfaces_enabled[];
extern char collideable_object_references[];
extern char global_collision_bsp[];
extern char global_structure_collision_bsp[];
extern char k_impact_damage_scale_table[];
extern char k_physics_collision_damping[];
extern char k_physics_displacement_directions[];
extern char k_physics_gravity[];
extern char material_table_bad_index[];
extern char material_table_fallback[];
extern char material_table_warning_issued[];
extern char object_cluster_stamp[];
extern char physics_disable_integration[];
extern char sphere_point_table[];
extern char sphere_point_table_count[];
}
