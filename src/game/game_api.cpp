#include "halo/game/api.hpp"

extern "C" {
extern game_engine_definition *current_game_engine;
extern game_variant game_engine_variant;
extern game_engine_state game_engine_state_value;
extern uint8_t game_engine_teams_enabled_flag;
extern game_variant game_engine_pending_variant;
extern int32_t game_variant_history_current;
extern game_variant game_variant_saved_default;
extern uint8_t game_variant_saved_default_valid;
extern int32_t player_profile_cache_count;
extern data_array *player_data;
extern player_globals *local_player_globals;
extern player_control_globals *player_control_globals_ptr;
extern data_array *update_client_queues;
extern data_array *update_server_queues;
extern int16_t local_player_count;
extern game_time_globals *game_time;
extern int32_t game_time_force_single_tick;
}

namespace halo::game {

Globals &globals()
{
    static Globals instance{::current_game_engine, ::game_engine_variant, ::game_engine_state_value, ::game_engine_teams_enabled_flag, ::game_engine_pending_variant, ::game_variant_history_current, ::game_variant_saved_default, ::game_variant_saved_default_valid, ::player_profile_cache_count, ::player_data, ::local_player_globals, ::player_control_globals_ptr, ::update_client_queues, ::update_server_queues, ::local_player_count, ::game_time, ::game_time_force_single_tick};
    return instance;
}

}
