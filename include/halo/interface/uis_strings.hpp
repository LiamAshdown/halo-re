#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::ui {

/**
 * Wide-string replacement helpers used to expand placeholders in UI text.
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiStrings {
    static int32_t real_to_int_truncate(float value);
    static void * replace_empty(widget_instance *widget);
    static void * replace_player_number(widget_instance *widget);
    static void * replace_product_id(widget_instance *widget);
    static void * replace_version(widget_instance *widget);
    static const uint16_t * search_replace_function_call(int16_t index, widget_instance *widget);
    static int32_t string_replace_all(wchar_t *search, uint16_t *replacement, wchar_t **buffer);
    static uint8_t wide_string_has_non_whitespace(const uint16_t *text);
};

}
