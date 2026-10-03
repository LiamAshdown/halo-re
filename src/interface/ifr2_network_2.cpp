#include "halo/interface/ifr2_network.hpp"
#include "halo/core/datum.hpp"
#include "halo/main/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern game_variant game_variant_saved_default;
extern uint8_t game_variant_saved_default_valid;
extern int32_t game_variant_history_current;
extern uint16_t split_screen_quit_prompt_string;
extern uint8_t split_screen_quit_prompt_armed;
extern void widget_close_all(void);
extern uint8_t game_engine_get_variant_by_name(const char *name, game_variant *out);
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index, widget_instance *parent, uint16_t controller_index, datum_index history_definition, datum_index history_list_definition, int16_t history_selection);
extern void game_engine_ensure_variant_history_has_entry(void);
extern void game_engine_apply_current_custom_variant(void);
extern void game_engine_sync_variant_defaults(void);
}

namespace halo::interface {

/**
 * Starts hosting a multiplayer game: closes every open UI widget, disposes any previous host session, resets
 * the network mode, clears the map-change queue, fetches the default game variant into
 * game_variant_saved_default, and opens the "connected pregame" screen. If that widget opens successfully,
 * creates the host and network session;
 *
 * @address 0x495ac0
 */
void NetworkSetup::game_host_start(char *map_name, char *variant_name, uint8_t disconnect_timeout_flag)
{
    widget_instance *widget;
    game_variant variant;

    halo::interface::widget_close_all();
    if (halo::networking::globals().server != (void *)0) {
        halo::networking::network_game_server_host_dispose(halo::networking::globals().server);
        halo::networking::globals().server = (network_server_globals *)((void *)0);
        halo::networking::globals().server_host_valid = 0;
    }
    halo::networking::network_client_globals_dispose();
    halo::networking::globals().game_mode = 0;
    halo::main::main_queue_map_change_by_name_or_clear(map_name);

    halo::game::game_engine_get_variant_by_name(variant_name, &variant);
    halo::game::globals().variant_saved_default = variant;
    halo::game::globals().variant_saved_default_valid = 1;

    widget = halo::interface::chimera__load_ui_widget(
        (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen",
        (datum_index)halo::k_dword_none, (widget_instance *)0, halo::k_word_none, (datum_index)halo::k_dword_none,
        (datum_index)halo::k_dword_none, -1);
    if (widget != (widget_instance *)0) {
        halo::game::game_engine_ensure_variant_history_has_entry();
        halo::networking::globals().disconnect_timeout_flag = disconnect_timeout_flag;
        if (halo::networking::network_game_server_host_create() != 0) {
            halo::networking::globals().client = (network_client_globals *)(halo::networking::network_session_create());
            if (halo::networking::globals().client != (void *)0) {
                halo::networking::globals().host_handoff_requested = 0;
                game_variant_history_current = -1;
                halo::game::game_engine_apply_current_custom_variant();
                halo::game::game_engine_sync_variant_defaults();
                halo::networking::globals().game_mode = 2;
                return;
            }
        }
        if (halo::networking::globals().server != (void *)0) {
            halo::networking::network_game_server_host_dispose(halo::networking::globals().server);
            halo::networking::globals().server = (network_server_globals *)((void *)0);
            halo::networking::globals().server_host_valid = 0;
        }
        halo::networking::network_client_globals_dispose();
        halo::networking::globals().disconnect_timeout_flag = 0;
    }
    split_screen_quit_prompt_string = halo::k_word_none;
    halo::networking::globals().join_error_reason = 0;
    split_screen_quit_prompt_armed = 1;
}

} // namespace halo::interface

namespace halo::interface {

void network_game_host_start(char *map_name, char *variant_name, uint8_t disconnect_timeout_flag)
{
    halo::interface::NetworkSetup::game_host_start(map_name, variant_name, disconnect_timeout_flag);
}

}
