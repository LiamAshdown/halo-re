#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::ui {

/**
 * Screen-space quad builders, the button-prompt text drawing and the generic UI rectangle fill.
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiDraw {
    static void button_prompt_draw_icon(HUDGlobalsButtonIcon *icon);
    static int16_t button_prompt_index_from_string(uint16_t *text);
    static void draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect);
    static void draw_rotated_screen_quad(int16_t *origin, int32_t source_record, float *corner_uvs,
                                  float scale, float rotation_radians, float alpha_fraction);
    static void draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data,
                          int16_t *clip_rect, uint32_t vertex_color);
    static void draw_trouble_brewing_indicator(void);
    static uint8_t string_has_button_prompt_token(uint16_t *text);
    static void widget_draw_formatted_prompt_string(Rectangle2D *bounds, uint8_t use_text_color, const uint16_t *text);
    static void widget_draw_prompt_span(const uint16_t *text, Rectangle2D *cursor, Rectangle2D *origin);
};

}
