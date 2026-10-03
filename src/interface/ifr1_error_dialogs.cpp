#include "halo/interface/ifr1_error_dialogs.hpp"
#include "halo/interface/engine_state.hpp"
#include "halo/cutscene/api.hpp"

extern "C" {
extern ui_pending_error ui_pending_errors[4];
extern player_globals *local_player_globals;
extern uint8_t ui_split_screen;
extern uint8_t network_wait_flag_00719739;
extern widget_instance *ui_root_widget[1];
extern int16_t network_game_mode;
extern int16_t ui_pause_depth;
extern game_time_globals *game_time;
extern void chimera__load_main_menu(void);
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection);
}

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
    char *tag_path;
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

        if (local_player_globals->local_players[0] != (datum_index)-1) {
            active_player = 0;
        }
        if (active_player != -1) {
            do {
                if (active_player == slot && player_count > 0) {
                    matched_index = player_index;
                    half_screen = 0;
                }
                player_count = player_count + 1;
                active_player = (local_player_globals->local_players[0] != (datum_index)-1 && active_player < 0)
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
        tag_path = (modal == 0) ? (char *)"ui\\shell\\error\\error_nonmodal_fullscreen"
                                 : (char *)"ui\\shell\\error\\error_modal_fullscreen";
        break;
    case 2:
        tag_path = (modal == 0) ? (char *)"ui\\shell\\error\\error_nonmodal_halfscreen"
                                 : (char *)"ui\\shell\\error\\error_modal_halfscreen";
        break;
    case 3:
        if (!half_screen) {
            tag_path = (modal == 0) ? (char *)"ui\\shell\\error\\error_nonmodal_qtrscreen"
                                     : (char *)"ui\\shell\\error\\error_modal_qtrscreen";
        } else if (modal == 0) {
            tag_path = (char *)"ui\\shell\\error\\error_nonmodal_halfscreen";
        } else {
            tag_path = (char *)"ui\\shell\\error\\error_modal_halfscreen";
        }
        break;
    case 4:
        tag_path = (modal == 0) ? (char *)"ui\\shell\\error\\error_nonmodal_qtrscreen"
                                 : (char *)"ui\\shell\\error\\error_modal_qtrscreen";
        break;
    default:
        return;
    }

    if (ui_split_screen != 0 && (state::screen_fade_progress < 1.0f) != (state::screen_fade_progress == 1.0f) &&
        0.0f <= state::screen_fade_progress) {
        chimera__load_main_menu();
        network_wait_flag_00719739 = 0;
        state::screen_fade_progress = -1.0f;
    }

    slot = (int16_t)(((uint16_t)player_index == 0xffff) ? 0 : (uint16_t)player_index);
    root = ui_root_widget[slot];
    if (root == (widget_instance *)0) {
        history_source = (datum_index)-1;
    } else {
        history_source = root->definition;
        if (root->is_error_dialog == 1) {
            return;
        }
    }

    dialog = chimera__load_ui_widget(tag_path, (datum_index)-1, (widget_instance *)0,
                                     (uint16_t)player_index, history_source, (datum_index)-1, -1);
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
            dialog->pauses_game_time = is_error;
            if (is_error == 1 && network_game_mode != 2) {
                ui_pause_depth = ui_pause_depth + 1;
                if (game_time->paused == 0) {
                    if (game_time->initialized != 0) {
                        game_time->active = 0;
                    }
                    game_time->paused = 1;
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
