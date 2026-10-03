/**
 * @file include/halo/projectiles/vars.hpp
 * Addresses of the engine variables the projectiles module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/projectiles_vars.hpp.
 */
#pragma once

namespace halo::projectiles {

/** Address table of the engine variables owned by the projectiles module. */
struct Vars {
    void *k_projectile_minimum_age_ticks;
    void *projectile_default_material_response;
    void *projectile_effect_coordinate_system_names;
    void *projectile_network_update_position_tolerance;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::projectiles
