/**
 * @file src/projectiles/vars.cpp
 * Binds halo::projectiles::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/projectiles/vars.hpp"
#include "link/projectiles_vars.hpp"

namespace halo::projectiles {

const Vars &vars()
{
    static const Vars table{
        k_projectile_minimum_age_ticks,
        projectile_default_material_response,
        projectile_effect_coordinate_system_names,
        projectile_network_update_position_tolerance,
    };
    return table;
}

}  // namespace halo::projectiles
