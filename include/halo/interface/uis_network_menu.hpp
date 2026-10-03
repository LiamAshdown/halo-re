#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

struct saved_player_profile;

namespace halo::ui {

/**
 * Network game menu behaviour: host setup, adapter details, client connection and wait timeouts.
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiNetworkMenu {
    static void network_adapter_details_refresh(widget_instance *widget);
    static void network_adapter_list_widget_build(widget_instance *widget);
    static uint8_t network_client_connect_and_save(void);
    static uint8_t network_game_options_populate(widget_instance *widget, const saved_player_profile *options_record);
    static void network_game_options_refresh(widget_instance *widget, const saved_player_profile *options_record);
    static uint8_t network_host_setup_defaults_init(widget_instance *widget);
    static void network_host_setup_refresh(widget_instance *widget);
    static void network_name_fields_refresh(widget_instance *widget);
    static uint32_t network_name_fields_reset(void);
    static void network_wait_timeout_check(void);
    static void network_wait_timeout_start(void);
    static uint8_t server_list_connect_selected(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t server_type_option_selected(widget_instance *widget);
};

}
