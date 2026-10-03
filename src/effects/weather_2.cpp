#include "halo/effects/effects.hpp"
#include "halo/scenario/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/render/vars.hpp"

static auto &weather_enabled = halo::link::ref<uint8_t>(halo::effects::vars().weather_enabled);
static auto &weather_instances = halo::link::ref<weather_instance [1]>(halo::effects::vars().weather_instances);
static auto &render_camera_global = halo::link::ref<real_point3d>(halo::render::vars().render_camera_global);

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
    if (weather_enabled != 0 && halo::interface::globals().current_local_player_index != -1) {
        int16_t instance_index = halo::interface::globals().current_local_player_index;
        weather_instance *instance = &weather_instances[instance_index];
        int16_t cluster_index;
        int32_t new_definition_index = -1;

        instance->render_cluster_index = halo::structures::globals().render_cluster_index;
        instance->render_leaf_index = halo::structures::globals().render_leaf_index;
        instance->in_sky = halo::scenario::scenario_location_get_water_and_weather(&render_camera_global,
            (bsp_leaf_reference *)&instance->render_leaf_index, &instance->cluster_index);
        cluster_index = instance->cluster_index;

        if (cluster_index != -1) {
            new_definition_index = *(int32_t *)((uint8_t *)halo::scenario::globals().structure_bsp->weather_palette.pointer +
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
