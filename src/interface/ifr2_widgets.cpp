#include "halo/interface/ifr2_widgets.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/text/api.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/interface/engine_state.hpp"
#include "sound.h"
#include <string.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/input/api.hpp"
#include "halo/text/text.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/constants.hpp"
#include "halo/interface/flags.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/ai/api.hpp"
#include "halo/interface/widget_pool.hpp"
#include "halo/interface/ui_event.hpp"

#ifdef interface
#undef interface
#endif

static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &ui_root_widget = halo::link::ref<widget_instance *[1]>(halo::ui::vars().ui_root_widget);
static auto &ui_event_function_table = halo::link::ref<void *[0xbe]>(halo::ui::vars().ui_event_function_table);
static auto &ui_pause_depth = halo::link::ref<int16_t>(halo::ui::vars().ui_pause_depth);
static auto &ui_split_screen = halo::link::ref<uint8_t>(halo::ui::vars().ui_split_screen);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &ui_widget_history = halo::link::ref<widget_history_node *[3]>(halo::ui::vars().ui_widget_history);
static auto &controls_capture_row = halo::link::ref<int32_t>(halo::ui::vars().controls_capture_row);
static auto &controls_input_capture_flags = halo::link::ref<uint8_t>(halo::ui::vars().controls_input_capture_flags);
static auto &controls_input_capture_buffer = halo::link::ref<uint8_t [0x290]>(halo::ui::vars().controls_input_capture_buffer);
static auto &widget_creating_children = halo::link::ref<uint8_t>(halo::ui::vars().widget_creating_children);
static auto &ui_cursor_x = halo::link::ref<int32_t>(halo::ui::vars().ui_cursor_x);
static auto &last_controller_index_00879f50 = halo::link::ref<int32_t>(halo::ui::vars().last_controller_index_00879f50);
static auto &virtual_keyboard = halo::link::ref<virtual_keyboard_globals>(halo::ui::vars().virtual_keyboard);
static auto &ui_time_milliseconds = halo::link::ref<int32_t>(halo::ui::vars().ui_time_milliseconds);
static auto &ui_restoring_previous_widget = halo::link::ref<uint8_t>(halo::ui::vars().ui_restoring_previous_widget);
static auto &split_screen_quit_prompt_string = halo::link::ref<uint16_t>(halo::ui::vars().split_screen_quit_prompt_string);
static auto &split_screen_quit_prompt_armed = halo::link::ref<uint8_t>(halo::ui::vars().split_screen_quit_prompt_armed);
static auto &game_data_input_function_table = halo::link::ref<void *[0x3b]>(halo::ui::vars().game_data_input_function_table);
static auto &override_color_00879f40 = halo::link::ref<float>(halo::ui::vars().override_color_00879f40);
static auto &override_color_00879f44 = halo::link::ref<float>(halo::ui::vars().override_color_00879f44);
static auto &override_color_00879f48 = halo::link::ref<float>(halo::ui::vars().override_color_00879f48);
static auto &override_color_00879f4c = halo::link::ref<float>(halo::ui::vars().override_color_00879f4c);

