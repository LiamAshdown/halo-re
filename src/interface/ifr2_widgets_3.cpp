#include "halo/interface/ifr2_widgets.hpp"
#include "halo/text/api.hpp"
#include "halo/memory/api.hpp"
#include <wchar.h>
#include "halo/rasterizer/api.hpp"
#include "halo/text/text.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/interface/flags.hpp"
#include "halo/core/datum.hpp"
#include "halo/interface/records.hpp"
#include "halo/interface/widget_pool.hpp"
#include "halo/interface/wide_text.hpp"

#ifdef interface
#undef interface
#endif

static auto &ui_time_milliseconds = halo::link::ref<int32_t>(halo::ui::vars().ui_time_milliseconds);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &ui_replace_function_table = halo::link::ref<void *[4]>(halo::ui::vars().ui_replace_function_table);
static auto &ui_invalid_replacement_text = halo::link::ref<uint16_t []>(halo::ui::vars().ui_invalid_replacement_text);
static auto &ui_out_of_memory_text = halo::link::ref<uint16_t []>(halo::ui::vars().ui_out_of_memory_text);

namespace halo::interface {

/**
 * Prepares and draws a text-box widget's caption: refreshes it from the tag's string list (if set), applies
 * search-and-replace substitutions, then draws it plain or through the button-prompt formatter,
 * colored/scaled/pulsed per the tag and widget state.
 *
 * @address 0x49b1d0
 */
void WidgetRender::render_text_box(UIWidgetDefinition *tag, Rectangle2D *dest, int32_t offset_xy, uint8_t is_top_of_stack)
{
    int32_t i;

    if (halo::interface::tag_handle(tag->text_label_unicode_strings_list.tag_id) != halo::k_dword_none) {
        int16_t index = widget->selection_index;
        uint16_t *src;
        uint32_t byte_len;
        uint16_t *buf;

        if (index == -1) {
            index = (int16_t)tag->string_list_index;
        }
        src = halo::text::text_string_list_get_string(halo::interface::tag_handle(tag->text_label_unicode_strings_list.tag_id), index);
        byte_len = wcslen((const wchar_t *)src) * 2;
        buf = halo::interface::widget_pool_resize_text(widget->text, byte_len + 2);
        widget->text = buf;
        if (buf == nullptr) {
            widget->text = ui_out_of_memory_text;
        } else {
            memcpy(buf, src, byte_len);
            buf[byte_len / 2] = 0;
        }
    }

    if (widget->text == nullptr || *halo::interface::widget_text(widget) == 0) {
        return;
    }

    for (i = 0; i < tag->search_and_replace_functions.count; i++) {
        SearchAndReplaceReference *entry = (SearchAndReplaceReference *)tag->search_and_replace_functions.pointer + i;

        if (entry != nullptr && entry->search_string.string[0] != 0) {
            int16_t fn = entry->replace_function;
            const uint16_t *replacement;
            uint16_t search[0x20];

            if (fn < 0 || fn >= 4) {
                replacement = ui_invalid_replacement_text;
            } else {
                replacement = (const uint16_t *)((ui_search_replace_function)ui_replace_function_table[fn])(widget);
            }
            halo::interface::ui_string_replace_all((wchar_t *)(halo::text::string_convert_ascii_to_unicode(search, 0x40, entry->search_string.string)), (uint16_t *)replacement,
                                  (wchar_t **)&widget->text);
        }
    }

    if (halo::interface::tag_handle(tag->text_font.tag_id) == halo::k_dword_none) {
        return;
    }
    if (tag->justification < 0 || tag->justification >= 3) {
        return;
    }
    if (widget->state == 0) {
        return;
    }

    {
        float scale = halo::interface::widget_instance_get_cumulative_scale(widget);
        int16_t x = (int16_t)offset_xy;
        int16_t y = (int16_t)(offset_xy >> 16);
        Rectangle2D rects[2];
        ColorARGB color;
        ColorARGB highlight;

        rects[1] = (dest != (Rectangle2D *)0) ? *dest : tag->bounds;
        rects[0].top = (int16_t)(tag->bounds.top + y + tag->vert_offset);
        rects[0].left = (int16_t)(tag->bounds.left + x + tag->horiz_offset);
        rects[0].bottom = (int16_t)(tag->bounds.bottom + y);
        rects[0].right = (int16_t)(tag->bounds.right + x);

        if (widget->text_color_override.alpha != 0.0f) {
            color = widget->text_color_override;
            color.alpha = color.alpha * scale;
        } else if (is_top_of_stack != 0) {
            color = *halo::interface::ui_get_saved_pulse_color(&highlight);
            color.alpha = tag->text_color.alpha * scale;
        } else {
            color = tag->text_color;
            if (color.red == 1.0f && color.green == 1.0f && color.blue == 1.0f) {
                color = *halo::interface::ui_get_saved_pulse_color(&highlight);
                color.alpha = tag->text_color.alpha;
            }
            color.alpha = color.alpha * scale;
        }
        if ((uint8_t)widget->selection_direction != 0 || halo::interface::has_bit(tag->flags_1, halo::tags::ui_widget_definition_flags1_tag_flag::flashing)) {
            double time = (double)ui_time_milliseconds;

            if (ui_time_milliseconds < 0) {
                time += 4294967296.0;
            }
            color.alpha = (float)((halo::libm::cos(time * 0.003) + 1.5) * 0.4 * (double)color.alpha);
        }

        halo::text::text_context::set_render_context(halo::interface::tag_handle(tag->text_font.tag_id), &color, -1, tag->justification, 0);
        if (halo::interface::ui_string_has_button_prompt_token(halo::interface::widget_text(widget)) == 0) {
            halo::interface::draw_text16(&rects[1], &rects[0], halo::interface::widget_text(widget));
            return;
        }
        halo::interface::ui_widget_draw_formatted_prompt_string(&rects[0], 0, halo::interface::widget_text(widget));
    }
}

} // namespace halo::interface

namespace halo::interface {

void widget_instance_render_text_box(widget_instance *widget, UIWidgetDefinition *tag, Rectangle2D *dest, int32_t offset_xy, uint8_t is_top_of_stack)
{
    halo::interface::WidgetRender(widget).render_text_box(tag, dest, offset_xy, is_top_of_stack);
}

}
