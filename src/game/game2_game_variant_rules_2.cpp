#include "halo/game/game2_variants.hpp"
#include "interface.h"
#include "main.h"
#include "halo/main/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"

static auto &game_engine_pending_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_pending_variant);
static auto &game_engine_active_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_active_variant);
static auto &cached_network_engine_index = halo::link::ref<int32_t>(halo::game::vars().cached_network_engine_index);
static auto &split_screen_quit_prompt_string = halo::link::ref<uint16_t>(halo::ui::vars().split_screen_quit_prompt_string);

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

    session = halo::networking::globals().server;
    hosting = (session != 0);

    game_engine_active_variant = game_engine_pending_variant;

    if (hosting && *(int32_t *)((uint8_t *)session + 0x13c) != cached_network_engine_index) {
        *(game_variant *)((uint8_t *)session + 0x10c) = game_engine_pending_variant;
        halo::networking::network_game_broadcast_player_set_changed((uint8_t *)session);
        session = halo::networking::globals().server;
    }

    if (halo::networking::globals().client == 0 && session == 0) {
        split_screen_quit_prompt_string = 0xffff;
        halo::networking::globals().join_error_reason = 0;
        halo::main::globals().main_globals.reset_map = 1;
        halo::main::globals().main_globals.lost_map = 0;
    }
}

}
