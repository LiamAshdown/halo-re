/**
 * @file src/text/text_context.cpp
 * The shared text render context, fonts, localised string lists and measuring.
 */

#include "halo/text/text.hpp"
#include "halo/text/limits.hpp"
#include "halo/core/datum.hpp"
#include "halo/cache/api.hpp"
#include "halo/text/api.hpp"


namespace halo::text {

void text_context::set_render_context(datum_index font, ColorARGB *color, int16_t style, int16_t justification, uint32_t flags)
{
    globals().hud_text_draw_font_tag_id = font;
    globals().hud_text_draw_color_a = *color;
    globals().hud_text_draw_color_or_flags = style;
    globals().hud_text_draw_column = justification;
    globals().hud_text_draw_unknown_4730 = flags;
}

void text_context::parse_state_initialize(void *string, int16_t justification, int16_t style, text_parse_state *state, datum_index font, ColorARGB *color)
{
    datum_index resolved_font;

    state->font = font;
    state->string = (uint32_t)string;
    state->justification = justification;
    state->position = 0;
    state->style = style;

    state->color = ((uint32_t)(int32_t)(color->alpha * globals().color_scale) << 24) |
                   ((uint32_t)(int32_t)(color->red   * globals().color_scale) << 16) |
                   ((uint32_t)(int32_t)(color->green * globals().color_scale) << 8) |
                   (uint32_t)(int32_t)(color->blue  * globals().color_scale);

    resolved_font = font;
    if (style != (int16_t)-1) {
        Font *base_font = (Font *)halo::cache::globals().tag_instances[halo::datum_slot(font)].data;

        TagDependency *style_dependency = &base_font->bold + style;
        resolved_font = *(datum_index *)&style_dependency->tag_id;
    }
    if (resolved_font == (datum_index)k_datum_index_none) {
        resolved_font = font;
    }
    state->font_definition = (uint32_t)halo::cache::globals().tag_instances[halo::datum_slot(resolved_font)].data;
}

void text_context::language_initialize_from_string_list(void)
{
    GlobalsInterfaceBitmaps *bitmaps;
    StringList *localization;
    char *encoding_string;

    bitmaps = (globals().global_globals->interface_bitmaps.count == 0) ? (GlobalsInterfaceBitmaps *)0
        : (GlobalsInterfaceBitmaps *)globals().global_globals->interface_bitmaps.pointer;
    globals().localization_strings = *(datum_index *)&bitmaps->localization.tag_id;

    if (globals().localization_strings != (datum_index)k_datum_index_none) {
        localization = (StringList *)halo::cache::globals().tag_instances[halo::datum_slot(globals().localization_strings)].data;
        encoding_string = globals().missing_string;
        if (localization->strings.count > 0) {
            StringListString *first = (StringListString *)localization->strings.pointer;
            if ((int32_t)first->string.size > 0) {
                encoding_string = (char *)first->string.pointer;
                encoding_string[first->string.size - 1] = '\0';
            }
        }
        globals().text_encoding_state = (int16_t)atol(encoding_string);
        if (globals().text_encoding_state < 0 || globals().text_encoding_state > 5) {
            globals().text_encoding_state = 0;
        }

        globals().hud_text_draw_font_tag_id = (datum_index)k_datum_index_none;
        globals().hud_text_draw_background_mode = 0;
        globals().hud_text_draw_unknown_4730 = 0;
        globals().hud_text_draw_column = 0;
        globals().ui_prompt_clip_x = 0;
        globals().ui_prompt_clip_y = 0;
    }
}

uint16_t * text_context::string_list_get_string(datum_index list_id, int16_t index)
{
    UnicodeStringList *list;
    UnicodeStringListString *entry;

    if (list_id == k_datum_index_none) {
        return globals().missing_string_text;
    }
    list = (UnicodeStringList *)halo::cache::globals().tag_instances[halo::datum_slot(list_id)].data;
    if (index < 0 || (int32_t)list->strings.count <= index) {
        return globals().missing_string_text;
    }
    entry = (UnicodeStringListString *)list->strings.pointer + index;
    if (0 < (int32_t)entry->string.size) {
        uint16_t *string = (uint16_t *)entry->string.pointer;
        *(uint16_t *)((uint8_t *)string + ((entry->string.size & ~1u) - 2)) = 0;
        return string;
    }
    return globals().missing_string_text;
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

    globals().text_measure_bounds.top = k_text_coordinate_max;
    globals().text_measure_bounds.left = k_text_coordinate_max;
    globals().text_measure_bounds.bottom = k_text_coordinate_min;
    globals().text_measure_bounds.right = k_text_coordinate_min;

    resolved_font = globals().hud_text_draw_font_tag_id;
    if (globals().hud_text_draw_color_or_flags != (int16_t)-1) {
        Font *base_font = (Font *)halo::cache::globals().tag_instances[halo::datum_slot(globals().hud_text_draw_font_tag_id)].data;

        TagDependency *style_dependency = &base_font->bold + globals().hud_text_draw_color_or_flags;
        resolved_font = *(datum_index *)&style_dependency->tag_id;
    }
    if (resolved_font == (datum_index)k_datum_index_none) {
        resolved_font = globals().hud_text_draw_font_tag_id;
    }
    globals().text_measure_font = (uint32_t)halo::cache::globals().tag_instances[halo::datum_slot(resolved_font)].data;

    wide_text_strategy::instance().wrap_and_draw(text_measure_glyph_callback, origin_bounds, &pen, (Rectangle2D *)0, 0, string);

    font = (Font *)globals().text_measure_font;

    out_cursor_rect->left = pen.x;
    out_cursor_rect->right = (int16_t)(pen.x + 1);
    out_cursor_rect->top = (int16_t)(pen.y - font->ascending_height);
    out_cursor_rect->bottom = (int16_t)(pen.y + font->descending_height);

    out_extents_rect->top = origin_bounds->top;
    out_extents_rect->left = globals().text_measure_bounds.left;
    out_extents_rect->right = globals().text_measure_bounds.right;
    out_extents_rect->bottom = out_cursor_rect->bottom;
}

int32_t text_context::measure_string_fit_width(void *string, int32_t *max_width_inout)
{
    text_parse_state state;
    int32_t consumed_width;
    int32_t break_column;

    consumed_width = 0;
    break_column = 0;
    text_context::parse_state_initialize(string, globals().hud_text_draw_column, globals().hud_text_draw_color_or_flags, &state, globals().hud_text_draw_font_tag_id, &globals().hud_text_draw_color_a);

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

    if (x < globals().text_measure_bounds.left) {
        globals().text_measure_bounds.left = x;
    }
    if (y < globals().text_measure_bounds.top) {
        globals().text_measure_bounds.top = y;
    }
    if (right > globals().text_measure_bounds.right) {
        globals().text_measure_bounds.right = right;
    }
    if (bottom > globals().text_measure_bounds.bottom) {
        globals().text_measure_bounds.bottom = bottom;
    }

    globals().text_measure_font = (uint32_t)font;
}

}  // namespace halo::text
