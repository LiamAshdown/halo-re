#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::ui {

/**
 * Level, map, profile and variant carousel refresh and slot cache population.
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiCarousels {
    static int32_t carousel_slot_compare_valid_first(const int32_t *a, const int32_t *b);
    static void level_carousel_refresh(widget_instance *widget);
    static void level_carousel_row_refresh(widget_instance *widget, int32_t level_index);
    static void map_list_carousel_refresh_window(widget_instance *widget);
    static void variant_carousel_slot_cache_populate(int32_t *candidate_ids, int32_t count);
};

}
