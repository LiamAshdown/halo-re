/**
 * @file src/text/text_context.cpp
 * The shared text render context, fonts, localised string lists and measuring.
 * The original author notes and decompiles are in docs/original/text/.
 */

#include "halo/text/text.hpp"
#include "halo/text/limits.hpp"
#include "halo/core/datum.hpp"

extern "C" {
extern datum_index hud_text_draw_font_tag_id;
extern ColorARGB hud_text_draw_color_a;
extern int16_t hud_text_draw_color_or_flags;
extern int16_t hud_text_draw_column;
extern uint32_t hud_text_draw_unknown_4730;
extern tag_instance *tag_instances;
extern float text_color_scale;
extern Globals *global_globals;
extern datum_index text_localization_strings;
extern int16_t text_encoding_state;
extern int16_t hud_text_draw_background_mode;
extern int16_t ui_prompt_clip_x;
extern int16_t ui_prompt_clip_y;
extern char missing_string[17];
extern uint16_t missing_string_text[];
extern Rectangle2D text_measure_bounds;
extern uint32_t text_measure_font;
}

namespace halo::text {

void text_context::set_render_context(datum_index font, ColorARGB *color, int16_t style, int16_t justification, uint32_t flags)
{
    hud_text_draw_font_tag_id = font;
    hud_text_draw_color_a = *color;
    hud_text_draw_color_or_flags = style;
    hud_text_draw_column = justification;
    hud_text_draw_unknown_4730 = flags;
}

void text_context::parse_state_initialize(void *string, int16_t justification, int16_t style, text_parse_state *state, datum_index font, ColorARGB *color)
{
    datum_index resolved_font;

    state->font = font;
    state->string = (uint32_t)string;
    state->justification = justification;
    state->position = 0;
    state->style = style;

    state->color = ((uint32_t)(int32_t)(color->alpha * text_color_scale) << 24) |
                   ((uint32_t)(int32_t)(color->red   * text_color_scale) << 16) |
                   ((uint32_t)(int32_t)(color->green * text_color_scale) << 8) |
                   (uint32_t)(int32_t)(color->blue  * text_color_scale);

    resolved_font = font;
    if (style != (int16_t)-1) {
        Font *base_font = (Font *)tag_instances[halo::datum_slot(font)].data;

        TagDependency *style_dependency = &base_font->bold + style;
        resolved_font = *(datum_index *)&style_dependency->tag_id;
    }
    if (resolved_font == (datum_index)k_datum_index_none) {
        resolved_font = font;
    }
    state->font_definition = (uint32_t)tag_instances[halo::datum_slot(resolved_font)].data;
}

void text_context::language_initialize_from_string_list(void)
{
    GlobalsInterfaceBitmaps *bitmaps;
    StringList *localization;
    char *encoding_string;

    bitmaps = (global_globals->interface_bitmaps.count == 0) ? (GlobalsInterfaceBitmaps *)0
        : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    text_localization_strings = *(datum_index *)&bitmaps->localization.tag_id;

    if (text_localization_strings != (datum_index)k_datum_index_none) {
        localization = (StringList *)tag_instances[halo::datum_slot(text_localization_strings)].data;
        encoding_string = missing_string;
        if (localization->strings.count > 0) {
            StringListString *first = (StringListString *)localization->strings.pointer;
            if ((int32_t)first->string.size > 0) {
                encoding_string = (char *)first->string.pointer;
                encoding_string[first->string.size - 1] = '\0';
            }
        }
        text_encoding_state = (int16_t)atol(encoding_string);
        if (text_encoding_state < 0 || text_encoding_state > 5) {
            text_encoding_state = 0;
        }

        hud_text_draw_font_tag_id = (datum_index)k_datum_index_none;
        hud_text_draw_background_mode = 0;
        hud_text_draw_unknown_4730 = 0;
        hud_text_draw_column = 0;
        ui_prompt_clip_x = 0;
        ui_prompt_clip_y = 0;
    }
}

uint16_t * text_context::string_list_get_string(datum_index list_id, int16_t index)
{
    UnicodeStringList *list;
    UnicodeStringListString *entry;

    if (list_id == (datum_index)-1) {
        return missing_string_text;
    }
    list = (UnicodeStringList *)tag_instances[halo::datum_slot(list_id)].data;
    if (index < 0 || (int32_t)list->strings.count <= index) {
        return missing_string_text;
    }
    entry = (UnicodeStringListString *)list->strings.pointer + index;
    if (0 < (int32_t)entry->string.size) {
        uint16_t *string = (uint16_t *)entry->string.pointer;
        *(uint16_t *)((uint8_t *)string + ((entry->string.size & ~1u) - 2)) = 0;
        return string;
    }
    return missing_string_text;
}

FontCharacter * text_context::get_character_metrics(uint16_t character, Font *font)
{
    FontCharacterTables *page;

    page = (FontCharacterTables *)font->character_tables.pointer + (character >> 8);
    if (0 < (int32_t)page->character_table.count) {
        int16_t *character_index = (page->character_table.count ==
            k_text_font_character_table_page_size)
                ? (int16_t *)page->character_table.pointer + (character & 0xff)
                : (int16_t *)0;
        int16_t hardware_index = *character_index;
        if (hardware_index != -1) {
            return (FontCharacter *)font->characters.pointer + hardware_index;
        }
    }
    return (FontCharacter *)((void *)0);
}

void text_context::measure_string_extents(Rectangle2D *origin_bounds, Rectangle2D *out_cursor_rect, Rectangle2D *out_extents_rect, void *string)
{
    Font *font;
    datum_index resolved_font;
    Point2DInt pen;

    text_measure_bounds.top = k_text_coordinate_max;
    text_measure_bounds.left = k_text_coordinate_max;
    text_measure_bounds.bottom = k_text_coordinate_min;
    text_measure_bounds.right = k_text_coordinate_min;

    resolved_font = hud_text_draw_font_tag_id;
    if (hud_text_draw_color_or_flags != (int16_t)-1) {
        Font *base_font = (Font *)tag_instances[halo::datum_slot(hud_text_draw_font_tag_id)].data;

        TagDependency *style_dependency = &base_font->bold + hud_text_draw_color_or_flags;
        resolved_font = *(datum_index *)&style_dependency->tag_id;
    }
    if (resolved_font == (datum_index)k_datum_index_none) {
        resolved_font = hud_text_draw_font_tag_id;
    }
    text_measure_font = (uint32_t)tag_instances[halo::datum_slot(resolved_font)].data;

    wide_text_strategy::instance().wrap_and_draw(text_measure_glyph_callback, origin_bounds, &pen, (Rectangle2D *)0, 0, string);

    font = (Font *)text_measure_font;

    out_cursor_rect->left = pen.x;
    out_cursor_rect->right = (int16_t)(pen.x + 1);
    out_cursor_rect->top = (int16_t)(pen.y - font->ascending_height);
    out_cursor_rect->bottom = (int16_t)(pen.y + font->descending_height);

    out_extents_rect->top = origin_bounds->top;
    out_extents_rect->left = text_measure_bounds.left;
    out_extents_rect->right = text_measure_bounds.right;
    out_extents_rect->bottom = out_cursor_rect->bottom;
}

int32_t text_context::measure_string_fit_width(void *string, int32_t *max_width_inout)
{
    text_parse_state state;
    int32_t consumed_width;
    int32_t break_column;

    consumed_width = 0;
    break_column = 0;
    text_context::parse_state_initialize(string, hud_text_draw_column, hud_text_draw_color_or_flags, &state, hud_text_draw_font_tag_id, &hud_text_draw_color_a);

    for (;;) {
        wide_text_strategy::instance().parse_next_token(&state);
        switch (state.token) {
        case _text_token_end:
            *max_width_inout -= consumed_width;
            return 0;
        case _text_token_newline:
        case _text_token_break_character:
        case _text_token_tab:
        case _text_token_justification:
        case _text_token_unused_5:
            break_column = state.position;
            break;
        case _text_token_character: {
            Font *font;
            FontCharacterTables *page;

            font = (Font *)state.font_definition;
            page = (FontCharacterTables *)font->character_tables.pointer +
                (state.character >> 8);
            if (0 < (int32_t)page->character_table.count) {
                int16_t *character_index = (page->character_table.count ==
                    k_text_font_character_table_page_size)
                        ? (int16_t *)page->character_table.pointer + (state.character & 0xff)
                        : (int16_t *)0;
                int16_t hardware_index = *character_index;
                if (hardware_index != -1) {
                    FontCharacter *character = (FontCharacter *)font->characters.pointer +
                        hardware_index;
                    if (character != (void *)0) {
                        if (*max_width_inout <= (int32_t)character->bitmap_width + consumed_width) {
                            *max_width_inout -= consumed_width;
                            return break_column;
                        }
                        break_column = state.position;
                        consumed_width += character->character_width;
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
}

void text_measure_glyph_callback(text_parse_state *state, void *font, void *character, uint32_t color, int16_t x, int16_t y, int16_t source_x, int16_t source_y, int16_t width, int16_t height)
{
    int16_t right;
    int16_t bottom;

    right = (int16_t)(x + width);
    bottom = (int16_t)(y + height);

    if (x < text_measure_bounds.left) {
        text_measure_bounds.left = x;
    }
    if (y < text_measure_bounds.top) {
        text_measure_bounds.top = y;
    }
    if (right > text_measure_bounds.right) {
        text_measure_bounds.right = right;
    }
    if (bottom > text_measure_bounds.bottom) {
        text_measure_bounds.bottom = bottom;
    }

    text_measure_font = (uint32_t)font;
}

}  // namespace halo::text
