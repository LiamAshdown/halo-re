/**
 * @file src/scenario/scenario_api.cpp
 * The scenario module's free-function API (include/halo/scenario/api.hpp), forwarding to the class implementations.
 */

#include <stdarg.h>
#include "halo/scenario/scenario.hpp"
#include "halo/scenario/api.hpp"


namespace halo::scenario {


uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf, int16_t *weather_index_out)
{
    return halo::scenario::scenario_query::location_get_water_and_weather(point, leaf, weather_index_out);
}

void scenario_structure_bsp_switch_after_load(void)
{
    halo::scenario::structure_bsp_switcher::switch_after_load();
}

uint8_t scenario_load(char *path)
{
    return halo::scenario::scenario_loader::load(path);
}

int16_t scenario_object_name_find_index(Scenario *scenario, char *name)
{
    return halo::scenario::scenario_view(scenario).object_name_find_index(name);
}

GlobalsMaterial * globals_material_get(int16_t material_index)
{
    return scenario_loader::globals_material_get(material_index);
}

}
