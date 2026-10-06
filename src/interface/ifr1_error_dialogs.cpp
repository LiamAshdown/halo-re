#include "halo/game/lockstep.hpp"
#include "halo/interface/ifr1_error_dialogs.hpp"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/core/datum.hpp"
#include "halo/interface/engine_state.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

static auto &ui_pending_errors = halo::link::ref<ui_pending_error [4]>(halo::ui::vars().ui_pending_errors);
static auto &ui_split_screen = halo::link::ref<uint8_t>(halo::ui::vars().ui_split_screen);
static auto &network_wait_flag_00719739 = halo::link::ref<uint8_t>(halo::ui::vars().network_wait_flag_00719739);
static auto &ui_root_widget = halo::link::ref<widget_instance *[1]>(halo::ui::vars().ui_root_widget);
static auto &ui_pause_depth = halo::link::ref<int16_t>(halo::ui::vars().ui_pause_depth);

namespace halo::interface {

/**
 * Queues or immediately opens the appropriately-sized modal/non-modal error dialog widget for the given error
 * message id, tag'd by how many local players are active and whether it should block input.
 *
 * @address 0x498f20
 */
void ErrorDialogs::show(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error)
{
    int16_t slot = (int16_t)player_index;
    int16_t active_player;
    int16_t player_count = 0;
    uint8_t half_screen = 1;
    const char *tag_path;
    widget_instance *root;
    datum_index history_source;
    widget_instance *dialog;

    if (halo::cutscene::globals().cinematic_globals->in_progress != 0) {
        int32_t index = (slot == -1) ? 0 : slot;

        if (ui_pending_errors[index].error_string_index != -1) {
            return;
        }
        ui_pending_errors[index].error_string_index = error_string_index;
        ui_pending_errors[index].modal = modal;
        ui_pending_errors[index].is_error = is_error;
        return;
    }

    active_player = -1;
    if (slot == -1) {
        if (ui_split_screen == 0) {
            player_index = -1;
        }
    } else {
        int32_t matched_index = -1;

        if (halo::game::globals().local_player_globals->local_players[0] != k_datum_index_none) {
            active_player = 0;
        }
        if (active_player != -1) {
            do {
                if (active_player == slot && player_count > 0) {
                    matched_index = player_index;
                    half_screen = 0;
                }
                player_count = player_count + 1;
                active_player = (halo::game::globals().local_player_globals->local_players[0] != k_datum_index_none && active_player < 0)
                                    ? 0
                                    : -1;
            } while (active_player != -1);
            if ((int16_t)matched_index == -1) {
                if (ui_split_screen == 0) {
                    player_index = -1;
                }
            }
        } else {
            if (ui_split_screen == 0) {
                player_index = -1;
            }
        }
    }

    switch (player_count) {
    case 0:
    case 1:
        tag_path = (modal == 0) ? halo::tag_paths::error_nonmodal_fullscreen
                                 : halo::tag_paths::error_modal_fullscreen;
        break;
    case 2:
        tag_path = (modal == 0) ? halo::tag_paths::error_nonmodal_halfscreen
                                 : halo::tag_paths::error_modal_halfscreen;
        break;
    case 3:
        if (!half_screen) {
            tag_path = (modal == 0) ? halo::tag_paths::error_nonmodal_qtrscreen
                                     : halo::tag_paths::error_modal_qtrscreen;
        } else if (modal == 0) {
            tag_path = halo::tag_paths::error_nonmodal_halfscreen;
        } else {
            tag_path = halo::tag_paths::error_modal_halfscreen;
        }
        break;
    case 4:
        tag_path = (modal == 0) ? halo::tag_paths::error_nonmodal_qtrscreen
                                 : halo::tag_paths::error_modal_qtrscreen;
        break;
    default:
        return;
    }

    if (ui_split_screen != 0 && (state::screen_fade_progress < 1.0f) != (state::screen_fade_progress == 1.0f) &&
        0.0f <= state::screen_fade_progress) {
        halo::interface::chimera__load_main_menu();
        network_wait_flag_00719739 = 0;
        state::screen_fade_progress = -1.0f;
    }

    slot = (int16_t)(((uint16_t)player_index == halo::k_word_none) ? 0 : (uint16_t)player_index);
    root = ui_root_widget[slot];
    if (root == (widget_instance *)0) {
        history_source = k_datum_index_none;
    } else {
        history_source = root->definition;
        if (root->is_error_dialog == 1) {
            return;
        }
    }

    dialog = halo::interface::chimera__load_ui_widget(tag_path, k_datum_index_none, (widget_instance *)0,
                                     (uint16_t)player_index, history_source, k_datum_index_none, -1);
    if (dialog != (widget_instance *)0) {
        int16_t clamped;

        if (error_string_index < 0) {
            clamped = 0;
        } else {
            clamped = 0x3b;
            if (error_string_index < 0x3c) {
                clamped = error_string_index;
            }
        }
        dialog->first_child->first_child->selection_index = clamped;
        dialog->is_error_dialog = 1;
        if (dialog->pauses_game_time == 0) {
            dialog->pauses_game_time = halo::game::lockstep::session_active() ? 0 : is_error;  // co-op does not pause
            if (dialog->pauses_game_time == 1 && halo::networking::globals().game_mode != 2) {
                ui_pause_depth = ui_pause_depth + 1;
                if (halo::game::globals().game_time->paused == 0) {
                    if (halo::game::globals().game_time->initialized != 0) {
                        halo::game::globals().game_time->active = 0;
                    }
                    halo::game::globals().game_time->paused = 1;
                }
            }
        }
        if (error_string_index != 0xc) {
            if (error_string_index != 0xd) {
                dialog->close_on_controller_connected[0] = 0;
                return;
            }
            dialog->close_on_controller_connected[0] = 1;
        }
        dialog->milliseconds_to_auto_close = 0;
        dialog->milliseconds_auto_close_fade = 0;
    }
}

}
