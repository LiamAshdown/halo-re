/**
 * @file src/scenario/scenario_c_api.cpp
 * The scenario module's C ABI: one extern "C" function per original symbol, same name and signature,
 * forwarding to the halo::scenario implementation. The shims do nothing else.
 */

#include "halo/scenario/scenario_c_api.h"

extern "C" void scenario_location_from_point(bsp_leaf_reference *out, real_point3d *point)
{
    halo::scenario::location_view(out).from_point(point);
}

extern "C" int16_t scenario_location_fog_region(bsp_leaf_reference *leaf, real_point3d *point)
{
    return halo::scenario::location_view(leaf).fog_region(point);
}

extern "C" float scenario_location_water_surface_distance(bsp_leaf_reference *leaf, real_point3d *point)
{
    return halo::scenario::location_view(leaf).water_surface_distance(point);
}

extern "C" uint8_t scenario_location_background_sound_is_deafening_to_ais(bsp_leaf_reference *location)
{
    return halo::scenario::location_view(location).background_sound_is_deafening_to_ais();
}

extern "C" uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf, int16_t *weather_index_out)
{
    return halo::scenario::scenario_query::location_get_water_and_weather(point, leaf, weather_index_out);
}

extern "C" uint32_t scenario_fog_region_resolve_tag(int16_t fog_region)
{
    return halo::scenario::scenario_query::fog_region_resolve_tag(fog_region);
}

extern "C" uint8_t scenario_cluster_visibility_test(int16_t row_cluster, int16_t column_cluster)
{
    return halo::scenario::scenario_query::cluster_visibility_test(row_cluster, column_cluster);
}

extern "C" uint8_t scenario_trigger_volume_contains_point(int16_t trigger_volume_index, real_point3d *point)
{
    return halo::scenario::scenario_query::trigger_volume_contains_point(trigger_volume_index, point);
}

extern "C" void scenario_sky_fog_state_update(int16_t sky_index, int16_t local_player_index, real_point3d *camera_position, render_fog *out)
{
    halo::scenario::scenario_query::sky_fog_state_update(sky_index, local_player_index, camera_position, out);
}

extern "C" void scenario_structure_bsp_activate_callbacks(void)
{
    halo::scenario::structure_bsp_switcher::activate_callbacks();
}

extern "C" void scenario_structure_bsp_deactivate_callbacks(void)
{
    halo::scenario::structure_bsp_switcher::deactivate_callbacks();
}

extern "C" uint8_t scenario_structure_bsp_switch(int16_t structure_bsp_index)
{
    return halo::scenario::structure_bsp_switcher::switch_to(structure_bsp_index);
}

extern "C" void scenario_structure_bsp_switch_after_load(void)
{
    halo::scenario::structure_bsp_switcher::switch_after_load();
}

extern "C" uint8_t scenario_structure_bsp_locate_point_nudge_up(real_point3d *point)
{
    return halo::scenario::structure_bsp_switcher::locate_point_nudge_up(point);
}

extern "C" uint8_t scenario_load(char *path)
{
    return halo::scenario::scenario_loader::load(path);
}

extern "C" int16_t scenario_object_name_find_index(Scenario *scenario, char *name)
{
    return halo::scenario::scenario_view(scenario).object_name_find_index(name);
}

extern "C" GlobalsMaterial * globals_material_get(int16_t material_index)
{
    return halo::scenario::scenario_loader::globals_material_get(material_index);
}
