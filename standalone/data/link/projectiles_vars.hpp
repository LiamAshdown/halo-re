/**
 * @file standalone/data/link/projectiles_vars.hpp
 * Link names of the engine variables owned by the projectiles module (halo::projectiles::vars()). The data image defines them under these C
 * names; only src/projectiles/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char k_projectile_minimum_age_ticks[];
extern char projectile_default_material_response[];
extern char projectile_effect_coordinate_system_names[];
extern char projectile_network_update_position_tolerance[];
}
