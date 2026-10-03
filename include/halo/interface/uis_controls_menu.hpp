#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

struct saved_player_profile;

namespace halo::ui {

/**
 * Controls options menu: binding rows, sensitivity rows and the profile reload.
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiControlsMenu {
    static void controls_4wide_selector_refresh(widget_instance *widget);
    static uint32_t controls_options_free_list(widget_instance *widget);
    static uint32_t controls_options_populate_from_profile(widget_instance *widget);
    static uint8_t controls_options_reload_profile(void);
    static void controls_populate_bind_rows(widget_instance *widget, uint32_t packed);
    static void controls_populate_input_row(widget_instance *widget, const saved_player_profile *profile_record);
    static void controls_populate_sensitivity_row(widget_instance *widget, const saved_player_profile *profile_record);
    static uint32_t controls_sensitivity_row_refresh(widget_instance *widget, const saved_player_profile *profile_record);
};

}
