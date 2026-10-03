/**
 * @file include/halo/items/vars.hpp
 * Addresses of the engine variables the items module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/items_vars.hpp.
 */
#pragma once

namespace halo::items {

/** Address table of the engine variables owned by the items module. */
struct Vars {
    void *equipment_network_update_position_tolerance;
    void *k_equipment_minimum_age_ticks;
    void *k_weapon_minimum_age_ticks;
    void *k_weapon_zoom_fov_maximum;
    void *k_weapon_zoom_fov_minimum;
    void *s_ground_point_marker;
    void *s_primary_trigger_marker;
    void *s_secondary_trigger_marker;
    void *weapon_blur_permutation_names;
    void *weapon_bottomless_clip;
    void *weapon_client_side_projectiles;
    void *weapon_infinite_ammo;
    void *weapon_network_update_position_tolerance;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::items
