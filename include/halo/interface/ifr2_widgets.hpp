#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Widget drawing: instances, list heads, text boxes and screen regions.
 */
class WidgetRender {
public:
    explicit constexpr WidgetRender(widget_instance *view_widget) : widget(view_widget) {}
    widget_instance *widget;

    static void draw_fullscreen_region(int16_t controller_index);
    static void draw_split_screen_region(Rectangle2D *viewport, int16_t controller_index);
    void render(Rectangle2D *dest, int32_t offset_xy, uint32_t flag1, int32_t flag2);
    void render_column_list_items(UIWidgetDefinition *tag, Rectangle2D *dest, int32_t offset_xy, uint32_t flags);
    void render_list_head(UIWidgetDefinition *tag, Rectangle2D *dest, int32_t offset_xy, uint8_t is_top_of_stack);
    void render_text_box(UIWidgetDefinition *tag, Rectangle2D *dest, int32_t offset_xy, uint8_t is_top_of_stack);
};

/**
 * List-type widget selection, scrolling and synchronisation.
 */
class WidgetList {
public:
    explicit constexpr WidgetList(widget_instance *view_widget) : widget(view_widget) {}
    widget_instance *widget;

    void column_list_sync_selected();
    uint32_t cyclable_list_nudge();
    void extended_description_sync_selection();
    void select_list_index(datum_index list_definition, int32_t selection);
    void adjust_rect_for_scroll_arrows(Rectangle2D *rect);
    static widget_instance * get_child_by_index(widget_instance *list, int32_t index);
    static void scroll_window(int32_t out[3], widget_instance *widget);
    uint8_t select_next();
    uint8_t select_previous();
    void spinner_list_sync_selected(UIWidgetDefinition *tag);
};

/**
 * Single line text edit state operations.
 */
class TextEdit {
public:
    explicit constexpr TextEdit(text_edit_state *view_state) : state(view_state) {}
    text_edit_state *state;

    void clamp_selection();
    uint32_t get_selection(int16_t *out_start, int16_t *out_end);
    void insert_string(char *insert_str);
    void process_key(ui_key_event *event);
    void reset_length();
};

/**
 * Widget creation, closing, memory pool and focus history.
 */
class WidgetLifecycle {
public:
    explicit constexpr WidgetLifecycle(widget_instance *view_widget) : widget(view_widget) {}
    widget_instance *widget;

    static void pop(widget_history_node *out, widget_history_node **head);
    static void prepend(widget_history_node *template_record, widget_history_node **head);
    void close();
    static void close_all();
    uint8_t create_children_from_tag(UIWidgetDefinition *tag);
    void initialize_from_tag(datum_index tag_index, widget_instance *parent, uint16_t controller_index, UIWidgetDefinition *tag);
    void close_and_restore_previous();
    static void memory_pool_initialize();
    static void play_sound_effect(int16_t effect_id);
    static void play_sound_effect_tag(datum_index sound_tag);
    static void pool_list_free_all(widget_history_node **head);
    widget_instance * reopen_as_root_with_history(datum_index open_tag);
};

/**
 * Widget tree navigation, focus handling and input dispatch.
 */
class WidgetView {
public:
    explicit constexpr WidgetView(widget_instance *view_widget) : widget(view_widget) {}
    widget_instance *widget;

    int32_t cursor_side_of_midpoint();
    widget_instance * find_by_tag_id(datum_index tag_id);
    void focus_next_child();
    void focus_previous_child();
    int32_t get_sibling_index();
    widget_instance * find_at_point(int32_t cursor_x, int32_t cursor_y, int32_t offset_xy);
    widget_instance * find_root();
    float get_cumulative_scale();
    void handle_input_event(UIWidgetDefinition *tag, int16_t *event, uint8_t *out_handled);
    uint8_t is_input_eligible();
    uint8_t is_top_of_stack();
    uint8_t point_in_bounds();
    void relink_focus(widget_instance *child);
    void set_state_recursive(uint8_t state);
    static uint8_t verify_stack_chain(widget_instance *node);
    void relink_focus_by_tag_id(datum_index child_definition);

private:
    static uint8_t widget_is_focus_candidate(widget_instance *candidate);
};

} // namespace halo::interface
