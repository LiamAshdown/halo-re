/**
 * @file standalone/data/link/items_vars.hpp
 * Link names of the engine variables owned by the items module (halo::items::vars()). The data image defines them under these C
 * names; only src/items/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char equipment_network_update_position_tolerance[];
extern char k_equipment_minimum_age_ticks[];
extern char k_weapon_minimum_age_ticks[];
extern char k_weapon_zoom_fov_maximum[];
extern char k_weapon_zoom_fov_minimum[];
extern char s_ground_point_marker[];
extern char s_primary_trigger_marker[];
extern char s_secondary_trigger_marker[];
extern char weapon_blur_permutation_names[];
extern char weapon_bottomless_clip[];
extern char weapon_client_side_projectiles[];
extern char weapon_infinite_ammo[];
extern char weapon_network_update_position_tolerance[];
}