namespace halo::interface {

/**
 * A sibling is a focus candidate if it is not hidden and is either itself a list (spinner_list/column_list) or
 * its tag has at least one game_data_input binding.
 */
uint8_t WidgetView::widget_is_focus_candidate(widget_instance *candidate)
{
    UIWidgetDefinition *tag = halo::interface::tag_data<UIWidgetDefinition>(candidate->definition);
    return (uint8_t)(candidate->hidden == 0 &&
                      (tag->game_data_inputs.count > 0 || candidate->widget_type == uiwidgettype_spinner_list ||
                       candidate->widget_type == uiwidgettype_column_list));
}

/**
 * blam-cc: ECX -> out, EDX -> head Pops the head widget_history_node from `*head` into `*out`
 * (definition/list_definition/ selection/controller_index only, not the link), advances `*head`, and frees the
 * popped node's own heap block.
 *
 * @address 0x499460
 */
void WidgetLifecycle::pop(widget_history_node *out, widget_history_node **head)
{
    widget_history_node *node = *head;
    out->definition = node->definition;
    out->list_definition = node->list_definition;
    out->selection = node->selection;
    out->controller_index = node->controller_index;
    *head = node->next;

    halo::interface::widget_pool_free(node);
}

/**
 * blam-cc: ESI -> template_record, EDI -> head Allocates a widget_history_node from the widget heap, copies
 * definition/list_definition/ selection/controller_index from `template`, and pushes it onto the singly-linked
 * list at `*head`.
 *
 * @address 0x499430
 */
void WidgetLifecycle::prepend(widget_history_node *template_record, widget_history_node **head)
{
    widget_history_node *node =
        (widget_history_node *)halo::memory::heap_allocate(sizeof(widget_history_node), widget_memory_pool);

    if (node != (widget_history_node *)0) {
        node->definition = template_record->definition;
        node->list_definition = template_record->list_definition;
        node->selection = template_record->selection;
        node->controller_index = template_record->controller_index;
        node->next = *head;
        *head = node;
    }
}

/**
 * Closes a widget instance: fires its tag's "deleted" event handlers, unlinks it from the widget tree
 * (recursively closing every child first), frees its type-specific extra allocation (text for a text_box,
 * list_render_data plus a recursively-closed extended_description for a list), then unlinks its own heap block
 * and clears whichever root-widget slot pointed at it.
 *
 * @address 0x497c00
 */
void WidgetLifecycle::close()
{
    uint8_t *self = (uint8_t *)widget;
    static_assert(offsetof(UIWidgetDefinition, event_handlers) == 0x54 && sizeof(ChildWidgetReference) == 0x50, "UIWidgetDefinition block layout");

    if (widget->closing != 0) {
        return;
    }
    widget->closing = 1;

    if (widget->controller_index != -1 && widget->parent == (widget_instance *)0) {
        local_player_control *entry = &halo::game::globals().player_control->local_players[0] + (int16_t)widget->controller_index;

        entry->suppressed_buttons |= halo::interface::k_menu_button_mask;
        entry->suppressed_until_released |= halo::interface::k_menu_button_mask;
    }

    {
        UIWidgetDefinition *definition = halo::interface::tag_data<UIWidgetDefinition>(widget->definition);
        TagReflexive *event_handlers = &definition->event_handlers;
        int32_t i;

        for (i = 0; i < (int32_t)event_handlers->count; i++) {
            EventHandlerReference *handler = halo::interface::reflexive_elements<EventHandlerReference>(*event_handlers) + i;

            if (handler->event_type == uieventtype_deleted && (int8_t)handler->flags < 0) {
                uint16_t function = (uint16_t)handler->function;

                if ((int16_t)function >= 0 && function < 0xbe) {
                    ui_event_function fn = (ui_event_function)ui_event_function_table[function];
                    uint8_t handled = 0;

                    if (fn(widget, nullptr, &handled) == 1 && halo::interface::has_bit(handler->flags, halo::tags::event_handler_references_tag_flag::open_widget) &&
                        halo::interface::tag_handle(handler->widget_tag.tag_id) != halo::k_dword_none) {
                        halo::interface::widget_reopen_as_root_with_history(widget, halo::interface::tag_handle(handler->widget_tag.tag_id));
                    }
                }
            }
        }
    }

    if (widget->pauses_game_time == 1 && halo::networking::globals().game_mode != 2 &&
        ui_split_screen == 0) {
        ui_pause_depth = ui_pause_depth - 1;
        if (ui_pause_depth == 0 && halo::game::globals().game_time->paused != 0) {
            if (halo::game::globals().game_time->initialized != 0) {
                halo::game::globals().game_time->active = 1;
            }
            halo::game::globals().game_time->paused = 0;
        }
    }

    {
        widget_instance *child = widget->first_child;

        while (child != (widget_instance *)0) {
            widget_instance *next = child->next_sibling;

            halo::interface::widget_close(child);
            if (next == (widget_instance *)0) {
                break;
            }
            next->previous_sibling = (widget_instance *)0;
            child = next;
        }
    }

    if (widget->previous_sibling != (widget_instance *)0) {
        widget->previous_sibling->next_sibling = widget->next_sibling;
    }
    if (widget->next_sibling != (widget_instance *)0) {
        widget->next_sibling->previous_sibling = widget->previous_sibling;
    }
    if (widget->parent != (widget_instance *)0 && widget->parent->first_child == widget) {
        widget->parent->first_child = widget->next_sibling;
    }

    {
        heap *pool = widget_memory_pool;

        if (widget->widget_type == uiwidgettype_text_box) {
            if (widget->text != nullptr) {
                heap_block *block = (heap_block *)((uint8_t *)widget->text - 0x10);

                halo::memory::heap_unlink_block(block, widget_memory_pool);
                pool = widget_memory_pool;
            }
        } else if (widget->widget_type == uiwidgettype_spinner_list || widget->widget_type == uiwidgettype_column_list) {
            if (widget->list_render_data != nullptr) {
                heap_block *block = (heap_block *)((uint8_t *)widget->list_render_data - 0x10);

                halo::memory::heap_unlink_block(block, widget_memory_pool);
                pool = widget_memory_pool;
            }
            if (widget->extended_description != (widget_instance *)0) {
                halo::interface::widget_close(widget->extended_description);
                pool = widget_memory_pool;
            }
        }

        {
            heap_block *self_block = (heap_block *)(self - 0x10);
            uint32_t size = self_block->size;
            int32_t slot = self_block->slot;

            if (self_block->previous != (heap_block *)0) {
                self_block->previous->next = self_block->next;
            }
            if (self_block->next != (heap_block *)0) {
                self_block->next->previous = self_block->previous;
            }
            if (self_block == pool->first_block) {
                pool->first_block = self_block->next;
            }
            if (self_block == pool->last_block) {
                pool->last_block = self_block->previous;
            }
            pool->blocks[slot] = (heap_block *)0;
            pool->next_free_slot = (pool->first_block != (heap_block *)0) ? slot : 0;
            pool->allocation_count = pool->allocation_count - 1;
            pool->bytes_allocated = pool->bytes_allocated - (int32_t)(size & halo::interface::k_pool_block_size_mask);
        }
    }

    if (ui_root_widget[0] == widget) {
        ui_root_widget[0] = (widget_instance *)0;
    }
}

/**
 * Closes the (single) root widget, frees its go-back history list, resets ui_pause_depth to 0, and, when
 * controls_capture_row is armed, clears the controls input capture buffer.
 *
 * @address 0x498650
 */
void WidgetLifecycle::close_all()
{
    int32_t i;

    if (ui_root_widget[0] != (widget_instance *)0) {
        halo::interface::widget_close(ui_root_widget[0]);
    }
    if (ui_widget_history[0] != (widget_history_node *)0) {
        halo::interface::widget_pool_list_free_all(&ui_widget_history[0]);
    }
    ui_pause_depth = 0;
    if (controls_capture_row != -1) {
        controls_input_capture_flags = controls_input_capture_flags & 0xf7;
        for (i = 0; i < halo::interface::k_controls_capture_buffer_size; i++) {
            controls_input_capture_buffer[i] = 0;
        }
        controls_capture_row = -1;
    }
}

/**
 * @address 0x49c040
 */
void WidgetList::column_list_sync_selected()
{
    widget_instance *child;

    for (child = widget->first_child; child != (widget_instance *)0; child = child->next_sibling) {
        if (child == widget->focused_child) {
            if (child->background_bitmap_frames == 2) {
                child->background_bitmap_frame = 1;
            }
        } else if (child->background_bitmap_frames == 2) {
            child->background_bitmap_frame = 0;
        }
    }
}

/**
 * blam-cc: stack -> tag, ESI -> widget Instantiates and links in all of a widget's static child widgets: once
 * per string when the tag draws its list items from a string list (each such "child" reuses this widget's own
 * tag), then once per explicit ChildWidgetReference entry (honoring a per-child custom controller index and
 * its horizontal/vertical offset), then the extended_description child if this is a list type, finally picking
 * a default focused_child among the real children when the tag allows it.
 *
 * @address 0x499540
 */
uint8_t WidgetLifecycle::create_children_from_tag(UIWidgetDefinition *tag)
{
    uint8_t ok = 1;
    int32_t i;

    if (halo::interface::has_bit(tag->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_items_from_string_list_tag)) {
        UnicodeStringList *list =
            halo::interface::tag_data<UnicodeStringList>(halo::interface::tag_handle(tag->text_label_unicode_strings_list.tag_id));

        widget_creating_children = 1;
        for (i = 0; i < list->strings.count; i++) {
            widget_instance *child = halo::interface::chimera__load_ui_widget(nullptr, widget->definition, widget,
                                                               widget->controller_index, (datum_index)-1,
                                                               (datum_index)-1, -1);

            if (child == (widget_instance *)0) {
                ok = 0;
                break;
            }
            if (widget->first_child == (widget_instance *)0) {
                widget->first_child = child;
            } else {
                widget_instance *tail = widget->first_child;

                while (tail->next_sibling != (widget_instance *)0) {
                    tail = tail->next_sibling;
                }
                tail->next_sibling = child;
                child->previous_sibling = tail;
            }
            widget->item_count = widget->item_count + 1;
        }
        widget_creating_children = 0;
    }

    if (tag->child_widgets.count > 0) {
        ChildWidgetReference *entries = halo::interface::reflexive_elements<ChildWidgetReference>(tag->child_widgets);

        for (i = 0; i < tag->child_widgets.count; i++) {
            ChildWidgetReference *entry = entries + i;
            datum_index child_tag_index = halo::interface::tag_handle(entry->widget_tag.tag_id);

            if (child_tag_index != (datum_index)-1) {
                uint16_t controller = widget->controller_index;
                widget_instance *child;

                if (halo::interface::has_bit(entry->flags, halo::tags::child_widget_reference_tag_flag::use_custom_controller_index) && entry->custom_controller_index < 4) {
                    controller = entry->custom_controller_index;
                }
                child = halo::interface::chimera__load_ui_widget(nullptr, child_tag_index, widget, controller,
                                                 (datum_index)-1, (datum_index)-1, -1);
                if (child == (widget_instance *)0) {
                    ok = 0;
                    break;
                }
                child->local_x = entry->horizontal_offset + widget->local_x;
                child->local_y = entry->vertical_offset + widget->local_y;
                if (widget->first_child == (widget_instance *)0) {
                    widget->first_child = child;
                } else {
                    widget_instance *tail = widget->first_child;

                    while (tail->next_sibling != (widget_instance *)0) {
                        tail = tail->next_sibling;
                    }
                    tail->next_sibling = child;
                    child->previous_sibling = tail;
                }
            }
        }
    }

    if ((widget->widget_type == uiwidgettype_spinner_list || widget->widget_type == uiwidgettype_column_list) &&
        halo::interface::tag_handle(tag->extended_description_widget.tag_id) != halo::k_dword_none) {
        widget_instance *desc = halo::interface::chimera__load_ui_widget(
            nullptr, halo::interface::tag_handle(tag->extended_description_widget.tag_id), widget,
            widget->controller_index, (datum_index)-1, (datum_index)-1, -1);

        widget->extended_description = desc;
        if (desc != (widget_instance *)0) {
            if (desc->previous_sibling != (widget_instance *)0) {
                desc->previous_sibling->next_sibling = (widget_instance *)0;
            }
            desc->previous_sibling = (widget_instance *)0;
            desc->parent = (widget_instance *)0;
        }
    }

    if ((int8_t)tag->flags >= 0) {
        if (widget->widget_type == uiwidgettype_spinner_list || widget->widget_type == uiwidgettype_column_list) {
            widget->selection_index = 0;
            widget->scroll_blink = 0;
            widget->selection_direction = 0;
        } else if (!halo::interface::has_bit(tag->flags, halo::tags::ui_widget_definition_tag_flag::pass_unhandled_events_to_focused_child)) {
            return ok;
        }
        {
            widget_instance *child = widget->first_child;

            while (child != (widget_instance *)0) {
                if (widget->widget_type == uiwidgettype_spinner_list || widget->widget_type == uiwidgettype_column_list) {
                    break;
                }
                {
                    UIWidgetDefinition *child_tag =
                        halo::interface::tag_data<UIWidgetDefinition>(child->definition);

                    if (child->hidden == 0 &&
                        (child_tag->event_handlers.count > 0 || child->widget_type == uiwidgettype_spinner_list ||
                         child->widget_type == uiwidgettype_column_list)) {
                        break;
                    }
                }
                child = child->next_sibling;
            }
            if (child != (widget_instance *)0) {
                widget->focused_child = child;
            }
        }
    }
    return ok;
}

/**
 * blam-cc: EAX -> widget
 *
 * @address 0x4a1ff0
 */
int32_t WidgetView::cursor_side_of_midpoint()
{
    UIWidgetDefinition *tag = halo::interface::tag_data<UIWidgetDefinition>(widget->definition);
    int16_t cumulative_x = 0;
    widget_instance *ancestor;
    int16_t left;
    int16_t right;
    int32_t midpoint;

    for (ancestor = widget; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
        cumulative_x = cumulative_x + ancestor->local_x;
    }

    Rectangle2D rect = tag->bounds;
    halo::interface::widget_list_adjust_rect_for_scroll_arrows(widget, &rect);
    left = rect.left;
    right = rect.right;

    midpoint = ((int16_t)(right + cumulative_x) + (int16_t)(left + cumulative_x)) / 2;
    return (midpoint < ui_cursor_x) * 2 - 1;
}

/**
 * @address 0x4a2070
 */
uint32_t WidgetList::cyclable_list_nudge()
{
    int32_t side = halo::interface::widget_cursor_side_of_midpoint(widget);
    int32_t new_index;

    if (side < 1) {
        new_index = widget->selection_index - 1;
        if (new_index < 0) {
            new_index = (uint16_t)widget->item_count - 1;
        }
        if (new_index == widget->selection_index) {
            goto play_and_return;
        }
        ((struct widget_instance *)widget)->scroll_blink = -halo::interface::k_scroll_blink_ticks;
        ((struct widget_instance *)widget)->selection_direction = -1;
    } else {
        if (side != 1) {
            return 1;
        }
        new_index = widget->selection_index + 1;
        if (new_index >= (uint16_t)widget->item_count) {
            new_index = 0;
        }
        if (new_index == widget->selection_index) {
            goto play_and_return;
        }
        ((struct widget_instance *)widget)->scroll_blink = halo::interface::k_scroll_blink_ticks;
        ((struct widget_instance *)widget)->selection_direction = 1;
    }
    widget->selection_index = (int16_t)new_index;
    halo::interface::widget_play_sound_effect(0);

play_and_return:
    halo::interface::widget_play_sound_effect(0);
    return 1;
}

/**
 * Draws all currently active full-screen UI widgets (root widget slot 0, matching
 * widget_draw_split_screen_region's own eligibility test), the mouse cursor on top of them if any were drawn,
 * and, once a network-wait alpha is fading in the 0..1 range, a fullscreen fade quad over everything. If the
 * virtual keyboard is open, draws it (plus the cursor) instead of any of that.
 *
 * @address 0x4984c0
 */
void WidgetRender::draw_fullscreen_region(int16_t controller_index)
{
    uint8_t drew_any = 0;
    int32_t clamped_controller;
    int32_t i;

    last_controller_index_00879f50 = (controller_index == (int16_t)halo::k_word_none) ? 0 : controller_index;

    if (virtual_keyboard.active != 0) {
        halo::interface::virtual_keyboard_render();
        halo::interface::interface_draw_cursor();
        return;
    }

    clamped_controller = 0;

    for (i = 0; i < 1; i++) {
        widget_instance *widget = ui_root_widget[i];

        if (widget == (widget_instance *)0) {
            continue;
        }
        if (widget->render_always == 1 ||
            (widget->is_error_dialog == 1 &&
             (widget->controller_index == clamped_controller || widget->controller_index == -1 ||
              ui_split_screen != 0)) ||
            (widget->is_error_dialog != 1 &&
             ((widget->controller_index == -1 && i == 0) || widget->controller_index == clamped_controller))) {
            Rectangle2D dest = {0, 0, halo::interface::k_base_screen_height, halo::interface::k_base_screen_width};

            halo::interface::widget_instance_render(widget, &dest, 0, 1, 0);
            drew_any = 1;
        }
    }

    if (drew_any != 0) {
        halo::interface::interface_draw_cursor();
    }
    if (0.0f <= state::screen_fade_progress && (state::screen_fade_progress < 1.0f) != (state::screen_fade_progress == 1.0f)) {
        Rectangle2D rect = {0, 0, halo::interface::k_base_screen_height, halo::interface::k_base_screen_width};
        int32_t fade_color;

        if (0.95f <= state::screen_fade_progress) {
            state::screen_fade_progress = 1.0f;
        }
        fade_color = (int32_t)(state::screen_fade_progress * 255.0f + 0.5f);
        halo::interface::ui_draw_filled_rectangle((uint32_t)fade_color, &rect);
    }
}

/**
 * Draws the current root widget clipped to one split-screen viewport region. Only ever runs for controller
 * slot 0 (see header note); the destination rect is the caller's viewport shifted to the origin.
 *
 * @address 0x498330
 */
void WidgetRender::draw_split_screen_region(Rectangle2D *viewport, int16_t controller_index)
{

    Point2DInt offsets[19] = {};
    int32_t clamped_controller;
    int32_t i;

    offsets[8].y = k_base_screen_height / 2;
    offsets[12].y = k_base_screen_height / 2;
    offsets[13].x = k_base_screen_width / 2;
    offsets[13].y = k_base_screen_height / 2;
    offsets[16].x = k_base_screen_width / 2;
    offsets[17].y = k_base_screen_height / 2;
    offsets[18].x = k_base_screen_width / 2;
    offsets[18].y = k_base_screen_height / 2;

    if (virtual_keyboard.active != 0) {
        return;
    }

    clamped_controller = 0;

    for (i = 0; i < 1; i++) {
        widget_instance *widget = ui_root_widget[i];

        if (widget == (widget_instance *)0) {
            continue;
        }
        if (widget->render_always == 1 ||
            (widget->is_error_dialog == 1 &&
             (widget->controller_index == clamped_controller || widget->controller_index == -1 ||
              ui_split_screen != 0)) ||
            (widget->is_error_dialog != 1 &&
             ((widget->controller_index == -1 && i == 0) || widget->controller_index == clamped_controller))) {
            Rectangle2D dest;
            int32_t offset_index = clamped_controller + 4 * halo::game::globals().local_player_globals->local_player_count;
            int32_t offset_xy = *(int32_t *)&offsets[offset_index];

            dest.top = 0;
            dest.left = 0;
            dest.bottom = viewport->bottom - viewport->top;
            dest.right = viewport->right - viewport->left;
            halo::interface::widget_instance_render(widget, &dest, offset_xy, 1, 0);
        }
    }
}

/**
 * Enables (and reindexes) or disables a widget's extended_description widget by scanning the widget's own
 * children for a nested spinner_list, flattening the count/selection of every sub-list into a single index for
 * the shared description widget.
 *
 * @address 0x4a66b0
 */
void WidgetList::extended_description_sync_selection()
{
    widget_instance *description;
    widget_instance *focused;
    widget_instance *sibling;
    widget_instance *list_child;
    int32_t index;
    UIWidgetDefinition *list_tag;

    description = widget->extended_description;
    focused = widget->focused_child;
    if (focused == 0) {
        goto disable;
    }

    sibling = widget->first_child;
    index = 0;
    if (sibling == 0) {
        goto enable;
    }

    do {
        if (sibling->widget_type == uiwidgettype_column_list && sibling->next_sibling == 0) {
            goto disable;
        }

        list_child = sibling->first_child;
        for (; list_child != 0; list_child = list_child->next_sibling) {
            if (list_child->widget_type == uiwidgettype_spinner_list) {
                list_tag = halo::interface::tag_data<UIWidgetDefinition>(list_child->definition);
                if (sibling == focused) {
                    if (!halo::interface::has_bit(list_tag->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_items_only_one_tooltip)) {
                        index = index + list_child->selection_index;
                    }
                    goto enable;
                }
                if (halo::interface::has_bit(list_tag->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_items_only_one_tooltip)) {
                    index = index + 1;
                } else {
                    index = index + (uint16_t)list_child->item_count;
                }
                goto next_sibling;
            }
        }
        if (sibling == focused) {
            break;
        }
next_sibling:
        sibling = sibling->next_sibling;
    } while (sibling != 0);

    if (index == -1) {
        goto disable;
    }

enable:
    description->selection_index = (int16_t)index;
    description->state = 1;
    return;

disable:
    description->state = 0;
    return;
}

/**
 * @address 0x499950
 */
widget_instance * WidgetView::find_by_tag_id(datum_index tag_id)
{
    widget_instance *found;
    widget_instance *child;

    if (widget->definition == tag_id) {
        return widget;
    }
    found = (widget_instance *)0;
    child = widget->first_child;
    while (child != (widget_instance *)0 && found == (widget_instance *)0) {
        found = child;
        if (child->definition != tag_id) {
            found = halo::interface::widget_find_by_tag_id(child, tag_id);
        }
        child = child->next_sibling;
    }
    return found;
}

/**
 * blam-cc: EDX -> widget Starting just after the currently focused child (or at the first child if none is
 * focused), scans forward through the sibling ring -- wrapping past the last child back to the first -- for a
 * child that is not hidden and is either a list itself or has at least one game_data_input binding, or whose
 * container `widget` is itself a spinner_list/column_list, and focuses the first one found.
 *
 * @address 0x49c080
 */
void WidgetView::focus_next_child()
{
    widget_instance *current = widget->focused_child;
    widget_instance *candidate;

    if (current != (widget_instance *)0 && current->next_sibling != (widget_instance *)0) {
        candidate = current->next_sibling;
    } else {
        candidate = widget->first_child;
    }
    if (candidate == (widget_instance *)0) {
        return;
    }

    while (candidate != current) {
        UIWidgetDefinition *tag = halo::interface::tag_data<UIWidgetDefinition>(candidate->definition);

        if ((tag->game_data_inputs.count > 0 || halo::interface::has_bit(tag->flags, halo::tags::ui_widget_definition_tag_flag::pass_unhandled_events_to_focused_child) ||
             widget->widget_type == uiwidgettype_spinner_list || widget->widget_type == uiwidgettype_column_list) &&
            candidate->hidden == 0) {
            widget->focused_child = candidate;
            return;
        }
        candidate = candidate->next_sibling;
        if (candidate == (widget_instance *)0) {
            candidate = widget->first_child;
            if (candidate == (widget_instance *)0) {
                return;
            }
        }
    }
}

/**
 * blam-cc: EDX -> widget Mirror image of focus_next_child: starting just before the currently focused child (or at
 * the last child if none is focused), scans backward through the sibling ring -- wrapping past the first child
 * back to the last -- for an eligible candidate (same test as focus_next_child) and focuses it.
 *
 * @address 0x49c0f0
 */
void WidgetView::focus_previous_child()
{
    widget_instance *focused = widget->focused_child;
    widget_instance *candidate;
    widget_instance *tail;
    UIWidgetDefinition *tag;

    if (focused == (widget_instance *)0) {
        if (widget->first_child == (widget_instance *)0) {
            return;
        }
        candidate = widget->first_child->previous_sibling;
        if (candidate == (widget_instance *)0) {
            candidate = widget->first_child;
        }
    } else {
        candidate = focused->previous_sibling;
        if (candidate == (widget_instance *)0) {
            tail = widget->first_child;
            if (tail == (widget_instance *)0) {
                return;
            }
            while (tail->next_sibling != (widget_instance *)0) {
                tail = tail->next_sibling;
            }
            candidate = tail;
        }
    }

    for (;;) {
        while (candidate != (widget_instance *)0) {
            if (candidate == focused) {
                return;
            }
            tag = halo::interface::tag_data<UIWidgetDefinition>(candidate->definition);
            if ((tag->game_data_inputs.count > 0 || halo::interface::has_bit(tag->flags, halo::tags::ui_widget_definition_tag_flag::pass_unhandled_events_to_focused_child) ||
                 widget->widget_type == uiwidgettype_spinner_list || widget->widget_type == uiwidgettype_column_list) &&
                candidate->hidden == 0) {
                widget->focused_child = candidate;
                return;
            }
            candidate = candidate->previous_sibling;
        }
        candidate = widget->first_child;
        if (candidate == (widget_instance *)0) {
            return;
        }
        while (candidate->next_sibling != (widget_instance *)0) {
            candidate = candidate->next_sibling;
        }
    }
}

/**
 * @address 0x498e30
 */
int32_t WidgetView::get_sibling_index()
{
    int32_t index = -1;

    if (widget->parent != (widget_instance *)0) {
        widget_instance *cursor = widget->parent->first_child;
        int32_t i = 0;

        if (cursor != (widget_instance *)0) {
            while (cursor != widget) {
                cursor = cursor->next_sibling;
                i = i + 1;
                if (cursor == (widget_instance *)0) {
                    return -1;
                }
            }
            index = i;
        }
    }
    return index;
}

/**
 * blam-cc: ECX -> widget, EAX -> tag_index, EDX -> parent Populates a freshly-allocated widget instance's
 * fields from its tag definition, builds its static children (unless already inside a nested
 * widget_create_children_from_tag call), fires any "created" (event code 0x18) event handlers for its own
 * children's entries, picks a default focused_child among its own children when none was set, and, when this
 * widget pauses game time, raises the shared ui_pause_depth counter and updates the console pause-request
 * state.
 *
 * @address 0x499780
 */
void WidgetLifecycle::initialize_from_tag(datum_index tag_index, widget_instance *parent, uint16_t controller_index, UIWidgetDefinition *tag)
{
    int32_t i;

    {
        int32_t *zero = (int32_t *)widget;

        for (i = 0; i < 0x18; i++) {
            zero[i] = 0;
        }
    }

    if (halo::interface::has_bit(tag->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_items_from_string_list_tag) && parent != (widget_instance *)0 && tag_index == parent->definition) {
        widget->widget_type = uiwidgettype_text_box;
    }
    widget->controller_index = controller_index;
    widget->definition = tag_index;
    widget->name = (char *)tag + 4;
    widget->widget_type = tag->widget_type;
    widget->state = 1;
    widget->render_always = (uint8_t)(tag->flags >> 9) & 1;
    widget->pauses_game_time = (uint8_t)(tag->flags >> 1) & 1;
    widget->creation_time = ui_time_milliseconds;
    widget->milliseconds_to_auto_close =
        tag->milliseconds_to_auto_close & ((tag->milliseconds_to_auto_close < 1) ? 0 : -1);
    widget->milliseconds_auto_close_fade =
        tag->milliseconds_auto_close_fade_time & ((tag->milliseconds_auto_close_fade_time < 1) ? 0 : -1);
    widget->scale = 1.0f;
    widget->parent = parent;
    if (widget->widget_type == uiwidgettype_text_box) {
        widget->selection_index = -1;
        widget->list_render_data = nullptr;
    }
    if (halo::interface::tag_handle(tag->background_bitmap.tag_id) != halo::k_dword_none) {
        tag_instance *bg = &halo::cache::globals().tag_instances[halo::interface::tag_handle(tag->background_bitmap.tag_id) & halo::k_slot_mask];
        Bitmap *bitmap = (Bitmap *)bg->data;
        BitmapGroupSequence *seq = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer;

        widget->background_bitmap_frames = seq[0].bitmap_count;
    }
    if (widget_creating_children == 0) {
        halo::interface::widget_create_children_from_tag(widget, tag);
    }
    if (tag->child_widgets.count > 0) {
        uint8_t *entry = (uint8_t *)tag->child_widgets.pointer;

        for (i = 0; i < tag->child_widgets.count; i++, entry += 0x48) {
            if (*(int16_t *)(entry + 4) == 0x18) {
                uint8_t handled;
                int16_t event[4] = {0, 0, 0, 0};

                halo::interface::ui_widget_list_item_activate(widget, tag, event, (EventHandlerReference *)entry, &handled);
            }
        }
    }
    if (widget->focused_child == (widget_instance *)0) {
        widget_instance *child;

        for (child = widget->first_child; child != (widget_instance *)0; child = child->next_sibling) {
            UIWidgetDefinition *child_tag = halo::interface::tag_data<UIWidgetDefinition>(child->definition);

            if (child->hidden == 0 &&
                (child_tag->event_handlers.count > 0 || child->widget_type == uiwidgettype_spinner_list || child->widget_type == uiwidgettype_column_list)) {
                halo::interface::widget_instance_relink_focus(widget, child);
            }
        }
    }
    if (widget->pauses_game_time == 1 && halo::networking::globals().game_mode != 2 && ui_split_screen == 0) {
        ui_pause_depth = ui_pause_depth + 1;
        if (halo::game::globals().game_time->paused == 0) {
            if (halo::game::globals().game_time->initialized != 0) {
                halo::game::globals().game_time->active = 0;
            }
            halo::game::globals().game_time->paused = 1;
        }
    }
}

/**
 * blam-cc: EAX -> widget Closes the whole tree `widget` belongs to (climbing to its root first) and, if that
 * controller slot has a saved go-back record, pops it and reopens the widget it names (with no go-back record
 * of its own), restoring its saved list selection.
 *
 * @address 0x49c3e0
 */
void WidgetLifecycle::close_and_restore_previous()
{
    int16_t slot = (widget->controller_index == (int16_t)halo::k_word_none) ? 0 : widget->controller_index;
    widget_history_node history;
    datum_index history_definition = (datum_index)-1;
    widget_instance *root;
    widget_instance *reopened;

    if (ui_widget_history[slot] != (widget_history_node *)0) {
        halo::interface::list_node_pop(&history, &ui_widget_history[slot]);
        history_definition = history.definition;
    }

    root = widget;
    while (root->parent != (widget_instance *)0) {
        root = root->parent;
    }
    halo::interface::widget_close(root);

    if (history_definition != (datum_index)-1) {
        ui_restoring_previous_widget = 1;
        reopened = halo::interface::chimera__load_ui_widget(nullptr, history_definition, (widget_instance *)0,
                                            (uint16_t)history.controller_index,
                                            (datum_index)-1, (datum_index)-1, -1);
        ui_restoring_previous_widget = 0;
        if (reopened != (widget_instance *)0) {
            halo::interface::widget_instance_select_list_index(reopened, history.list_definition, history.selection);
        }
    }
}

/**
 * @address 0x498e10
 */
widget_instance * WidgetView::find_root()
{
    while (widget->parent != (widget_instance *)0) {
        widget = widget->parent;
    }
    return widget;
}

/**
 * @address 0x499c20
 */
float WidgetView::get_cumulative_scale()
{
    float scale = widget->scale;
    widget_instance *ancestor;

    for (ancestor = widget->parent; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
        scale = scale * ancestor->scale;
    }
    return scale;
}

/**
 * Processes an input event against a widget instance's bound UI events: first, if the widget has a
 * per-controller close request pending, closes the topmost ancestor and reports handled;
 *
 * @address 0x499d00
 */
void WidgetView::handle_input_event(UIWidgetDefinition *tag, int16_t *event, uint8_t *out_handled)
{
    uint8_t handled = 0;
    uint8_t list_nav_done = 0;
    uint8_t controller_matches;
    int16_t sound_effect = 0;
    int32_t handler_scan_count = 0;

    controller_matches = (widget->hidden == 0 &&
                           (widget->controller_index == -1 || widget->controller_index == event[1]));

    if (widget->close_on_controller_connected[0] == 1) {
        int16_t controller = widget->controller_index;
        widget_instance *ancestor = widget;

        if (controller < 0 || controller > 3) {
            int32_t i;

            for (i = 0; i <= 3; i++) {
                if (halo::input::globals().joystick_slot_devices[i] != -1) {
                    break;
                }
            }
            if (i > 3) {
                goto after_close_check;
            }
            ancestor = widget;
            while (ancestor->parent != (widget_instance *)0) {
                ancestor = ancestor->parent;
            }
            halo::interface::widget_close(ancestor);
            handled = 1;
        } else if (halo::input::globals().joystick_slot_devices[controller] != -1) {
            ancestor = widget;
            while (ancestor->parent != (widget_instance *)0) {
                ancestor = ancestor->parent;
            }
            halo::interface::widget_close(ancestor);
            handled = 1;
        }
    }
after_close_check:

    if (controller_matches && handled == 0 && event[0] == 3 && halo::interface::event_state(event) == 1) {
        int8_t code = (int8_t)event[2];
        uint8_t found = 0;

        if (code == '\r') {
            if (tag->event_handlers.count > 0) {
                EventHandlerReference *entry = halo::interface::reflexive_elements<EventHandlerReference>(tag->event_handlers);
                int32_t i;

                for (i = 0; i < tag->event_handlers.count; i++, entry++) {
                    if (entry->event_type == 0xd) {
                        found = 1;
                        break;
                    }
                }
            }
        } else if (code == 1) {
            if (tag->event_handlers.count > 0) {
                EventHandlerReference *entry = halo::interface::reflexive_elements<EventHandlerReference>(tag->event_handlers);
                int32_t i;

                for (i = 0; i < tag->event_handlers.count; i++, entry++) {
                    if (entry->event_type == 1) {
                        found = 1;
                        break;
                    }
                }
            }
        } else {
            found = 1;
        }
        if (!found) {
            halo::interface::widget_instance_close_and_restore_previous(widget);
            sound_effect = 3;
            list_nav_done = 1;
            handled = 1;
        }
    }

    if (widget->milliseconds_to_auto_close == 1) {
        widget->state = 0;
    }
    if (handled == 0) {
        if (widget->milliseconds_to_auto_close != 0) {
            int32_t fade = widget->milliseconds_auto_close_fade;
            int32_t close_at = ui_time_milliseconds - widget->creation_time;

            if ((uint32_t)(fade + widget->milliseconds_to_auto_close) <= (uint32_t)close_at) {
                widget_instance *ancestor = widget;

                while (ancestor->parent != (widget_instance *)0) {
                    ancestor = ancestor->parent;
                }
                halo::interface::widget_close(ancestor);
                handled = 1;
                goto dispatch_to_children;
            }
            if (fade != 0) {
                int32_t remaining = close_at - widget->milliseconds_to_auto_close;

                if (remaining > 0) {
                    widget_instance *ancestor = widget;

                    while (ancestor->parent != (widget_instance *)0) {
                        ancestor = ancestor->parent;
                    }
                    ancestor->scale = 1.0f - (float)remaining / (float)fade;
                }
            }
        }
        {
            int16_t *word_5a = (int16_t *)&widget->unknown_5a[0];
            int16_t *word_5c = (int16_t *)&widget->unknown_5a[2];

            if (*word_5a < 0) *word_5a = 0;
            if (*word_5c < 0) *word_5c = 0;
        }
        if (widget->widget_type == uiwidgettype_spinner_list) {
            halo::interface::widget_spinner_list_sync_selected(widget, tag);
        } else if (widget->widget_type == uiwidgettype_column_list) {
            halo::interface::widget_column_list_sync_selected(widget);
        }
        if (controller_matches) {
            if (list_nav_done != 0) {
                goto dispatch_to_children;
            }
            if (halo::interface::has_bit(tag->flags, halo::tags::ui_widget_definition_tag_flag::dpad_up_down_tabs_thru_children) && widget->focused_child != (widget_instance *)0 && handled == 0) {
                if (event[0] == 3 && halo::interface::event_state(event) == 1) {
                    int8_t code = (int8_t)event[2];

                    if (code == '\b') {
                        goto tab_forward;
                    }
                    if (code != '\t') {
                        goto dpad_lr_nav;
                    }
                    halo::interface::widget_focus_next_child(widget);
                    goto tab_commit;
                } else if (event[0] == 1) {
                    if (event[3] != halo::interface::k_event_argument_unset) {
                        if (event[3] == halo::interface::k_action_none) {
                            goto tab_forward;
                        }
                        goto dpad_lr_nav;
                    }
                    goto tab_back;
                }
            }
            goto dpad_lr_nav;
        tab_back:
            halo::interface::widget_focus_next_child(widget);
        tab_commit:
            if (sound_effect == 0) {
                sound_effect = 1;
            }
            list_nav_done = 1;
            goto dpad_lr_nav_done;
        tab_forward:
            halo::interface::widget_focus_previous_child(widget);
            goto tab_commit;

        dpad_lr_nav:
            if (halo::interface::has_bit(tag->flags, halo::tags::ui_widget_definition_tag_flag::dpad_left_right_tabs_thru_children) && widget->focused_child != (widget_instance *)0 && handled == 0) {
                if (event[0] == 3 && halo::interface::event_state(event) == 1) {
                    int8_t code = (int8_t)event[2];

                    if (code == '\n') {
                        halo::interface::widget_focus_previous_child(widget);
                        goto tab_commit;
                    }
                    if (code == '\v') {
                        halo::interface::widget_focus_next_child(widget);
                        goto tab_commit;
                    }
                } else if (event[0] == 1) {
                    if (event[2] == halo::interface::k_event_argument_unset) {
                        halo::interface::widget_focus_previous_child(widget);
                        goto tab_commit;
                    }
                    if (event[2] == halo::interface::k_action_none) {
                        halo::interface::widget_focus_next_child(widget);
                        goto tab_commit;
                    }
                }
            }
        dpad_lr_nav_done:
            if (halo::interface::has_bit(tag->flags, halo::tags::ui_widget_definition_tag_flag::dpad_up_down_tabs_thru_list_items) && (widget->widget_type == uiwidgettype_spinner_list || widget->widget_type == uiwidgettype_column_list) &&
                list_nav_done == 0 && handled == 0) {
                if (event[0] == 3 && halo::interface::event_state(event) == 1) {
                    int8_t code = (int8_t)event[2];

                    if (code == '\b') {
                        halo::interface::widget_list_select_previous(widget);
                    } else if (code == '\t') {
                        halo::interface::widget_list_select_next(widget);
                    } else {
                        goto dpad_ud_nav;
                    }
                    if (sound_effect == 0) {
                        sound_effect = 1;
                    }
                    list_nav_done = 1;
                } else if (event[0] == 1) {
                    if (event[3] == halo::interface::k_event_argument_unset) {
                        halo::interface::widget_list_select_next(widget);
                        if (sound_effect == 0) sound_effect = 1;
                        list_nav_done = 1;
                    } else if (event[3] == halo::interface::k_action_none) {
                        halo::interface::widget_list_select_previous(widget);
                        if (sound_effect == 0) sound_effect = 1;
                        list_nav_done = 1;
                    }
                }
            }
        dpad_ud_nav:
            if (halo::interface::has_bit(tag->flags, halo::tags::ui_widget_definition_tag_flag::dpad_left_right_tabs_thru_list_items) && (widget->widget_type == uiwidgettype_spinner_list || widget->widget_type == uiwidgettype_column_list) &&
                list_nav_done == 0 && handled == 0) {
                if (event[0] == 3 && halo::interface::event_state(event) == 1) {
                    int8_t code = (int8_t)event[2];

                    if (code == '\n') {
                        halo::interface::widget_list_select_previous(widget);
                    } else if (code == '\v') {
                        halo::interface::widget_list_select_next(widget);
                    } else {
                        goto dispatch_to_children;
                    }
                    if (sound_effect == 0) sound_effect = 1;
                    list_nav_done = 1;
                } else if (event[0] == 1) {
                    if (event[2] == halo::interface::k_event_argument_unset) {
                        halo::interface::widget_list_select_previous(widget);
                        if (sound_effect == 0) sound_effect = 1;
                        list_nav_done = 1;
                    } else if (event[2] == halo::interface::k_action_none) {
                        halo::interface::widget_list_select_next(widget);
                        if (sound_effect == 0) sound_effect = 1;
                        list_nav_done = 1;
                    }
                }
            }
            goto dispatch_to_children;
        }
    }
    goto dispatch_to_children;

dispatch_to_children:

    if (controller_matches && tag->event_handlers.count > 0) {
        EventHandlerReference *entry_base = halo::interface::reflexive_elements<EventHandlerReference>(tag->event_handlers);

        handler_scan_count = 0;
        while (handler_scan_count < tag->event_handlers.count) {
            if (handled != 0) {
                break;
            }
            {
                EventHandlerReference *entry = entry_base + handler_scan_count;
                int16_t event_type = entry->event_type;
                uint8_t match = 0;

                switch (event[0]) {
                case 1:
                    switch (event_type) {
                    case 0x10: match = (event[3] == halo::interface::k_action_none); break;
                    case 0x11: match = (event[3] == halo::interface::k_event_argument_unset); break;
                    case 0x12: match = (event[2] == halo::interface::k_event_argument_unset); break;
                    case 0x13: match = (event[2] == halo::interface::k_action_none); break;
                    default: goto scan_next;
                    }
                    break;
                case 2:
                    switch (event_type) {
                    case 0x14: match = (event[3] == halo::interface::k_action_none); break;
                    case 0x15: match = (event[3] == halo::interface::k_event_argument_unset); break;
                    case 0x16: match = (event[2] == halo::interface::k_event_argument_unset); break;
                    case 0x17: match = (event[2] == halo::interface::k_action_none); break;
                    default: goto scan_next;
                    }
                    break;
                case 3:
                    if (event_type == (uint16_t)(uint8_t)event[2]) {
                        match = (halo::interface::event_state(event) == 1);
                    }
                    break;
                case 4:
                    if (halo::interface::event_state(event) == 1 && halo::interface::widget_instance_point_in_bounds(widget) != 0) {
                        switch (event_type) {
                        case 0x1c: match = ((int8_t)event[2] == 0); break;
                        case 0x1d: match = ((int8_t)event[2] == 1); break;
                        case 0x1e: match = ((int8_t)event[2] == 2); break;
                        case 0x1f: match = ((int8_t)event[2] == 3); break;
                        default: goto scan_next;
                        }
                    }
                    break;
                case 5:
                    match = (event_type == 0x20);
                    break;
                default:
                    break;
                }
                if (match) {
                    list_nav_done = 1;
                    halo::interface::ui_widget_list_item_activate(widget, tag, event, entry, &handled);
                }
            }
        scan_next:
            handler_scan_count = handler_scan_count + 1;
        }
    }

    {
        uint32_t flags = tag->flags;

        if ((has_bit(flags, widget_flag::pass_handled_events_to_all_children) || list_nav_done == 0) &&
            (has_bit(flags, widget_flag::pass_unhandled_events_to_focused_child) || has_bit(flags, widget_flag::pass_unhandled_events_to_all_children)) &&
            handled == 0) {
            if (!has_bit(flags, widget_flag::pass_unhandled_events_to_all_children)) {
                widget_instance *child = widget->focused_child;

                if (child != (widget_instance *)0 &&
                    (child->controller_index == -1 || child->controller_index == event[1])) {
                    UIWidgetDefinition *child_tag =
                        halo::interface::tag_data<UIWidgetDefinition>(child->definition);

                    halo::interface::widget_instance_handle_input_event(child, child_tag, event, &handled);
                }
            } else {
                widget_instance *child = widget->first_child;

                while (child != (widget_instance *)0) {
                    if (child->controller_index == -1 || child->controller_index == event[1]) {
                        UIWidgetDefinition *child_tag =
                            halo::interface::tag_data<UIWidgetDefinition>(child->definition);

                        halo::interface::widget_instance_handle_input_event(child, child_tag, event, &handled);
                        if (handled == 1) {
                            break;
                        }
                    }
                    child = child->next_sibling;
                }
            }
        }
    }

    if (handled == 1 && halo::interface::has_bit(tag->flags, halo::tags::ui_widget_definition_tag_flag::return_to_main_menu_if_no_history)) {
        int32_t i;

        for (i = 0; i < 1; i++) {
            if (ui_root_widget[i] != (widget_instance *)0) {
                break;
            }
        }
        if (i == 1) {
            split_screen_quit_prompt_string = halo::k_word_none;
            halo::networking::globals().join_error_reason = 0;
            split_screen_quit_prompt_armed = 1;
        }
    }

    halo::interface::widget_play_sound_effect(sound_effect);
    *out_handled = handled;
}

/**
 * blam-cc: EAX -> widget
 *
 * @address 0x499c40
 */
uint8_t WidgetView::is_input_eligible()
{
    widget_instance *cursor;
    uint8_t result;
    widget_instance *tag_source;
    void *tag_data;

    if (widget->hidden != 0) {
        return 0;
    }
    cursor = widget->parent;
    if (cursor == (widget_instance *)0) {
        return 1;
    }
    result = 1;
    tag_source = cursor;
    tag_data = halo::interface::tag_data<void>(cursor->definition);
    while (cursor != (widget_instance *)0 && result != 0) {
        UIWidgetDefinition *definition = (UIWidgetDefinition *)tag_data;

        tag_source = cursor;
        if (!halo::interface::has_bit(definition->flags, halo::tags::ui_widget_definition_tag_flag::pass_unhandled_events_to_focused_child) && cursor->widget_type != uiwidgettype_spinner_list && cursor->widget_type != uiwidgettype_column_list) {
            result = 0;
        } else {
            result = 1;
        }
        cursor = cursor->parent;
        tag_data = halo::interface::tag_data<void>(tag_source->definition);
    }
    return result;
}

/**
 * @address 0x499cb0
 */
uint8_t WidgetView::is_top_of_stack()
{
    widget_instance *cursor = widget->parent;
    uint8_t is_focused;
    widget_instance *ancestor;

    if (cursor == (widget_instance *)0) {
        return 1;
    }
    is_focused = (cursor->focused_child == widget);
    if (is_focused) {
        return 1;
    }
    do {
        ancestor = cursor->parent;
        if (ancestor != (widget_instance *)0) {
            uint8_t is_list;

            if (ancestor->focused_child != cursor) {
                return 0;
            }
            is_list = (ancestor->widget_type == uiwidgettype_spinner_list || ancestor->widget_type == uiwidgettype_column_list);
            is_focused = is_focused | is_list;
        }
        cursor = ancestor;
    } while (ancestor != (widget_instance *)0);
    return is_focused;
}

/**
 * blam-cc: EAX -> widget, ECX -> child Relinks `child` (or, if it is hidden, the closest eligible sibling --
 * preferring forward, then falling back to scanning backward when the forward scan only rediscovers the
 * parent's current focus) as the focused_child all the way up the tree rooted at the topmost ancestor of
 * `widget`. If the previous focus chain shared child's immediate parent, only that one link is rewritten;
 *
 * @address 0x49bba0
 */
void WidgetView::relink_focus(widget_instance *child)
{
    widget_instance *root = widget;
    widget_instance *old_focus;
    widget_instance *cursor;

    while (root->parent != (widget_instance *)0) {
        root = root->parent;
    }
    old_focus = root->focused_child;

    if (child->hidden != 0) {
        widget_instance *found = (widget_instance *)0;

        for (cursor = child->next_sibling; cursor != (widget_instance *)0; cursor = cursor->next_sibling) {
            if (widget_is_focus_candidate(cursor)) {
                found = cursor;
                break;
            }
        }
        if (found == (widget_instance *)0 && child->parent != (widget_instance *)0) {
            widget_instance *parent = child->parent;

            for (cursor = parent->first_child;
                 cursor != (widget_instance *)0 && !widget_is_focus_candidate(cursor);
                 cursor = cursor->next_sibling) {
            }
            if (cursor == parent->focused_child) {
                for (cursor = child->previous_sibling; cursor != (widget_instance *)0;
                     cursor = cursor->previous_sibling) {
                    if (widget_is_focus_candidate(cursor)) {
                        found = cursor;
                        break;
                    }
                }
            } else {
                found = cursor;
            }
        }
        if (found != (widget_instance *)0) {
            child = found;
        }
    }

    if (old_focus != (widget_instance *)0) {
        if (child != (widget_instance *)0 && child->parent != (widget_instance *)0 &&
            old_focus->parent == child->parent) {
            child->parent->focused_child = child;
            return;
        }
        while (old_focus != (widget_instance *)0) {
            widget_instance *next = old_focus->focused_child;
            old_focus->parent->focused_child = (widget_instance *)0;
            old_focus = next;
        }
    }

    while (child->parent != (widget_instance *)0) {
        child->parent->focused_child = child;
        child = child->parent;
    }
}

/**
 * Recursively renders a widget instance and all of its children: applies inherited scale/fade, runs the tag's
 * per-frame game-data-input bindings, draws its background bitmap (if any) and per-type content, recurses into
 * every child (tracking whether each is the focused one), then fires any bound "post_render" event handler.
 *
 * @address 0x49a8c0
 */
void WidgetRender::render(Rectangle2D *dest, int32_t offset_xy, uint32_t flag1, int32_t flag2)
{
    UIWidgetDefinition *tag = halo::interface::tag_data<UIWidgetDefinition>(widget->definition);
    float scale = widget->scale;
    widget_instance *ancestor;
    int32_t i;

    for (ancestor = widget->parent; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
        scale = scale * ancestor->scale;
    }

    if ((int8_t)flag2 == 0 && halo::interface::has_bit(tag->flags, halo::tags::ui_widget_definition_tag_flag::always_use_nifty_render_fx)) {
        flag2 = (flag2 & ~0xff) | 1;
    }

    offset_xy = (int32_t)(((int16_t)(offset_xy >> 16) + widget->local_y) << 16) |
                (uint16_t)((int16_t)offset_xy + widget->local_x);

    if (tag->game_data_inputs.count > 0) {
        GameDataInputReference *entry = halo::interface::reflexive_elements<GameDataInputReference>(tag->game_data_inputs);

        for (i = 0; i < tag->game_data_inputs.count; i++, entry++) {
            int16_t function_id = entry->function;

            if (function_id >= 0 && function_id < 0x3b) {
                ((ui_game_data_input_function)game_data_input_function_table[function_id])(widget);
            }
        }
    }

    if (widget->state != 0) {
        BitmapData *bitmap_data = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(
            halo::interface::tag_handle(((struct UIWidgetDefinition *)tag)->background_bitmap.tag_id), 0, widget->background_bitmap_frame);

        if (bitmap_data != 0) {
            float alpha = scale;
            int16_t x = (int16_t)offset_xy;
            int16_t y = (int16_t)(offset_xy >> 16);
            Rectangle2D bounds = tag->bounds;
            Rectangle2D clip;
            Rectangle2D *clip_arg = (Rectangle2D *)0;

            if ((int8_t)flag2 != 0) {
                override_color_00879f40 = 0.0f;
                override_color_00879f44 = 0.05f;
                override_color_00879f48 = 0.05f;
                override_color_00879f4c = 0.05f;
            }
            bounds.top = (int16_t)(bounds.top + y);
            bounds.left = (int16_t)(bounds.left + x);
            bounds.bottom = (int16_t)(bounds.bottom + y);
            bounds.right = (int16_t)(bounds.right + x);
            if (dest != (Rectangle2D *)0) {
                clip.top = (int16_t)(dest->top + y);
                clip.left = (int16_t)(dest->left + x);
                clip.bottom = (int16_t)(dest->bottom + y);
                clip.right = (int16_t)(dest->right + x);
                clip_arg = &clip;
            }
            if (halo::interface::has_bit(tag->flags, halo::tags::ui_widget_definition_tag_flag::flash_background_bitmap)) {
                double t = (double)ui_time_milliseconds;

                if (ui_time_milliseconds < 0) t += 4294967296.0;
                alpha = (float)((halo::libm::cos(t * 0.003) + 1.0) * 0.5 * (double)alpha);
            }

            halo::interface::ui_draw_screen_quad(&bounds, &bounds, bitmap_data, clip_arg,
                                 (uint32_t)((int32_t)(alpha * 255.0f + 0.5f) << 24) | halo::interface::k_rgb_mask);
            if ((int8_t)flag2 != 0) {
                override_color_00879f40 = 0.0f;
                override_color_00879f44 = 0.0f;
                override_color_00879f48 = 0.0f;
                override_color_00879f4c = 0.0f;
            }
        }
    }

    if (widget->widget_type == uiwidgettype_text_box) {
        uint32_t use_flag1 = flag1;

        if (!halo::interface::has_bit(tag->flags_1, halo::tags::ui_widget_definition_flags1_tag_flag::don_t_do_that_weird_focus_test)) {
            use_flag1 = halo::interface::widget_instance_is_top_of_stack(widget);
        }
        halo::interface::widget_instance_render_text_box(widget, tag, dest, offset_xy, use_flag1 & 0xff);
    } else if (widget->widget_type == uiwidgettype_spinner_list) {
        halo::interface::widget_instance_render_list_head(widget, tag, dest, offset_xy, flag1);
        if (halo::interface::has_bit(tag->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_items_from_string_list_tag) && tag->child_widgets.count == 0) {
            goto post_render;
        }
    } else if (widget->widget_type == uiwidgettype_column_list) {
        halo::interface::widget_instance_render_column_list_items(widget, tag, dest, offset_xy, flag1);
        if (halo::interface::has_bit(tag->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_items_generated_in_code)) {
            goto post_render;
        }
    }

    {
        widget_instance *child;

        for (child = widget->first_child; child != (widget_instance *)0; child = child->next_sibling) {
            uint32_t child_flag1 = (flag1 & ~0xffu) | (child == widget->focused_child);
            int32_t child_flag2;

            if (child == widget->focused_child && (widget->widget_type == uiwidgettype_spinner_list || widget->widget_type == uiwidgettype_column_list)) {
                child_flag2 = (flag2 & ~0xff) | 1;
            } else {
                child_flag2 = (flag2 >> 8) << 8;
            }
            halo::interface::widget_instance_render(child, dest, offset_xy, child_flag1, child_flag2);
        }
    }

post_render:
    if (tag->event_handlers.count > 0) {
        EventHandlerReference *entry = halo::interface::reflexive_elements<EventHandlerReference>(tag->event_handlers);

        for (i = 0; i < tag->event_handlers.count; i++, entry++) {
            if (entry->event_type == 0x21) {
                int16_t event[4] = {0, 0, 0, 0};
                uint8_t handled;

                event[1] = widget->controller_index;
                halo::interface::ui_widget_list_item_activate(widget, tag, event, entry, &handled);
            }
        }
    }
}

/**
 * blam-cc: EDI -> widget, stack -> tag, dest, offset_xy, flags Propagates the widget's cumulative scale to its
 * extended_description widget and renders it, then -- only when the tag marks the list's items as
 * code-generated -- renders every child of `widget` in order, flagging the one at selection_index as selected.
 * Always clears scroll_blink on the way out.
 *
 * @address 0x49bac0
 */
void WidgetRender::render_column_list_items(UIWidgetDefinition *tag, Rectangle2D *dest, int32_t offset_xy, uint32_t flags)
{
    widget_instance *child;
    int16_t index;

    if (widget->extended_description != (widget_instance *)0) {
        float scale = widget->scale;
        widget_instance *ancestor;

        for (ancestor = widget->parent; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
            scale = scale * ancestor->scale;
        }
        widget->extended_description->scale = scale;
        halo::interface::widget_instance_render(widget->extended_description, dest, offset_xy, 0, 1);
    }

    if (!halo::interface::has_bit(tag->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_items_generated_in_code)) {
        widget->scroll_blink = 0;
        return;
    }

    child = widget->first_child;
    if (child == (widget_instance *)0) {
        widget->scroll_blink = 0;
        return;
    }
    index = 0;
    do {
        if ((uint16_t)widget->item_count <= (uint16_t)index) {
            break;
        }
        halo::interface::widget_instance_render(child, dest, offset_xy, flags, index == widget->selection_index);
        child = child->next_sibling;
        index = index + 1;
    } while (child != (widget_instance *)0);
    widget->scroll_blink = 0;
}

/**
 * blam-cc: EAX -> list_definition, EBX -> widget, stack -> selection Finds `widget`'s descendant tagged
 * `list_definition`. With a negative `selection`, re-focuses that descendant if it is still input-eligible and
 * is not itself a multi-item spinner_list.
 *
 * @address 0x49bd00
 */
void WidgetList::select_list_index(datum_index list_definition, int32_t selection)
{
    widget_instance *target;

    if (list_definition == (datum_index)-1) {
        return;
    }
    target = halo::interface::widget_find_by_tag_id(widget, list_definition);
    if (target == (widget_instance *)0) {
        return;
    }

    if ((int16_t)selection < 0) {
        if (halo::interface::widget_instance_is_input_eligible(target) != 0) {
            UIWidgetDefinition *tag = halo::interface::tag_data<UIWidgetDefinition>(target->definition);

            if (target->widget_type != uiwidgettype_spinner_list || tag->child_widgets.count < 2) {
                halo::interface::widget_instance_relink_focus(target, target);
            }
        }
    } else {
        widget_instance *cursor = target->first_child;
        int16_t index = 0;

        for (;;) {
            UIWidgetDefinition *tag;

            if (cursor == (widget_instance *)0) {
                return;
            }
            tag = halo::interface::tag_data<UIWidgetDefinition>(cursor->definition);
            if (cursor->widget_type == uiwidgettype_spinner_list && tag->child_widgets.count > 1) {
                return;
            }
            if (index == (int16_t)selection) {
                break;
            }
            cursor = cursor->next_sibling;
            index = index + 1;
        }
        halo::interface::widget_instance_relink_focus(cursor, cursor);
        if (cursor->parent != (widget_instance *)0 &&
            (cursor->parent->widget_type == uiwidgettype_spinner_list || cursor->parent->widget_type == uiwidgettype_column_list)) {
            cursor->parent->selection_index = index;
        }
    }
}

/**
 * @address 0x498e60
 */
void WidgetView::set_state_recursive(uint8_t state)
{
    widget_instance *child = widget->first_child;

    widget->state = state;
    for (; child != (widget_instance *)0; child = child->next_sibling) {
        halo::interface::widget_instance_set_state_recursive(child, state);
    }
}

/**
 * @address 0x499aa0
 */
uint8_t WidgetView::verify_stack_chain(widget_instance *node)
{
    widget_instance *ancestor;

    if (node == (widget_instance *)0) {
        return 0;
    }
    ancestor = node->parent;
    while (ancestor != (widget_instance *)0) {
        if (ancestor->focused_child != node) {
            return 0;
        }
        node = ancestor;
        ancestor = node->parent;
    }
    return 1;
}

/**
 * @address 0x498630
 */
widget_instance * WidgetList::get_child_by_index(widget_instance *list, int32_t index)
{
    widget_instance *child = list->first_child;
    int32_t i = 0;

    if (index > 0) {
        do {
            if (child == (widget_instance *)0) {
                return (widget_instance *)0;
            }
            child = child->next_sibling;
            i = i + 1;
        } while (i < index);
    }
    return child;
}

/**
 * @address 0x4a7400
 */
void WidgetList::scroll_window(int32_t out[3], widget_instance *widget)
{
    int32_t item_count = (uint16_t)widget->item_count;
    int16_t selection = widget->selection_index;

    if (widget->focused_child == widget->first_child) {
        out[0] = selection;
        out[1] = selection + 1;
        if (out[1] == item_count) {
            out[1] = 0;
        }
    } else {
        if (widget->focused_child != widget->first_child->next_sibling) {
            out[2] = selection;
            out[1] = selection - 1;
            if (out[1] < 0) out[1] = item_count - 1;
            out[0] = out[1] - 1;
            if (out[0] < 0) out[0] = item_count - 1;
            goto clamp;
        }
        out[1] = selection;
        out[0] = selection - 1;
        if (out[0] < 0) out[0] = item_count - 1;
    }
    out[2] = out[1] + 1;
    if (out[2] == item_count) {
        out[2] = 0;
    }
clamp:
    if (item_count <= out[0]) out[0] = -1;
    if (item_count <= out[1]) out[1] = -1;
    if (item_count <= out[2]) out[2] = -1;
}

/**
 * Moves a list or scrollable widget's current selection to the next item, wrapping around at the end; for
 * column_list, walks the focused child's sibling chain and reopens the new child; for spinner_list, reselects
 * a child by computed index when the tag allows scroll-window paging; otherwise, if a generated list_items
 * array is present, just advances the stored index.
 *
 * @address 0x4986b0
 */
uint8_t WidgetList::select_next()
{
    UIWidgetDefinition *tag = halo::interface::tag_data<UIWidgetDefinition>(widget->definition);

    if (widget->list_items != nullptr && widget->item_count != 0) {
        int16_t next_index = widget->selection_index + 1;

        if ((int32_t)(uint16_t)widget->item_count <= next_index) {
            next_index = 0;
        }
        if (widget->widget_type == uiwidgettype_column_list) {
            widget_instance *child = halo::interface::widget_list_get_child_by_index(widget, next_index);

            if (child == (widget_instance *)0) {
                return 0;
            }
            halo::interface::widget_relink_focus_by_tag_id(widget, child->definition);
        } else if (widget->widget_type == uiwidgettype_spinner_list) {
            if (tag->child_widgets.count > 1) {
                widget_instance *focused = widget->focused_child;

                if ((focused == widget->first_child || focused == widget->first_child->next_sibling) &&
                    focused->next_sibling != (widget_instance *)0) {
                    halo::interface::widget_instance_relink_focus(widget, focused->next_sibling);
                    widget->selection_index = next_index;
                    widget->selection_direction = 1;
                    widget->scroll_blink = halo::interface::k_scroll_blink_long;
                    return 1;
                }
            }
        } else {
            goto commit;
        }
        widget->selection_index = next_index;
        goto commit;
    }

    if (widget->widget_type != uiwidgettype_spinner_list || !halo::interface::has_bit(tag->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_items_from_string_list_tag) || tag->child_widgets.count != 0) {
        widget_instance *child;

        if ((widget->focused_child == (widget_instance *)0 ||
             (child = widget->focused_child->next_sibling,
              (int32_t)(widget->selection_index + 1) == (uint16_t)widget->item_count) ||
             (child = widget->focused_child->next_sibling, child == (widget_instance *)0)) &&
            (child = widget->first_child, child == (widget_instance *)0)) {
            return 0;
        }
        {
            widget_instance *cursor = widget->first_child;
            int16_t index = 0;

            halo::interface::widget_relink_focus_by_tag_id(widget, child->definition);
            if (cursor != (widget_instance *)0) {
                index = 0;
                do {
                    if (cursor == widget->focused_child) {
                        break;
                    }
                    cursor = cursor->next_sibling;
                    index = index + 1;
                } while (cursor != (widget_instance *)0);
            }
            widget->selection_direction = 1;
            widget->selection_index = index;
            widget->scroll_blink = halo::interface::k_scroll_blink_long;
            return 1;
        }
    }

    widget->selection_index = widget->selection_index + 1;
    if ((uint16_t)widget->selection_index == (uint16_t)widget->item_count) {
        widget->selection_direction = 1;
        widget->selection_index = 0;
        widget->scroll_blink = halo::interface::k_scroll_blink_long;
        return 1;
    }

commit:
    widget->selection_direction = 1;
    widget->scroll_blink = halo::interface::k_scroll_blink_long;
    return 1;
}

/**
 * Moves a list or scrollable widget's current selection to the previous item, wrapping around at the start;
 * column_list and spinner_list use the same index-based reselection as widget_list_select_next, while the
 * plain case searches previous_sibling for the nearest eligible (non-hidden, event-bearing or list-type)
 * child, wrapping to the last child.
 *
 * @address 0x498820
 */
uint8_t WidgetList::select_previous()
{
    UIWidgetDefinition *tag = halo::interface::tag_data<UIWidgetDefinition>(widget->definition);

    if (widget->list_items != nullptr && widget->item_count != 0) {
        int16_t prev_index = widget->selection_index - 1;

        if (prev_index < 0) {
            prev_index = (int16_t)((uint16_t)widget->item_count - 1);
        }
        if (widget->widget_type == uiwidgettype_column_list) {
            widget_instance *child = halo::interface::widget_list_get_child_by_index(widget, prev_index);

            if (child == (widget_instance *)0) {
                return 0;
            }
            halo::interface::widget_relink_focus_by_tag_id(widget, child->definition);
            widget->selection_index = prev_index;
            widget->scroll_blink = -halo::interface::k_scroll_blink_long;
            widget->selection_direction = halo::k_word_none;
            return 1;
        }
        if (widget->widget_type == uiwidgettype_spinner_list) {
            if (tag->child_widgets.count > 1 && widget->focused_child != widget->first_child &&
                widget->focused_child != (widget_instance *)0 &&
                widget->focused_child->previous_sibling != (widget_instance *)0) {
                halo::interface::widget_instance_relink_focus(widget, widget->focused_child->previous_sibling);
            }
            widget->selection_index = prev_index;
            widget->scroll_blink = -halo::interface::k_scroll_blink_long;
            widget->selection_direction = halo::k_word_none;
            return 1;
        }
        goto commit;
    }

    if (widget->widget_type == uiwidgettype_spinner_list && halo::interface::has_bit(tag->flags_2, halo::tags::ui_widget_definition_flags2_tag_flag::list_items_from_string_list_tag) && tag->child_widgets.count == 0) {
        widget->selection_index = widget->selection_index - 1;
        if (widget->selection_index < 0) {
            widget->selection_index = widget->item_count - 1;
            widget->scroll_blink = -halo::interface::k_scroll_blink_long;
            widget->selection_direction = halo::k_word_none;
            return 1;
        }
        goto commit;
    }

    {
        widget_instance *cursor;
        int32_t index;

        if (widget->focused_child == (widget_instance *)0) {
            widget_instance *tail = widget->first_child;

            index = 0;
            for (cursor = (tail != (widget_instance *)0) ? tail->next_sibling : (widget_instance *)0;
                 cursor != (widget_instance *)0; cursor = cursor->next_sibling) {
                index = index + 1;
                tail = cursor;
            }
            cursor = tail;
        } else {
            cursor = widget->focused_child->previous_sibling;
            index = widget->selection_index - 1;
            if (cursor == (widget_instance *)0) {
                widget_instance *tail = widget->first_child;

                index = 0;
                {
                    widget_instance *w;

                    for (w = (tail != (widget_instance *)0) ? tail->next_sibling : (widget_instance *)0;
                         w != (widget_instance *)0; w = w->next_sibling) {
                        index = index + 1;
                        tail = w;
                    }
                }
                cursor = tail;
            }
        }

        if (index != widget->selection_index) {
            do {
                if (cursor->hidden == 0) {
                    UIWidgetDefinition *cursor_tag =
                        halo::interface::tag_data<UIWidgetDefinition>(cursor->definition);

                    if (cursor_tag->event_handlers.count > 0 || cursor->widget_type == uiwidgettype_spinner_list ||
                        cursor->widget_type == uiwidgettype_column_list) {
                        break;
                    }
                }
                index = index - 1;
                cursor = cursor->previous_sibling;
                if (index < 0) {
                    widget_instance *tail = widget->first_child;

                    index = 0;
                    {
                        widget_instance *w;

                        for (w = (tail != (widget_instance *)0) ? tail->next_sibling : (widget_instance *)0;
                             w != (widget_instance *)0; w = w->next_sibling) {
                            index = index + 1;
                            tail = w;
                        }
                    }
                    cursor = tail;
                }
            } while (index != widget->selection_index);
        }

        halo::interface::widget_relink_focus_by_tag_id(widget, cursor->definition);
        {
            widget_instance *walk = widget->first_child;
            int16_t found_index = 0;

            if (walk != (widget_instance *)0) {
                found_index = 0;
                do {
                    if (walk == widget->focused_child) {
                        break;
                    }
                    walk = walk->next_sibling;
                    found_index = found_index + 1;
                } while (walk != (widget_instance *)0);
            }
            widget->selection_index = found_index;
        }
    }

commit:
    widget->scroll_blink = -halo::interface::k_scroll_blink_long;
    widget->selection_direction = halo::k_word_none;
    return 1;
}

/**
 * blam-cc: AX -> effect_id Plays one of the standard UI sound effects (cursor move, forward, back, or
 * failure), selected by a one-based id; any other id is a silent no-op.
 *
 * @address 0x498e90
 */
void WidgetLifecycle::play_sound_effect(int16_t effect_id)
{
    datum_index sound_tag;

    switch (effect_id) {
    case 1:
        sound_tag = halo::interface::lookup_tag(halo::fourcc('s', 'n', 'd', '!'), "sound\\sfx\\ui\\cursor");
        halo::interface::widget_play_sound_effect_tag(sound_tag);
        return;
    case 2:
        sound_tag = halo::interface::lookup_tag(halo::fourcc('s', 'n', 'd', '!'), "sound\\sfx\\ui\\forward");
        halo::interface::widget_play_sound_effect_tag(sound_tag);
        return;
    case 3:
        sound_tag = halo::interface::lookup_tag(halo::fourcc('s', 'n', 'd', '!'), "sound\\sfx\\ui\\back");
        halo::interface::widget_play_sound_effect_tag(sound_tag);
        return;
    case 4:
        sound_tag = halo::interface::lookup_tag(halo::fourcc('s', 'n', 'd', '!'), "sound\\sfx\\ui\\flag_failure");
        halo::interface::widget_play_sound_effect_tag(sound_tag);
        return;
    default:
        return;
    }
}

/**
 * blam-cc: EAX -> sound_tag
 *
 * @address 0x49bdd0
 */
void WidgetLifecycle::play_sound_effect_tag(datum_index sound_tag)
{
    if (sound_tag != (datum_index)-1) {

        sound_location location;
        memset(&location, 0, sizeof(location));
        location.type = 0;
        location.scale = 1.0f;
        location.gain = 1.0f;
        halo::sound::sound_play_new(sound_tag, &location, -1, 0, 0, 0, 0);
    }
}

/**
 * blam-cc: EDI -> head Frees every node of a widget_history_node list, unlinking each node's own heap block as
 * it goes (the same manual sequence widget_close uses for the widget it closes).
 *
 * @address 0x4994b0
 */
void WidgetLifecycle::pool_list_free_all(widget_history_node **head)
{
    heap *pool = widget_memory_pool;
    widget_history_node *node = *head;

    while (node != (widget_history_node *)0) {
        heap_block *block = (heap_block *)((uint8_t *)node - 0x10);
        uint32_t size = block->size;
        int32_t slot = block->slot;

        *head = node->next;

        if (block->previous != (heap_block *)0) {
            block->previous->next = block->next;
        }
        if (block->next != (heap_block *)0) {
            block->next->previous = block->previous;
        }
        if (block == pool->first_block) {
            pool->first_block = block->next;
        }
        if (block == pool->last_block) {
            pool->last_block = block->previous;
        }
        pool->blocks[slot] = (heap_block *)0;
        pool->next_free_slot = (pool->first_block != (heap_block *)0) ? slot : 0;
        pool->bytes_allocated = pool->bytes_allocated - (int32_t)(size & halo::interface::k_pool_block_size_mask);
        pool->allocation_count = pool->allocation_count - 1;

        node = *head;
    }
}

/**
 * blam-cc: EAX -> widget, stack -> child_definition Climbs from `widget` to the root of its tree, then relinks
 * the root's descendant tagged `child_definition` (if any) into the focus chain.
 *
 * @address 0x49bb60
 */
void WidgetView::relink_focus_by_tag_id(datum_index child_definition)
{
    widget_instance *root = widget;
    widget_instance *found;

    while (root->parent != (widget_instance *)0) {
        root = root->parent;
    }
    found = halo::interface::widget_find_by_tag_id(root, child_definition);
    if (found != (widget_instance *)0) {
        halo::interface::widget_instance_relink_focus(found, found);
    }
}

/**
 * Reopens `open_tag` as a new root widget, with a go-back record aimed at restoring focus to `widget`
 * specifically (its tree root's definition, its parent's definition, and its own sibling index within that
 * parent).
 *
 * @address 0x49c4c0
 */
widget_instance * WidgetLifecycle::reopen_as_root_with_history(datum_index open_tag)
{
    UIWidgetDefinition *open_definition = halo::interface::tag_data<UIWidgetDefinition>(open_tag);
    uint16_t controller_index;
    widget_instance *ancestor;
    widget_instance *root;
    datum_index parent_definition;
    int16_t sibling_index;

    if (!halo::interface::has_bit(open_definition->flags, halo::tags::ui_widget_definition_tag_flag::always_use_tag_controller_index)) {
        switch (open_definition->controller_index) {
        case 0: controller_index = 0; break;
        case 1: controller_index = 1; break;
        case 2: controller_index = 2; break;
        case 3: controller_index = 3; break;
        case 4:
            controller_index = (uint16_t)widget->controller_index;
            break;
        default:
            controller_index = (uint16_t)(uintptr_t)widget;
            break;
        }
    } else {
        switch (open_definition->controller_index) {
        case 0: controller_index = 0; break;
        case 1: controller_index = 1; break;
        case 2: controller_index = 2; break;
        case 3: controller_index = 3; break;
        case 4: controller_index = (uint16_t)-1; break;
        default: controller_index = (uint16_t)(uintptr_t)widget; break;
        }
    }

    ancestor = widget->parent;
    if (ancestor == (widget_instance *)0) {
        parent_definition = (datum_index)-1;
        root = widget;
    } else {
        parent_definition = ancestor->definition;
        root = ancestor;
        while (root->parent != (widget_instance *)0) {
            root = root->parent;
        }
    }

    sibling_index = -1;
    if (widget->parent != (widget_instance *)0) {
        widget_instance *cursor;
        int16_t index = 0;

        sibling_index = -1;
        for (cursor = widget->parent->first_child; cursor != (widget_instance *)0;
             cursor = cursor->next_sibling) {
            if (cursor == widget) {
                sibling_index = index;
                break;
            }
            index = index + 1;
        }
    }

    return halo::interface::chimera__load_ui_widget(nullptr, open_tag, (widget_instance *)0, controller_index,
                             root->definition, parent_definition, sibling_index);
}

/**
 * @address 0x49c000
 */
void WidgetList::spinner_list_sync_selected(UIWidgetDefinition *tag)
{
    widget_instance *child;

    if (tag->child_widgets.count == 3 && widget->focused_child == (widget_instance *)0) {
        widget->focused_child = widget->first_child;
    }
    for (child = widget->first_child; child != (widget_instance *)0; child = child->next_sibling) {
        child->background_bitmap_frame = 0;
        if (child == widget->focused_child && child->background_bitmap_frames == 2) {
            child->background_bitmap_frame = 1;
        }
    }
}

/**
 * Recomputes a text-edit control's current length and clamps its cursor into [0, length] and its selection
 * anchor into [-1, length], collapsing the selection to "none" (-1) if it now coincides with the cursor.
 * Finally re-snaps the cursor, and the selection anchor if one remains, off any double-byte character boundary
 * they might now split.
 *
 * @address 0x44c780
 */
void TextEdit::clamp_selection()
{
    int32_t len;
    int16_t new_cursor;
    int16_t new_selection;

    len = (int32_t)strlen(state->text);

    if (state->cursor < 0) {
        new_cursor = 0;
    } else if (state->cursor <= (int16_t)len) {
        new_cursor = state->cursor;
    } else {
        new_cursor = (int16_t)len;
    }

    if (state->selection_anchor < -1) {
        new_selection = -1;
    } else if (state->selection_anchor <= (int16_t)len) {
        new_selection = state->selection_anchor;
    } else {
        new_selection = (int16_t)len;
    }

    state->cursor = new_cursor;
    state->selection_anchor = new_selection;
    if (new_cursor == new_selection) {
        state->selection_anchor = -1;
    }

    halo::text::text_clamp_byte_length_to_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
    if (state->selection_anchor != -1) {
        halo::text::text_clamp_byte_length_to_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->selection_anchor);
    }
}

