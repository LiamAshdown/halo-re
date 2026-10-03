#include "halo/game/game2_engine_match.hpp"
#include "halo/game/legacy_globals.hpp"
#include "halo/input/api.hpp"

extern "C" {
extern game_engine_definition *current_game_engine;
extern game_engine_state game_engine_state_value;
extern float game_engine_end_game_timer;
extern float game_engine_post_game_fade;
extern uint8_t game_engine_dedicated_idle;
extern float game_engine_dedicated_idle_timer;
extern int16_t network_game_mode;
extern uint8_t *network_server;
extern uint8_t network_host_handoff_requested;
extern local_player_input_state local_player_input_states[k_maximum_local_players];
extern uint8_t chimera_loading_screen_cleanup_gate;
extern void game_engine_end_game_sequence_stage3(void);
extern void game_engine_send_end_game_notification(uint32_t reason);
extern void chimera__console_out(ColorARGB *color, char *format, ...);
extern void chat_close(void);
extern void network_game_client_game_settings_updated(void *session);
}

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
        if (network_game_mode != 2) {
            return;
        }
        game_engine_end_game_sequence_stage3();
        game_engine_send_end_game_notification(3);
        return;
    }

    if (game_engine_state_value != _game_engine_state_post_game) {
        return;
    }

    game_engine_post_game_fade = game_engine_post_game_fade + delta_time;
    if (1.0f < game_engine_post_game_fade) {
        game_engine_post_game_fade = 1.0f;
    }

    if (network_game_mode == 2) {
        uint8_t idle_timer_expired = 0;

        if (game_engine_dedicated_idle == 0) {
            if ((*((uint8_t *)network_server + 6) >> 2 & 1) != 0) {
                chimera__console_out((ColorARGB *)0, (char *)"Game Complete. Dedicated server is now idle.");
                network_host_handoff_requested = 1;
                chat_close();
            }
        } else {
            game_engine_dedicated_idle_timer = game_engine_dedicated_idle_timer - delta_time;
            if (game_engine_dedicated_idle_timer <= 0.0f) {
                idle_timer_expired = 1;
                game_engine_dedicated_idle = 0;
            }
        }

        if (local_player_input_states[0].buttons[halo::game::globals::k_input_action_accept] != 0 || halo::input::input_get_key_state(0x66) == 1 || idle_timer_expired) {
            network_game_client_game_settings_updated(network_server);
        }
    }

    if (chimera_loading_screen_cleanup_gate != 0) {
        network_host_handoff_requested = 1;
        chat_close();
    }
}

}
