#include "halo/interface/ifr2_widgets.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern int32_t ui_time_milliseconds;
extern heap *widget_memory_pool;
extern double cos(double x);
extern void *ui_replace_function_table[4];
extern uint16_t ui_invalid_replacement_text[];
extern uint16_t ui_out_of_memory_text[];
extern uint16_t *text_string_list_get_string(datum_index string_list_tag, int16_t index);
extern uint32_t wcslen(const uint16_t *s);
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self);
extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dest, int32_t dest_bytes, const char *source);
extern void ui_string_replace_all(const uint16_t *search, const uint16_t *replacement, uint16_t **text);
extern float widget_instance_get_cumulative_scale(widget_instance *widget);
extern ColorARGB *ui_get_saved_pulse_color(ColorARGB *out);
extern void text_set_render_context(datum_index font, ColorARGB *color, int32_t unknown_0, int32_t justification, int32_t unknown_1);
extern uint8_t ui_string_has_button_prompt_token(uint16_t *text);
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0, int32_t unknown_1, const uint16_t *text);
extern void ui_widget_draw_formatted_prompt_string(Rectangle2D *bounds, uint8_t use_text_color, const uint16_t *text);
}

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
    uint8_t *w = (uint8_t *)widget;
    uint8_t *t = (uint8_t *)tag;
    int32_t i;

    if (*(uint32_t *)&tag->text_label_unicode_strings_list.tag_id != 0xffffffffu) {
        int16_t index = widget->selection_index;
        uint16_t *src;
        uint32_t byte_len;
        uint16_t *buf;

        if (index == -1) {
            index = *(int16_t *)&((struct UIWidgetDefinition *)t)->string_list_index;
        }
        src = text_string_list_get_string(*(datum_index *)&tag->text_label_unicode_strings_list.tag_id, index);
        byte_len = wcslen(src) * 2;
        buf = (uint16_t *)heap_reallocate(widget->text, byte_len + 2, widget_memory_pool);
        widget->text = buf;
        if (buf == (uint16_t *)0) {
            widget->text = ui_out_of_memory_text;
        } else {
            uint8_t *dst8 = (uint8_t *)buf;
            uint8_t *src8 = (uint8_t *)src;

            for (i = 0; i < (int32_t)byte_len; i++) {
                dst8[i] = src8[i];
            }
            *(uint16_t *)((uint8_t *)widget->text + byte_len) = 0;
        }
    }

    if (widget->text == (void *)0 || *(uint16_t *)widget->text == 0) {
        return;
    }

    for (i = 0; i < tag->search_and_replace_functions.count; i++) {
        uint8_t *entry = (uint8_t *)tag->search_and_replace_functions.pointer + i * 0x22;

        if (entry != (uint8_t *)0 && *entry != 0) {
            int16_t fn = *(int16_t *)(entry + 0x20);
            const uint16_t *replacement;
            uint16_t search[0x20];

            if (fn < 0 || fn >= 4) {
                replacement = ui_invalid_replacement_text;
            } else {
                replacement = (const uint16_t *)((ui_search_replace_function)ui_replace_function_table[fn])(widget);
            }
            ui_string_replace_all(string_convert_ascii_to_unicode(search, 0x40, (const char *)entry), replacement,
                                  (uint16_t **)&widget->text);
        }
    }

    if (*(uint32_t *)&tag->text_font.tag_id == 0xffffffffu) {
        return;
    }
    if (tag->justification < 0 || tag->justification >= 3) {
        return;
    }
    if (widget->state == 0) {
        return;
    }

    {
        float scale = widget_instance_get_cumulative_scale(widget);
        int16_t x = (int16_t)offset_xy;
        int16_t y = (int16_t)(offset_xy >> 16);
        Rectangle2D rects[2];
        ColorARGB color;
        ColorARGB highlight;

        rects[1] = (dest != (Rectangle2D *)0) ? *dest : tag->bounds;
        rects[0].top = (int16_t)(tag->bounds.top + y + ((struct UIWidgetDefinition *)t)->vert_offset);
        rects[0].left = (int16_t)(tag->bounds.left + x + ((struct UIWidgetDefinition *)t)->horiz_offset);
        rects[0].bottom = (int16_t)(tag->bounds.bottom + y);
        rects[0].right = (int16_t)(tag->bounds.right + x);

        if (*(float *)&((struct widget_instance *)w)->list_items != 0.0f) {
            color = *(ColorARGB *)&((struct widget_instance *)w)->list_items;
            color.alpha = color.alpha * scale;
        } else if (is_top_of_stack != 0) {
            color = *ui_get_saved_pulse_color(&highlight);
            color.alpha = *(float *)&((struct UIWidgetDefinition *)t)->text_color * scale;
        } else {
            color = ((struct UIWidgetDefinition *)t)->text_color;
            if (color.red == 1.0f && color.green == 1.0f && color.blue == 1.0f) {
                color = *ui_get_saved_pulse_color(&highlight);
                color.alpha = *(float *)&((struct UIWidgetDefinition *)t)->text_color;
            }
            color.alpha = color.alpha * scale;
        }
        if (w[0x54] != 0 || (t[0x11e] & 4) != 0) {
            double time = (double)ui_time_milliseconds;

            if (ui_time_milliseconds < 0) {
                time += 4294967296.0;
            }
            color.alpha = (float)((cos(time * 0.003) + 1.5) * 0.4 * (double)color.alpha);
        }

        text_set_render_context(*(datum_index *)&tag->text_font.tag_id, &color, -1, tag->justification, 0);
        if (ui_string_has_button_prompt_token((uint16_t *)widget->text) == 0) {
            chimera__draw_16_bit_text(&rects[1], &rects[0], 0, 0, (uint16_t *)widget->text);
            return;
        }
        ui_widget_draw_formatted_prompt_string(&rects[0], 0, (uint16_t *)widget->text);
    }
}

} // namespace halo::interface

extern "C" {

void widget_instance_render_text_box(widget_instance *widget, UIWidgetDefinition *tag, Rectangle2D *dest, int32_t offset_xy, uint8_t is_top_of_stack)
{
    halo::interface::WidgetRender(widget).render_text_box(tag, dest, offset_xy, is_top_of_stack);
}

}