/**
 * Re-clamps the control, then reports its selection as an ordered [start, end) pair (start is the lower of
 * cursor/selection_anchor, end the higher). Returns 0 and leaves *out_start/ *out_end untouched when there is
 * no active selection.
 *
 * @address 0x44c5e0
 */
uint32_t TextEdit::get_selection(int16_t *out_start, int16_t *out_end)
{
    int16_t start;
    int16_t end;

    halo::interface::widget_text_edit_clamp_selection(state);

    if (state->selection_anchor == -1) {
        return 0;
    }

    start = (state->cursor < state->selection_anchor) ? state->cursor : state->selection_anchor;
    *out_start = start;

    end = (state->selection_anchor <= state->cursor) ? state->cursor : state->selection_anchor;
    *out_end = end;

    return 1;
}

/**
 * With no active selection: inserts insert_str at the cursor (shifting the tail right), but only if the
 * resulting string still fits under maximum_length; the cursor advances past the inserted text. With an active
 * selection: unconditionally replaces the selected range with insert_str (no maximum_length check in this
 * path, matching the original), moves the cursor to the end of the inserted text, and clears the selection.
 *
 * @address 0x44c640
 */
void TextEdit::insert_string(char *insert_str)
{
    int16_t sel_start;
    int16_t sel_end;
    uint32_t has_selection;
    char *insertion_point;
    char *tail_source;
    int32_t insert_len;
    int32_t tail_len;

    has_selection = halo::interface::widget_text_edit_get_selection(state, &sel_start, &sel_end);

    if (!has_selection) {
        insert_len = (int32_t)strlen(insert_str);
        if ((int32_t)strlen(state->text) + insert_len < (int32_t)state->maximum_length) {
            insertion_point = state->text + state->cursor;
            tail_len = (int32_t)strlen(insertion_point);
            memmove(insertion_point + insert_len, insertion_point, (size_t)(tail_len + 1));
            while (*insert_str != '\0') {
                state->text[state->cursor] = *insert_str;
                state->cursor = state->cursor + 1;
                insert_str = insert_str + 1;
            }
        }
    } else {
        tail_source = state->text + sel_end;
        tail_len = (int32_t)strlen(tail_source);
        insert_len = (int32_t)strlen(insert_str);
        memmove(state->text + sel_start + insert_len, tail_source, (size_t)(tail_len + 1));
        state->cursor = sel_start;
        state->selection_anchor = -1;
        while (*insert_str != '\0') {
            state->text[state->cursor] = *insert_str;
            state->cursor = state->cursor + 1;
            insert_str = insert_str + 1;
        }
    }

    halo::text::text_clamp_byte_length_to_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
}

