#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"

static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &game_engine_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_variant);
static auto &game_engine_state_value = halo::link::ref<game_engine_state>(halo::game::vars().game_engine_state_value);
static auto &game_engine_teams_enabled_flag = halo::link::ref<uint8_t>(halo::game::vars().game_engine_teams_enabled_flag);
static auto &game_engine_pending_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_pending_variant);
static auto &game_variant_history_current = halo::link::ref<int32_t>(halo::ui::vars().game_variant_history_current);
static auto &game_variant_saved_default = halo::link::ref<game_variant>(halo::ui::vars().game_variant_saved_default);
static auto &game_variant_saved_default_valid = halo::link::ref<uint8_t>(halo::game::vars().game_variant_saved_default_valid);
static auto &player_profile_cache_count = halo::link::ref<int32_t>(halo::game::vars().player_profile_cache_count);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &player_control_globals_ptr = halo::link::ref<player_control_globals *>(halo::game::vars().player_control_globals_ptr);
static auto &update_client_queues = halo::link::ref<data_array *>(halo::game::vars().update_client_queues);
static auto &update_server_queues = halo::link::ref<data_array *>(halo::game::vars().update_server_queues);
static auto &local_player_count = halo::link::ref<int16_t>(halo::game::vars().local_player_count);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &game_time_force_single_tick = halo::link::ref<int32_t>(halo::game::vars().game_time_force_single_tick);

namespace halo::game {

Globals &globals()
{
    static Globals instance{::current_game_engine, ::game_engine_variant, ::game_engine_state_value, ::game_engine_teams_enabled_flag, ::game_engine_pending_variant, ::game_variant_history_current, ::game_variant_saved_default, ::game_variant_saved_default_valid, ::player_profile_cache_count, ::player_data, ::local_player_globals, ::player_control_globals_ptr, ::update_client_queues, ::update_server_queues, ::local_player_count, ::game_time, ::game_time_force_single_tick};
    return instance;
}

}
