#include "halo/interface/ifr2_widgets.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/text/api.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/memory/api.hpp"
#include <wchar.h>
#include "halo/cache/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/text/text.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/constants.hpp"
#include "halo/interface/flags.hpp"
#include "halo/core/datum.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern int32_t ui_cursor_x;
extern int32_t ui_cursor_y;
extern double cos(double x);
extern double sin(double x);
extern int32_t ui_time_milliseconds;
extern heap *widget_memory_pool;
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0, int32_t unknown_1, const uint16_t *text);
}

static_assert(offsetof(UIWidgetDefinition, list_header_bitmap.tag_id) == 0x160 && offsetof(UIWidgetDefinition, list_footer_bitmap.tag_id) == 0x170 && offsetof(UIWidgetDefinition, header_bounds) == 0x174 && offsetof(UIWidgetDefinition, footer_bounds) == 0x17c && offsetof(UIWidgetDefinition, flags_1) == 0x11e, "UIWidgetDefinition list head layout");

namespace halo::interface {

/**
 * Recursively searches a widget instance and its descendants for the topmost widget whose bounding rectangle
 * contains the given screen point (offset_x/offset_y is the accumulated local_x/local_y of every ancestor
 * above `widget`), honoring visibility and list-type widget rules.
 *
 * @address 0x499ad0
 */
widget_instance * WidgetView::find_at_point(int32_t cursor_x, int32_t cursor_y, int32_t offset_xy)
{
    UIWidgetDefinition *tag = halo::interface::tag_data<UIWidgetDefinition>(widget->definition);
    widget_instance *first_child = widget->first_child;
    uint8_t eligible =
        (widget->hidden == 0 &&
         (tag->event_handlers.count > 0 || widget->widget_type == uiwidgettype_spinner_list || widget->widget_type == uiwidgettype_column_list)) ||
        (first_child == (widget_instance *)0 || first_child->widget_type == uiwidgettype_spinner_list ||
         first_child->widget_type == uiwidgettype_column_list) ||
        ((int8_t)((uint32_t)tag->flags >> 8) < 0);
    widget_instance *result = (widget_instance *)0;

    if (!eligible || widget->hidden == 1) {
        return (widget_instance *)0;
    }

    {
        int16_t off_x = (int16_t)offset_xy + widget->local_x;
        int16_t off_y = (int16_t)(offset_xy >> 16) + widget->local_y;
        Rectangle2D rect = tag->bounds;

        halo::interface::widget_list_adjust_rect_for_scroll_arrows(widget, &rect);

        if ((int16_t)(rect.left + off_x) <= cursor_x && cursor_x <= (int16_t)(rect.right + off_x) &&
            (int16_t)(rect.top + off_y) <= cursor_y && cursor_y <= (int16_t)(rect.bottom + off_y)) {
            widget_instance *child = first_child;
            int32_t child_offset = ((int32_t)off_y << 16) | (uint16_t)off_x;

            while (child != (widget_instance *)0 && result == (widget_instance *)0) {
                result = halo::interface::widget_instance_find_at_point(child, cursor_x, cursor_y, child_offset);
                child = child->next_sibling;
            }
            if (widget->widget_type == uiwidgettype_column_list || widget->widget_type == uiwidgettype_spinner_list) {
                widget = (widget_instance *)0;
            }
            if (result == (widget_instance *)0) {
                result = widget;
            }
            return result;
        }
        return (widget_instance *)0;
    }
}

/**
 * blam-cc: EAX -> widget Tests whether the current UI cursor position falls within `widget`'s on-screen
 * bounding rectangle (its tag's bounds, adjusted for spinner_list scroll arrows), offset by the sum of
 * local_x/local_y over the widget and every one of its ancestors.
 *
 * @address 0x4999f0
 */
uint8_t WidgetView::point_in_bounds()
{
    UIWidgetDefinition *tag = halo::interface::tag_data<UIWidgetDefinition>(widget->definition);
    Rectangle2D rect = tag->bounds;
    int16_t x_sum = 0;
    int16_t y_sum = 0;
    widget_instance *cursor = widget;

    do {
        x_sum = x_sum + cursor->local_x;
        y_sum = y_sum + cursor->local_y;
        cursor = cursor->parent;
    } while (cursor != (widget_instance *)0);

    halo::interface::widget_list_adjust_rect_for_scroll_arrows(widget, &rect);

    if ((int16_t)(rect.left + x_sum) <= ui_cursor_x && ui_cursor_x <= (int16_t)(rect.right + x_sum) &&
        (int16_t)(rect.top + y_sum) <= ui_cursor_y && ui_cursor_y <= (int16_t)(rect.bottom + y_sum)) {
        return 1;
    }
    return 0;
}

/**
 * Renders a list_head-type widget: draws its scroll-arrow indicators (only when the cursor is over it and the
 * corresponding bitmap has 4 frames), draws its extended_description child, then assembles and draws the
 * caption text of the currently selected list entry.
 *
 * @address 0x49b560
 */
void WidgetRender::render_list_head(UIWidgetDefinition *tag, Rectangle2D *dest, int32_t offset_xy, uint8_t is_top_of_stack)
{
    uint8_t *t = (uint8_t *)tag;
    float scale = widget->scale;
    widget_instance *ancestor;
    uint8_t in_bounds;
    int32_t cursor_side;
    uint16_t *text = nullptr;
    uint8_t scroll_dir_up = 0;
    uint8_t scroll_dir_down = 0;
    int16_t x_off = (int16_t)offset_xy;
    int16_t y_off = (int16_t)(offset_xy >> 16);

    for (ancestor = widget->parent; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
        scale = scale * ancestor->scale;
    }
    in_bounds = halo::interface::widget_instance_point_in_bounds(widget);
    cursor_side = halo::interface::widget_cursor_side_of_midpoint(widget);

    if (widget->state == 0) {
        return;
    }

    if (widget->extended_description != (widget_instance *)0) {
        float desc_scale = widget->scale;

        for (ancestor = widget->parent; ancestor != (widget_instance *)0; ancestor = ancestor->parent) {
            desc_scale = desc_scale * ancestor->scale;
        }
        widget->extended_description->scale = desc_scale;
        halo::interface::widget_instance_render(widget->extended_description, dest, offset_xy, 0, 1);
    }

    if (widget->scroll_blink != 0) {
        if (widget->scroll_blink < 0) {
            widget->scroll_blink = widget->scroll_blink + 1;
            scroll_dir_up = 1;
        } else {
            widget->scroll_blink = widget->scroll_blink - 1;
            scroll_dir_down = 1;
        }
    }
    widget->selection_direction = 0;

    {
        int32_t arrow;

        for (arrow = 0; arrow < 2; arrow++) {
            datum_index bitmap_tag = *(datum_index *)&(arrow == 0 ? tag->list_header_bitmap : tag->list_footer_bitmap).tag_id;
            uint8_t *bitmap_tag_data = halo::interface::tag_data<uint8_t>(bitmap_tag);
            int16_t frame = (int16_t)(arrow == 0 ? scroll_dir_up : scroll_dir_down);
            int32_t bitmap;

            if (in_bounds != 0 && bitmap_tag_data != nullptr && *(int32_t *)(bitmap_tag_data + 0x60) == 4 &&
                halo::interface::widget_instance_point_in_bounds(widget) != 0 &&
                (arrow == 0 ? cursor_side <= 0 : cursor_side > 0)) {
                frame = (int16_t)(frame + 2);
            }
            bitmap = reinterpret_cast<int32_t>(halo::bitmaps::bitmap_group_sequence_get_bitmap_data(bitmap_tag, 0, frame));
            if (bitmap != 0) {
                Rectangle2D rect = arrow == 0 ? tag->header_bounds : tag->footer_bounds;
                float alpha = scale * 255.0f;

                rect.top = (int16_t)(rect.top + y_off);
                rect.left = (int16_t)(rect.left + x_off);
                rect.bottom = (int16_t)(rect.bottom + y_off);
                rect.right = (int16_t)(rect.right + x_off);
                halo::interface::ui_draw_screen_quad((int16_t *)&rect, (int16_t *)&rect, bitmap, (int16_t *)dest,
                                     (uint32_t)((int32_t)(alpha + 0.5f) << 24) | halo::interface::k_rgb_mask);
            }
        }
    }

    if (tag->child_widgets.count != 0) {
        return;
    }

    if (*(uint32_t *)&tag->text_label_unicode_strings_list.tag_id == halo::k_dword_none) {
        text = (uint16_t *)widget->list_render_data;
    } else {
        uint16_t *src =
            halo::text::text_string_list_get_string(*(uint32_t *)&tag->text_label_unicode_strings_list.tag_id,
                                        widget->selection_index);
        uint32_t byte_len = wcslen((const wchar_t *)src) * 2;
        uint16_t *buf = (uint16_t *)halo::memory::heap_allocate(byte_len + 2, widget_memory_pool);
        int32_t i;

        text = buf;
        if (buf == nullptr) {
            goto free_and_return;
        }
        {
            uint8_t *dst8 = (uint8_t *)buf;
            uint8_t *src8 = (uint8_t *)src;

            for (i = 0; i < (int32_t)byte_len; i++) {
                dst8[i] = src8[i];
            }
            buf[byte_len / 2] = 0;
        }
        if (tag->search_and_replace_functions.count > 0) {
            uint8_t *entries = (uint8_t *)tag->search_and_replace_functions.pointer;

            for (i = 0; i < tag->search_and_replace_functions.count; i++) {
                uint8_t *entry = entries + i * 0x22;

                if (entry != nullptr && *entry != 0) {

                    const uint16_t *replacement = halo::interface::ui_search_replace_function_call(*(int16_t *)(entry + 0x20), widget);
                    uint16_t search[0x20];

                    halo::interface::ui_string_replace_all((wchar_t *)(halo::text::string_convert_ascii_to_unicode(search, 0x40, (const char *)entry)), (uint16_t *)replacement, (wchar_t **)&text);
                }
            }
        }
    }

    if (text != nullptr && *(uint32_t *)&tag->text_font.tag_id != halo::k_dword_none) {
        int16_t justification = tag->justification;

        if (justification >= 0 && justification < 3) {

            float cumulative = halo::interface::widget_instance_get_cumulative_scale(widget);
            int16_t x = (int16_t)offset_xy;
            int16_t y = (int16_t)(offset_xy >> 16);
            Rectangle2D rect = tag->bounds;
            Rectangle2D clip = (dest != (Rectangle2D *)0) ? *dest : tag->bounds;
            ColorARGB color = ((struct UIWidgetDefinition *)t)->text_color;
            ColorARGB flash;

            rect.top = (int16_t)(rect.top + y);
            rect.left = (int16_t)(rect.left + x);
            rect.bottom = (int16_t)(rect.bottom + y);
            rect.right = (int16_t)(rect.right + x);
            if (is_top_of_stack != 0) {
                float *rgb = (float *)halo::interface::ui_get_saved_color((ColorRGB *)&flash);

                color.red = rgb[0];
                color.green = rgb[1];
                color.blue = rgb[2];
            }
            color.alpha = color.alpha * cumulative;
            if (*(uint8_t *)&widget->selection_direction != 0 || halo::interface::has_bit(((struct UIWidgetDefinition *)t)->flags_1, halo::tags::ui_widget_definition_flags1_tag_flag::flashing)) {
                double td = (double)ui_time_milliseconds;

                if (ui_time_milliseconds < 0) td += 4294967296.0;
                color.alpha = (float)((sin(td * 0.003) + 1.0) * 0.5 * (double)color.alpha);
            }

            halo::text::text_context::set_render_context(*(datum_index *)&tag->text_font.tag_id, &color, -1, justification, 0);
            halo::rasterizer::chimera__draw_16_bit_text(&clip, (int32_t *)(&rect), 0, 0, (const int16_t *)text);
        }
    }

free_and_return:
    if (*(uint32_t *)&tag->text_label_unicode_strings_list.tag_id != halo::k_dword_none && text != nullptr) {
        heap_block *block = (heap_block *)((uint8_t *)text - 0x10);
        uint32_t size = block->size;

        halo::memory::heap_unlink_block(block, widget_memory_pool);
        widget_memory_pool->bytes_allocated -= (int32_t)(size & halo::interface::k_pool_block_size_mask);
        widget_memory_pool->allocation_count -= 1;
    }
}

/**
 * blam-cc: EAX -> widget, ECX -> rect For a spinner_list widget with fewer than two children and both
 * scroll-arrow bitmaps set, shrinks the given layout rectangle to leave room for them: nudges the left edge in
 * by the header bitmap's own left margin minus 10, and grows the right edge out to clear the footer bitmap's
 * right margin plus 2.
 *
 * @address 0x499990
 */
void WidgetList::adjust_rect_for_scroll_arrows(Rectangle2D *rect)
{
    UIWidgetDefinition *tag;

    if (widget->widget_type != uiwidgettype_spinner_list  ) {
        return;
    }
    tag = halo::interface::tag_data<UIWidgetDefinition>(widget->definition);
    if (tag->child_widgets.count >= 2) {
        return;
    }
    if (*(uint32_t *)&tag->list_footer_bitmap.tag_id == halo::k_dword_none ||
        *(uint32_t *)&tag->list_header_bitmap.tag_id == halo::k_dword_none) {
        return;
    }
    rect->left = rect->left + tag->header_bounds.left - 10;
    if (rect->right <= tag->footer_bounds.right) {
        rect->right = tag->footer_bounds.right + 2;
    }
}

} // namespace halo::interface

namespace halo::interface {

widget_instance * widget_instance_find_at_point(widget_instance *widget, int32_t cursor_x, int32_t cursor_y, int32_t offset_xy)
{
    return halo::interface::WidgetView(widget).find_at_point(cursor_x, cursor_y, offset_xy);
}

uint8_t widget_instance_point_in_bounds(widget_instance *widget)
{
    return halo::interface::WidgetView(widget).point_in_bounds();
}

void widget_instance_render_list_head(widget_instance *widget, UIWidgetDefinition *tag, Rectangle2D *dest, int32_t offset_xy, uint8_t is_top_of_stack)
{
    halo::interface::WidgetRender(widget).render_list_head(tag, dest, offset_xy, is_top_of_stack);
}

void widget_list_adjust_rect_for_scroll_arrows(widget_instance *widget, Rectangle2D *rect)
{
    halo::interface::WidgetList(widget).adjust_rect_for_scroll_arrows(rect);
}

}