/**
 * Applies one buffered key event to a widget's text-edit state: Home/End (collapse an existing selection to
 * its near edge, or otherwise step the cursor one character backward/forward), Backspace/Delete (remove the
 * selection, or the one character before/at the cursor), and plain printable characters (replace the
 * selection, or insert at the cursor, honoring maximum_length).
 *
 * @address 0x44c290
 */
void TextEdit::process_key(ui_key_event *event)
{
    int16_t sel_start;
    int16_t sel_end;
    uint32_t has_selection;
    int16_t old_cursor;
    int16_t scratch_offset;
    char *src;
    char *dst;
    int32_t tail_len;

    halo::interface::widget_text_edit_clamp_selection(state);

    if (event->key_code != _ui_edit_key_backspace && event->key_code != _ui_edit_key_delete) {
        if (event->key_code == _ui_edit_key_left_arrow || event->key_code == _ui_edit_key_right_arrow) {
            has_selection = ((event->modifiers & 1) == 0) &&
                             halo::interface::widget_text_edit_get_selection(state, &sel_start, &sel_end);
            if (has_selection) {
                state->selection_anchor = -1;
                if (event->key_code != _ui_edit_key_left_arrow) {
                    state->cursor = sel_end;
                } else {
                    state->cursor = sel_start;
                }
                halo::text::text_clamp_byte_length_to_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
                return;
            }

            if ((event->modifiers & 1) != 0 && state->selection_anchor == -1) {
                state->selection_anchor = state->cursor;
            }
            if (event->key_code == _ui_edit_key_left_arrow) {
                if (state->cursor > 0) {
                    halo::text::dbcs_text::find_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
                }
            } else {
                if ((size_t)state->cursor < strlen(state->text)) {
                    halo::text::dbcs_text::get_next_character(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
                }
            }
            if (state->selection_anchor == state->cursor) {
                state->selection_anchor = -1;
            }
        } else if (event->character > 0x1f && event->character != 0xff) {
            has_selection = halo::interface::widget_text_edit_get_selection(state, &sel_start, &sel_end);
            if (has_selection) {
                src = state->text + sel_end;
                tail_len = (int32_t)strlen(src);
                memmove(state->text + sel_start + 1, src, (size_t)(tail_len + 1));
                state->cursor = sel_start;
                state->selection_anchor = -1;
                state->text[sel_start] = (char)event->character;
                state->cursor = state->cursor + 1;
                halo::text::text_clamp_byte_length_to_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
                return;
            }
            if ((int32_t)strlen(state->text) < (int32_t)state->maximum_length) {
                dst = state->text + state->cursor;
                tail_len = (int32_t)strlen(dst);
                memmove(dst + 1, dst, (size_t)(tail_len + 1));
                state->text[state->cursor] = (char)event->character;
                state->cursor = state->cursor + 1;
                halo::text::text_clamp_byte_length_to_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
                return;
            }
        }
        halo::text::text_clamp_byte_length_to_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
        return;
    }

    has_selection = halo::interface::widget_text_edit_get_selection(state, &sel_start, &sel_end);
    if (has_selection) {
        src = state->text + sel_end;
        tail_len = (int32_t)strlen(src);
        memmove(state->text + sel_start, src, (size_t)(tail_len + 1));
        state->cursor = sel_start;
        state->selection_anchor = -1;
        halo::text::text_clamp_byte_length_to_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
        return;
    }

    if (event->key_code == _ui_edit_key_backspace) {
        old_cursor = state->cursor;
        if (old_cursor < 1) {
            halo::text::text_clamp_byte_length_to_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
            return;
        }
        halo::text::dbcs_text::find_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
        src = state->text + old_cursor;
        tail_len = (int32_t)strlen(src);
        dst = state->text + state->cursor;
        memmove(dst, src, (size_t)(tail_len + 1));
    } else {

        if ((size_t)state->cursor >= strlen(state->text)) {
            halo::text::text_clamp_byte_length_to_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
            return;
        }
        scratch_offset = state->cursor;
        halo::text::dbcs_text::get_next_character(reinterpret_cast<uint8_t *>(state->text), &scratch_offset);
        src = state->text + scratch_offset;
        tail_len = (int32_t)strlen(src);
        dst = state->text + state->cursor;
        memmove(dst, src, (size_t)(tail_len + 1));
    }

    halo::text::text_clamp_byte_length_to_character_boundary(reinterpret_cast<uint8_t *>(state->text), &state->cursor);
}

/**
 * blam-cc: state in ESI (unaff_ESI) Re-clamps the control, then re-derives its cursor from the current string
 * length and drops any selection: called whenever the underlying string was replaced out from under the
 * editor.
 *
 * @address 0x44c5b0
 */
void TextEdit::reset_length()
{
    halo::interface::widget_text_edit_clamp_selection(state);

    state->cursor = (int16_t)strlen(state->text);
    state->selection_anchor = -1;
}

} // namespace halo::interface

