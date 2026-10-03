#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::ui {

/**
 * Level, map and variant selection lists and the campaign start and restart paths.
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiGameSetup {
    static uint32_t build_level_select_list(widget_instance *widget, void *param_2, void *param_3);
    static void build_level_select_list_coop(widget_instance *widget, void *param_2, void *param_3);
    static void game_variant_flag_list_widget_build(widget_instance *widget);
    static void game_variant_list_widget_build(widget_instance *widget);
    static uint8_t level_select_confirm_choice(widget_instance *widget);
    static uint8_t map_select_confirm_choice(widget_instance *widget);
    static uint32_t restart_saved_game(void);
    static uint32_t start_campaign_from_level_one(void *widget, int16_t *event);
    static uint8_t variant_name_is_available(const uint16_t *name);
};

}
