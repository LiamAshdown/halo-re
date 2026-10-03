#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::ui {

/**
 * The UI selection lists (add, find, format, rebuild rows and scrolling).
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiLists {
    static void list_add_entry(int32_t group_index, const uint16_t *name, int32_t id, const void *data_blob,
                        uint32_t data_size, uint8_t is_default);
    static uint8_t list_default_item_format(void *item_buffer, int32_t item_index, void *list_items);
    static int32_t list_find_default(int32_t group_index);
    static void list_free_all(void);
    static void * list_get_data(int32_t index);
    static int32_t list_get_id(int32_t index);
    static uint8_t list_item_format_name_and_cache_flag(uint16_t *out_name, int32_t item_index);
    static int32_t list_widget_compute_scroll_start(widget_instance *widget);
    static void list_widget_rebuild_rows(widget_instance *widget, ui_list_item_format_function format_item);
    static void selection_list_mirror_value_build(widget_instance *widget);
    static void widget_list_item_activate(widget_instance *widget, UIWidgetDefinition *tag, int16_t *event,
                                   EventHandlerReference *handler, uint8_t *out_handled);
};

}
