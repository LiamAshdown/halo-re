#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::ui {

/**
 * Profile list and profile selection behaviour of the UI.
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiProfiles {
    static uint32_t build_profile_list(widget_instance *widget);
    static uint32_t free_profile_list(widget_instance *widget);
    static uint32_t new_profile_name_entry_commit(void);
    static uint8_t new_profile_name_entry_open(void *widget, int16_t *event, uint8_t *out_handled);
    static void profile_carousel_fetch_name(widget_instance *widget);
    static void profile_carousel_fetch_sensitivity(widget_instance *widget);
    static void profile_carousel_slot_cache_populate(int32_t count, const int32_t *candidate_ids);
    static void profile_details_list_widget_build(widget_instance *widget);
    static uint8_t profile_list_apply_selection(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint32_t profile_list_apply_selection_for_player(widget_instance *widget, int16_t *context);
    static uint8_t profile_require_existing(void *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t profile_select_or_create(void *widget, int16_t *event, uint8_t *out_handled);
};

}
