/**
 * @file src/physics/vars.cpp
 * Binds halo::physics::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/physics/vars.hpp"
#include "link/physics_vars.hpp"
#include "halo/physics/api.hpp"

namespace halo::physics {

const Vars &vars()
{
    static const Vars table{
        breakable_surface_state,
        breakable_surfaces_enabled,
        collideable_object_references,
        global_collision_bsp,
        global_structure_collision_bsp,
        k_impact_damage_scale_table,
        k_physics_collision_damping,
        k_physics_displacement_directions,
        ::k_physics_gravity,
        material_table_bad_index,
        material_table_fallback,
        material_table_warning_issued,
        object_cluster_stamp,
        physics_disable_integration,
        sphere_point_table,
        sphere_point_table_count,
    };
    return table;
}

}  // namespace halo::physics
