#include "halo/game/game2_engine_match.hpp"
#include "halo/game/legacy_globals.hpp"
#include "halo/input/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/networking/vars.hpp"

static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &game_engine_state_value = halo::link::ref<game_engine_state>(halo::game::vars().game_engine_state_value);
static auto &game_engine_end_game_timer = halo::link::ref<float>(halo::game::vars().game_engine_end_game_timer);
static auto &game_engine_post_game_fade = halo::link::ref<float>(halo::game::vars().game_engine_post_game_fade);
static auto &game_engine_dedicated_idle = halo::link::ref<uint8_t>(halo::game::vars().game_engine_dedicated_idle);
static auto &game_engine_dedicated_idle_timer = halo::link::ref<float>(halo::game::vars().game_engine_dedicated_idle_timer);
static auto &network_server = halo::link::ref<uint8_t *>(halo::networking::vars().network_server);
static auto &local_player_input_states = halo::link::ref<local_player_input_state [k_maximum_local_players]>(halo::game::vars().local_player_input_states);
static auto &chimera_loading_screen_cleanup_gate = halo::link::ref<uint8_t>(halo::game::vars().chimera_loading_screen_cleanup_gate);

namespace halo::game {

/**
 * Advances the two end-of-game countdown stages (game_engine_state _ending then _ended) and the post-game
 * fade, driving the dedicated-server idle message/chat shutdown and re-notifying connected clients of settings
 * changes once the fade or idle timer finishes.
 *
 * @address 0x45fdf0
 */
void EngineMatch::update_end_game_sequence(float delta_time)
{
    if (current_game_engine == 0) {
        return;
    }

    if (game_engine_state_value == _game_engine_state_ended) {
        game_engine_end_game_timer = game_engine_end_game_timer - delta_time;
        if (game_engine_end_game_timer > 0.0f) {
            return;
        }
        if (halo::networking::globals().game_mode != 2) {
            return;
        }
        halo::game::game_engine_end_game_sequence_stage3();
        halo::game::game_engine_send_end_game_notification(3);
        return;
    }

    if (game_engine_state_value != _game_engine_state_post_game) {
        return;
    }

    game_engine_post_game_fade = game_engine_post_game_fade + delta_time;
    if (1.0f < game_engine_post_game_fade) {
        game_engine_post_game_fade = 1.0f;
    }

    if (halo::networking::globals().game_mode == 2) {
        uint8_t idle_timer_expired = 0;

        if (game_engine_dedicated_idle == 0) {
            if ((*((uint8_t *)network_server + 6) >> 2 & 1) != 0) {
                halo::interface::chimera__console_out((ColorARGB *)0, (char *)"Game Complete. Dedicated server is now idle.");
                halo::networking::globals().host_handoff_requested = 1;
                halo::interface::chat_close();
            }
        } else {
            game_engine_dedicated_idle_timer = game_engine_dedicated_idle_timer - delta_time;
            if (game_engine_dedicated_idle_timer <= 0.0f) {
                idle_timer_expired = 1;
                game_engine_dedicated_idle = 0;
            }
        }

        if (local_player_input_states[0].buttons[halo::game::fields::k_input_action_accept] != 0 || halo::input::input_get_key_state(0x66) == 1 || idle_timer_expired) {
            halo::networking::network_game_client_game_settings_updated((network_server_globals *)network_server);
        }
    }

    if (chimera_loading_screen_cleanup_gate != 0) {
        halo::networking::globals().host_handoff_requested = 1;
        halo::interface::chat_close();
    }
}

}
