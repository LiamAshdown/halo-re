/**
 * Assorted UI screen-level behaviour: cursor, error modal, pause check, colours and option application.
 */

#include "tags.h"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/interface/engine_state.hpp"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "units.h"
#include "cutscene.h"

#include "halo/interface/uis_screens.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/ai/api.hpp"
#include "halo/main/api.hpp"

static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &ui_split_screen = halo::link::ref<uint8_t>(halo::ui::vars().ui_split_screen);
static auto &ui_pause_pending_count_00718fa0 = halo::link::ref<int32_t>(halo::main::vars().ui_pause_pending_count_00718fa0);
static auto &chat_dialog_open = halo::link::ref<uint8_t>(halo::ui::vars().chat_dialog_open);
static auto &ui_root_widget = halo::link::ref<widget_instance *[1]>(halo::ui::vars().ui_root_widget);
static auto &widget_memory_pool_valid = halo::link::ref<uint8_t>(halo::ui::vars().widget_memory_pool_valid);

namespace halo::ui {

/**
 * Chooses and loads the appropriately-sized pause-menu widget (or, off-line, the solo/split-screen one) for the
 * current session, gated on a long list of "not a safe time to pause" checks, and always decrements the pending
 * pause-request counter by one (floored at zero). Returns 1 (in the low byte) if a widget was loaded, 0
 * otherwise.
 *
 * @address 0x49c1a0
 */
uint32_t UiScreens::check_for_pause_game(void)
{
    uint8_t handled = 0;
    uint8_t networked = (halo::networking::globals().client != (network_client_globals *)0) ||
                         (halo::networking::globals().server != (network_server_globals *)0);
    int16_t active_player;
    int16_t player_count = 0;
    uint8_t single_player_at_start = 1;
    int16_t co_op_flag = -1;
    const char *tag_path;

    auto request_pause = [&]() -> uint8_t {
    if (halo::game::globals().game_time->initialized == 0 ||
        (halo::game::globals().game_time->active == 0 && halo::game::globals().game_time->paused == 0) ||
        halo::cutscene::globals().cinematic_globals->in_progress != 0 ||
        halo::networking::globals().game_mode == 3 || ui_split_screen != 0 || ui_pause_pending_count_00718fa0 != 0 ||
        (halo::game::globals().player_control->action_flags_latched >> 3 & 1) != 0 || chat_dialog_open != 0 ||
        halo::interface::state::escape_key_state != 1) {
        return 0;
    }

    active_player = (halo::game::globals().local_player_globals->local_players[0] != k_datum_index_none) ? 0 : -1;
    while (active_player != -1) {
        if (active_player == 0 && player_count > 0) {
            single_player_at_start = 0;
        }
        co_op_flag = 0;
        player_count = player_count + 1;
        active_player = (halo::game::globals().local_player_globals->local_players[0] != k_datum_index_none && active_player < 0)
                             ? 0 : -1;
    }

    if (networked) {
        if (halo::game::globals().state != 0 || co_op_flag != 0 || ui_root_widget[0] != (widget_instance *)0) {
            return 0;
        }
        switch (player_count) {
        case 1:
            tag_path = halo::tag_paths::_1p_pause_game;
            break;
        case 2:
            tag_path = halo::tag_paths::_2p_pause_game;
            break;
        case 3:
            if (!single_player_at_start) {
                tag_path = halo::tag_paths::_4p_pause_game;
            } else {
                tag_path = halo::tag_paths::_2p_pause_game;
            }
            break;
        case 4:
            tag_path = halo::tag_paths::_4p_pause_game;
            break;
        default:
            return 0;
        }
    } else {
        if (player_count < 0) {
            if (widget_memory_pool_valid != 0 && ui_root_widget[0] != (widget_instance *)0) {
                return 0;
            }
            tag_path = halo::tag_paths::solo_pause_game_widget;
        } else if (player_count > 1) {
            if (player_count == 2) {
                if (ui_root_widget[0] != (widget_instance *)0 ||
                    halo::game::globals().game_time->paused != 0) {
                    return 0;
                }
                tag_path = halo::tag_paths::pause_game_split_screen;
            } else {
                if (widget_memory_pool_valid != 0 && ui_root_widget[0] != (widget_instance *)0) {
                    return 0;
                }
                tag_path = halo::tag_paths::solo_pause_game_widget;
            }
        } else {
            if (ui_root_widget[0] != (widget_instance *)0) {
                return 0;
            }
            tag_path = halo::tag_paths::solo_pause_game_widget;
        }
    }

    halo::interface::chimera__load_ui_widget(tag_path, k_datum_index_none, (widget_instance *)0, 0,
                             k_datum_index_none, k_datum_index_none, -1);
    return 1;
    };

    handled = request_pause();
    ui_pause_pending_count_00718fa0 =
        (ui_pause_pending_count_00718fa0 - 1 < 0) ? 0 : ui_pause_pending_count_00718fa0 - 1;
    return handled;
}

}
