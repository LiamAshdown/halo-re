/**
 * @file src/items/vars.cpp
 * Binds halo::items::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/items/vars.hpp"
#include "link/items_vars.hpp"

namespace halo::items {

const Vars &vars()
{
    static const Vars table{
        equipment_network_update_position_tolerance,
        k_equipment_minimum_age_ticks,
        k_weapon_minimum_age_ticks,
        k_weapon_zoom_fov_maximum,
        k_weapon_zoom_fov_minimum,
        s_ground_point_marker,
        s_primary_trigger_marker,
        s_secondary_trigger_marker,
        weapon_blur_permutation_names,
        weapon_bottomless_clip,
        weapon_client_side_projectiles,
        weapon_infinite_ammo,
        weapon_network_update_position_tolerance,
    };
    return table;
}

}  // namespace halo::items
