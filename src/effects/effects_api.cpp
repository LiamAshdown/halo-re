#include "halo/effects/effects.hpp"
#include "halo/effects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"

static auto &effect_random_seed = halo::link::ref<random_seed>(halo::effects::vars().effect_random_seed);
static auto &contrail_data = halo::link::ref<data_array *>(halo::effects::vars().contrail_data);
static auto &contrail_point_data = halo::link::ref<data_array *>(halo::effects::vars().contrail_point_data);
static auto &decal_data = halo::link::ref<data_array *>(halo::effects::vars().decal_data);
static auto &effect_data = halo::link::ref<data_array *>(halo::effects::vars().effect_data);
static auto &effect_location_data = halo::link::ref<data_array *>(halo::effects::vars().effect_location_data);
static auto &particle_data = halo::link::ref<data_array *>(halo::effects::vars().particle_data);
static auto &particle_system_particle_data = halo::link::ref<data_array *>(halo::effects::vars().particle_system_particle_data);
static auto &weather_particle_data = halo::link::ref<data_array *>(halo::game::vars().weather_particle_data);
static auto &player_effect_globals_pointer = halo::link::ref<player_effect_globals *>(halo::effects::vars().player_effect_globals_pointer);
static auto &decals_for_all_responses = halo::link::ref<uint8_t>(halo::effects::vars().decals_for_all_responses);
static auto &player_effect_reentry_count = halo::link::ref<int32_t>(halo::effects::vars().player_effect_reentry_count);
static auto &particle_spawn_debug_mode = halo::link::ref<uint8_t>(halo::effects::vars().particle_spawn_debug_mode);

namespace halo::effects {

/**
 * The effects module's engine globals as one service object.
 */
Globals &globals()
{
    static Globals instance{::effect_random_seed,
                            ::contrail_data,
                            ::contrail_point_data,
                            ::decal_data,
                            ::effect_data,
                            ::effect_location_data,
                            ::particle_data,
                            ::particle_system_particle_data,
                            ::weather_particle_data,
                            ::player_effect_globals_pointer,
                            ::decals_for_all_responses,
                            ::player_effect_reentry_count,
                            ::particle_spawn_debug_mode};
    return instance;
}

}
