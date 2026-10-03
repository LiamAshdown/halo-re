/**
 * @file standalone/data/link/text.hpp
 * Link names of the engine variables the text module binds in halo::text::Globals (src/text/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern int16_t text_encoding_state;
extern char text_markup_codes[11];
extern ColorARGB hud_text_draw_color_a;
extern datum_index hud_text_draw_font_tag_id;
extern int16_t hud_text_draw_background_mode;
extern int16_t hud_text_draw_color_or_flags;
extern int16_t hud_text_draw_column;
extern uint32_t hud_text_draw_unknown_4730;
extern datum_index text_localization_strings;
extern float text_color_scale;
extern int16_t ui_prompt_clip_x;
extern int16_t ui_prompt_clip_y;
extern Globals *global_globals;
extern char missing_string[17];
extern uint16_t missing_string_text[];
extern Rectangle2D text_measure_bounds;
extern uint32_t text_measure_font;
extern int16_t text_highlight_start;
extern int16_t text_highlight_end;
extern int16_t text_tab_stops[k_text_maximum_tab_stops];
}
