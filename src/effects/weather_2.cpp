#include "halo/effects/effects.hpp"
#include "halo/structures/api.hpp"
#include "halo/effects/api.hpp"

extern "C" {
extern uint8_t weather_enabled;
extern int16_t current_local_player_index;
extern ScenarioStructureBSP *global_structure_bsp;
extern weather_instance weather_instances[1];
extern real_point3d render_camera_global;
extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf, int16_t *weather_index_out);
}

namespace halo::effects {

/**
 * Per-tick weather driver for the local player: re-probes the render sample point's BSP cluster,
 * looks up that cluster's weather row, and activates/deactivates the local player's weather
 * instance when the row changes, then rebuilds its render geometry while active.
 *
 * @address 0x458a90
 */
void weather_system::update_local_player()
{
    if (weather_enabled != 0 && current_local_player_index != -1) {
        int16_t instance_index = current_local_player_index;
        weather_instance *instance = &weather_instances[instance_index];
        int16_t cluster_index;
        int32_t new_definition_index = -1;

        instance->render_cluster_index = halo::structures::globals().render_cluster_index;
        instance->render_leaf_index = halo::structures::globals().render_leaf_index;
        instance->in_sky = scenario_location_get_water_and_weather(&render_camera_global,
            (bsp_leaf_reference *)&instance->render_leaf_index, &instance->cluster_index);
        cluster_index = instance->cluster_index;

        if (cluster_index != -1) {
            new_definition_index = *(int32_t *)((uint8_t *)global_structure_bsp->weather_palette.pointer +
                (uint32_t)cluster_index * 0xf0 + 0x2c);
        }

        if ((int32_t)instance->definition_index != new_definition_index) {
            if (instance->definition_index != (datum_index)0xffffffff) {
                halo::effects::weather_instance_deactivate(instance_index);
            }
            if (new_definition_index != -1) {
                halo::effects::weather_instance_activate((datum_index)new_definition_index, instance_index, 1.0f);
            }
        }

        if (instance->definition_index != (datum_index)0xffffffff) {
            halo::effects::weather_instance_build_render_geometry(instance_index);
        }
    }
}

}

namespace halo::effects {

void weather_update_local_player()
{
    halo::effects::weather_system::update_local_player();
}

}
