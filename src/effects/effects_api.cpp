#include "halo/effects/effects.hpp"
#include "halo/effects/api.hpp"

extern "C" {
extern random_seed effect_random_seed;
extern data_array *contrail_data;
extern data_array *contrail_point_data;
extern data_array *decal_data;
extern data_array *effect_data;
extern data_array *effect_location_data;
extern data_array *particle_data;
extern data_array *particle_system_particle_data;
extern data_array *weather_particle_data;
extern player_effect_globals *player_effect_globals_pointer;
extern uint8_t decals_for_all_responses;
extern int32_t player_effect_reentry_count;
extern uint8_t particle_spawn_debug_mode;
}

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
