/**
 * @file include/halo/scenario/api.hpp
 * Functions of the scenario module that other modules and the data tables call (namespace halo::scenario). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdarg.h>
#include <stdint.h>

struct GlobalsMaterial;
struct Scenario;
struct bsp_leaf_reference;
struct real_point3d;
struct render_fog;

namespace halo::scenario {

void scenario_location_from_point(bsp_leaf_reference *out, real_point3d *point);
int16_t scenario_location_fog_region(bsp_leaf_reference *leaf, real_point3d *point);
float scenario_location_water_surface_distance(bsp_leaf_reference *leaf, real_point3d *point);
uint8_t scenario_location_background_sound_is_deafening_to_ais(bsp_leaf_reference *location);
uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf, int16_t *weather_index_out);
uint32_t scenario_fog_region_resolve_tag(int16_t fog_region);
uint8_t scenario_cluster_visibility_test(int16_t row_cluster, int16_t column_cluster);
uint8_t scenario_trigger_volume_contains_point(int16_t trigger_volume_index, real_point3d *point);
void scenario_sky_fog_state_update(int16_t sky_index, int16_t local_player_index, real_point3d *camera_position, render_fog *out);
void scenario_structure_bsp_activate_callbacks(void);
void scenario_structure_bsp_deactivate_callbacks(void);
uint8_t scenario_structure_bsp_switch(int16_t structure_bsp_index);
void scenario_structure_bsp_switch_after_load(void);
uint8_t scenario_structure_bsp_locate_point_nudge_up(real_point3d *point);
uint8_t scenario_load(char *path);
int16_t scenario_object_name_find_index(Scenario *scenario, char *name);
GlobalsMaterial * globals_material_get(int16_t material_index);

}
