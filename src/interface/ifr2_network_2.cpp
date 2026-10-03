#include "halo/interface/ifr2_network.hpp"
#include "halo/main/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern network_server_globals *network_server;
extern uint8_t network_server_host_valid;
extern int16_t network_game_mode;
extern uint8_t network_disconnect_timeout_flag;
extern network_client_globals *network_client;
extern uint8_t network_host_handoff_requested;
extern uint16_t split_screen_quit_prompt_string;
extern uint8_t split_screen_quit_prompt_armed;
extern uint8_t network_join_error_reason;
extern void network_game_server_host_dispose(void *host);
extern void network_client_globals_dispose(void);
extern uint8_t network_game_server_host_create(void);
extern void *network_session_create(void);
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
    if (network_server != (void *)0) {
        network_game_server_host_dispose(network_server);
        network_server = (network_server_globals *)((void *)0);
        network_server_host_valid = 0;
    }
    network_client_globals_dispose();
    network_game_mode = 0;
    halo::main::main_queue_map_change_by_name_or_clear(map_name);

    halo::game::game_engine_get_variant_by_name(variant_name, &variant);
    halo::game::globals().variant_saved_default = variant;
    halo::game::globals().variant_saved_default_valid = 1;

    widget = halo::interface::chimera__load_ui_widget(
        (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen",
        (datum_index)0xffffffff, (widget_instance *)0, 0xffff, (datum_index)0xffffffff,
        (datum_index)0xffffffff, -1);
    if (widget != (widget_instance *)0) {
        halo::game::game_engine_ensure_variant_history_has_entry();
        network_disconnect_timeout_flag = disconnect_timeout_flag;
        if (network_game_server_host_create() != 0) {
            network_client = (network_client_globals *)(network_session_create());
            if (network_client != (void *)0) {
                network_host_handoff_requested = 0;
                halo::game::globals().variant_history_current = -1;
                halo::game::game_engine_apply_current_custom_variant();
                halo::game::game_engine_sync_variant_defaults();
                network_game_mode = 2;
                return;
            }
        }
        if (network_server != (void *)0) {
            network_game_server_host_dispose(network_server);
            network_server = (network_server_globals *)((void *)0);
            network_server_host_valid = 0;
        }
        network_client_globals_dispose();
        network_disconnect_timeout_flag = 0;
    }
    split_screen_quit_prompt_string = 0xffff;
    network_join_error_reason = 0;
    split_screen_quit_prompt_armed = 1;
}

} // namespace halo::interface

namespace halo::interface {

void network_game_host_start(char *map_name, char *variant_name, uint8_t disconnect_timeout_flag)
{
    halo::interface::NetworkSetup::game_host_start(map_name, variant_name, disconnect_timeout_flag);
}

}