namespace halo::interface {

void list_node_pop(widget_history_node *out, widget_history_node **head)
{
    halo::interface::WidgetLifecycle::pop(out, head);
}

void list_node_prepend(widget_history_node *template_record, widget_history_node **head)
{
    halo::interface::WidgetLifecycle::prepend(template_record, head);
}

void widget_close(widget_instance *widget)
{
    halo::interface::WidgetLifecycle(widget).close();
}

void widget_close_all(void)
{
    halo::interface::WidgetLifecycle::close_all();
}

void widget_column_list_sync_selected(widget_instance *widget)
{
    halo::interface::WidgetList(widget).column_list_sync_selected();
}

uint8_t widget_create_children_from_tag(widget_instance *widget, UIWidgetDefinition *tag)
{
    return halo::interface::WidgetLifecycle(widget).create_children_from_tag(tag);
}

int32_t widget_cursor_side_of_midpoint(widget_instance *widget)
{
    return halo::interface::WidgetView(widget).cursor_side_of_midpoint();
}

uint32_t widget_cyclable_list_nudge(widget_instance *widget)
{
    return halo::interface::WidgetList(widget).cyclable_list_nudge();
}

void widget_draw_fullscreen_region(int16_t controller_index)
{
    halo::interface::WidgetRender::draw_fullscreen_region(controller_index);
}

void widget_draw_split_screen_region(Rectangle2D *viewport, int16_t controller_index)
{
    halo::interface::WidgetRender::draw_split_screen_region(viewport, controller_index);
}

void widget_extended_description_sync_selection(widget_instance *widget)
{
    halo::interface::WidgetList(widget).extended_description_sync_selection();
}

widget_instance * widget_find_by_tag_id(widget_instance *widget, datum_index tag_id)
{
    return halo::interface::WidgetView(widget).find_by_tag_id(tag_id);
}

void widget_focus_next_child(widget_instance *widget)
{
    halo::interface::WidgetView(widget).focus_next_child();
}

void widget_focus_previous_child(widget_instance *widget)
{
    halo::interface::WidgetView(widget).focus_previous_child();
}

int32_t widget_get_sibling_index(widget_instance *widget)
{
    return halo::interface::WidgetView(widget).get_sibling_index();
}

void widget_initialize_from_tag(widget_instance *widget, datum_index tag_index, widget_instance *parent, uint16_t controller_index, UIWidgetDefinition *tag)
{
    halo::interface::WidgetLifecycle(widget).initialize_from_tag(tag_index, parent, controller_index, tag);
}

void widget_instance_close_and_restore_previous(widget_instance *widget)
{
    halo::interface::WidgetLifecycle(widget).close_and_restore_previous();
}

widget_instance * widget_instance_find_root(widget_instance *widget)
{
    return halo::interface::WidgetView(widget).find_root();
}

float widget_instance_get_cumulative_scale(widget_instance *widget)
{
    return halo::interface::WidgetView(widget).get_cumulative_scale();
}

void widget_instance_handle_input_event(widget_instance *widget, UIWidgetDefinition *tag, int16_t *event, uint8_t *out_handled)
{
    halo::interface::WidgetView(widget).handle_input_event(tag, event, out_handled);
}

uint8_t widget_instance_is_input_eligible(widget_instance *widget)
{
    return halo::interface::WidgetView(widget).is_input_eligible();
}

uint8_t widget_instance_is_top_of_stack(widget_instance *widget)
{
    return halo::interface::WidgetView(widget).is_top_of_stack();
}

void widget_instance_relink_focus(widget_instance *widget, widget_instance *child)
{
    halo::interface::WidgetView(widget).relink_focus(child);
}

void widget_instance_render(widget_instance *widget, Rectangle2D *dest, int32_t offset_xy, uint32_t flag1, int32_t flag2)
{
    halo::interface::WidgetRender(widget).render(dest, offset_xy, flag1, flag2);
}

void widget_instance_render_column_list_items(widget_instance *widget, UIWidgetDefinition *tag, Rectangle2D *dest, int32_t offset_xy, uint32_t flags)
{
    halo::interface::WidgetRender(widget).render_column_list_items(tag, dest, offset_xy, flags);
}

void widget_instance_select_list_index(widget_instance *widget, datum_index list_definition, int32_t selection)
{
    halo::interface::WidgetList(widget).select_list_index(list_definition, selection);
}

void widget_instance_set_state_recursive(widget_instance *widget, uint8_t state)
{
    halo::interface::WidgetView(widget).set_state_recursive(state);
}

uint8_t widget_instance_verify_stack_chain(widget_instance *node)
{
    return halo::interface::WidgetView::verify_stack_chain(node);
}

widget_instance * widget_list_get_child_by_index(widget_instance *list, int32_t index)
{
    return halo::interface::WidgetList::get_child_by_index(list, index);
}

void widget_list_scroll_window(int32_t out[3], widget_instance *widget)
{
    halo::interface::WidgetList::scroll_window(out, widget);
}

uint8_t widget_list_select_next(widget_instance *widget)
{
    return halo::interface::WidgetList(widget).select_next();
}

uint8_t widget_list_select_previous(widget_instance *widget)
{
    return halo::interface::WidgetList(widget).select_previous();
}

void widget_play_sound_effect(int16_t effect_id)
{
    halo::interface::WidgetLifecycle::play_sound_effect(effect_id);
}

void widget_play_sound_effect_tag(datum_index sound_tag)
{
    halo::interface::WidgetLifecycle::play_sound_effect_tag(sound_tag);
}

void widget_pool_list_free_all(widget_history_node **head)
{
    halo::interface::WidgetLifecycle::pool_list_free_all(head);
}

void widget_relink_focus_by_tag_id(widget_instance *widget, datum_index child_definition)
{
    halo::interface::WidgetView(widget).relink_focus_by_tag_id(child_definition);
}

widget_instance * widget_reopen_as_root_with_history(widget_instance *widget, datum_index open_tag)
{
    return halo::interface::WidgetLifecycle(widget).reopen_as_root_with_history(open_tag);
}

void widget_spinner_list_sync_selected(widget_instance *widget, UIWidgetDefinition *tag)
{
    halo::interface::WidgetList(widget).spinner_list_sync_selected(tag);
}

void widget_text_edit_clamp_selection(text_edit_state *state)
{
    halo::interface::TextEdit(state).clamp_selection();
}

uint32_t widget_text_edit_get_selection(text_edit_state *state, int16_t *out_start, int16_t *out_end)
{
    return halo::interface::TextEdit(state).get_selection(out_start, out_end);
}

void widget_text_edit_insert_string(text_edit_state *state, char *insert_str)
{
    halo::interface::TextEdit(state).insert_string(insert_str);
}

void widget_text_edit_process_key(text_edit_state *state, ui_key_event *event)
{
    halo::interface::TextEdit(state).process_key(event);
}

void widget_text_edit_reset_length(text_edit_state *state)
{
    halo::interface::TextEdit(state).reset_length();
}

}
