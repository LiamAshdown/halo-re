#include "halo/game/game2_variants.hpp"
#include "interface.h"
#include "main.h"
#include "halo/main/api.hpp"

extern "C" {
extern game_variant game_engine_pending_variant;
extern game_variant game_engine_active_variant;
extern int32_t cached_network_engine_index;
extern network_server_globals *network_server;
extern network_client_globals *network_client;
extern uint16_t split_screen_quit_prompt_string;
extern uint8_t network_join_error_reason;
extern main_globals main_globals_data;
extern void network_game_broadcast_player_set_changed(void *session);
}

namespace halo::game {

/**
 * Copies the pending game variant into the active/local copy, and, while hosting a session whose cached engine
 * index disagrees, also pushes it into the network session's own variant field and notifies the network layer.
 * When neither a session nor a client is active, resets a small block of otherwise-unattributed globals to
 * their idle defaults.
 *
 * @address 0x45fc80
 */
void GameVariantRules::sync_variant_defaults(void)
{
    void *session;
    uint8_t hosting;

    halo::main::main_queue_map_change_by_name_or_clear((char *)"");

    session = network_server;
    hosting = (session != 0);

    game_engine_active_variant = game_engine_pending_variant;

    if (hosting && *(int32_t *)((uint8_t *)session + 0x13c) != cached_network_engine_index) {
        *(game_variant *)((uint8_t *)session + 0x10c) = game_engine_pending_variant;
        network_game_broadcast_player_set_changed(session);
        session = network_server;
    }

    if (network_client == 0 && session == 0) {
        split_screen_quit_prompt_string = 0xffff;
        network_join_error_reason = 0;
        main_globals_data.reset_map = 1;
        main_globals_data.lost_map = 0;
    }
}

}
