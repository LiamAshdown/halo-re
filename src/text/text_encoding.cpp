/**
 * @file src/text/text_encoding.cpp
 * Narrow and wide text layout: tokenising, column drawing and wrapping behind one strategy interface.
 * The original author notes and decompiles are in docs/original/text/.
 */

#include "halo/text/text.hpp"
#include "halo/text/limits.hpp"
#include "halo/core/datum.hpp"
#include "halo/cache/api.hpp"
#include "halo/text/api.hpp"


namespace halo::text {

int16_t narrow_text_strategy::parse_next_token(text_parse_state *state)
{
    uint8_t *position;
    uint16_t ch;
    int16_t token;

    for (;;) {
        position = (uint8_t *)state->string + state->position;
        if (dbcs_text::char_is_double_byte(position)) {
            ch = ((uint16_t)position[0] << 8) | position[1];
            state->position = (int16_t)(state->position + 2);
        } else {
            ch = position[0];
            state->position = (int16_t)(state->position + 1);
        }

        token = -1;
        if ((ch & k_text_high_byte_mask) == k_text_escape_lead) {
            switch (tolower(ch & k_text_low_byte_mask)) {
            case 'b': state->style = _text_style_bold;      token = _text_token_style; break;
            case 'c': state->justification = _text_justification_center; token = _text_token_justification; break;
            case 'i': state->style = _text_style_italic;    token = _text_token_style; break;
            case 'k': state->style = _text_style_condense;  token = _text_token_style; break;
            case 'l': state->justification = _text_justification_left;  token = _text_token_justification; break;
            case 'n': token = _text_token_newline; break;
            case 'p': state->style = _text_style_plain;     token = _text_token_style; break;
            case 'r': state->justification = _text_justification_right; token = _text_token_justification; break;
            case 't': token = _text_token_tab; break;
            case 'u': state->style = _text_style_underline; token = _text_token_style; break;
            }
        }

        if (token == -1) {
            break;
        }

        if (token == _text_token_style) {
            datum_index resolved_font = state->font;
            if (state->style != (int16_t)-1) {
                Font *base_font = (Font *)halo::cache::globals().tag_instances[halo::datum_slot(state->font)].data;
                TagDependency *style_dependency = &base_font->bold + state->style;
                resolved_font = *(datum_index *)&style_dependency->tag_id;
                if (resolved_font == (datum_index)k_datum_index_none) {
                    resolved_font = state->font;
                }
            }
            state->font_definition = (uint32_t)halo::cache::globals().tag_instances[halo::datum_slot(resolved_font)].data;
        }

        if (token != _text_token_style && token != _text_token_unused_5) {
            state->token = token;
            state->character = ch;
            return token;
        }
    }

    if (ch == 0) {
        state->token = _text_token_end;
        state->character = 0;
        return _text_token_end;
    }
    if (ch == '\t') {
        state->token = _text_token_tab;
        state->character = ch;
        return _text_token_tab;
    }
    if (ch == '\r') {
        state->token = _text_token_newline;
        state->character = ch;
        return _text_token_newline;
    }

    {
        int is_double_byte_char = (ch & k_text_high_byte_mask) != 0;
        uint8_t *next_position = (uint8_t *)state->string + state->position;
        uint16_t lookahead_char;
        StringList *localization;
        char *single_byte_break_characters = globals().missing_string;
        char *double_byte_no_break_characters = globals().missing_string;
        char *no_break_characters = globals().missing_string;
        int is_break_character;

        if (dbcs_text::char_is_double_byte(next_position)) {
            lookahead_char = ((uint16_t)next_position[0] << 8) | next_position[1];
        } else {
            lookahead_char = next_position[0];
        }

        if (globals().localization_strings != (datum_index)k_datum_index_none) {
            localization = (StringList *)halo::cache::globals().tag_instances[halo::datum_slot(globals().localization_strings)].data;

            if (localization->strings.count > _text_localization_single_byte_break_characters) {
                StringListString *entry = (StringListString *)localization->strings.pointer +
                    _text_localization_single_byte_break_characters;
                if ((int32_t)entry->string.size > 0) {
                    single_byte_break_characters = (char *)entry->string.pointer;
                    single_byte_break_characters[entry->string.size - 1] = '\0';
                }
            }
            if (localization->strings.count > _text_localization_double_byte_no_break_characters) {
                StringListString *entry = (StringListString *)localization->strings.pointer +
                    _text_localization_double_byte_no_break_characters;
                if ((int32_t)entry->string.size > 0) {
                    double_byte_no_break_characters = (char *)entry->string.pointer;
                    double_byte_no_break_characters[entry->string.size - 1] = '\0';
                }
            }
            if (localization->strings.count > _text_localization_no_break_characters) {
                StringListString *entry = (StringListString *)localization->strings.pointer +
                    _text_localization_no_break_characters;
                if ((int32_t)entry->string.size > 0) {
                    no_break_characters = (char *)entry->string.pointer;
                    no_break_characters[entry->string.size - 1] = '\0';
                }
            }
        }

        if (!is_double_byte_char) {
            is_break_character = dbcs_text::find_character((int16_t)ch, (uint8_t *)single_byte_break_characters) != 0;
        } else {
            is_break_character = dbcs_text::find_character((int16_t)ch, (uint8_t *)double_byte_no_break_characters) == 0;
        }
        if (is_break_character) {
            is_break_character = dbcs_text::find_character((int16_t)lookahead_char, (uint8_t *)no_break_characters) == 0;
        }

        token = is_break_character ? _text_token_break_character : _text_token_character;
        state->token = token;
        state->character = ch;
        return token;
    }
}

void narrow_text_strategy::draw_character_range(Rectangle2D *bounds, text_glyph_draw_proc callback, Point2DInt *pen, Rectangle2D *clip, uint32_t color, void *string, int16_t start_column, int16_t end_column)
{
    text_parse_state state;

    int16_t left, right, top, bottom;

    left = k_text_coordinate_min;
    right = k_text_coordinate_max;
    top = k_text_coordinate_min;
    bottom = k_text_coordinate_max;
    if (bounds != (void *)0) {
        left = bounds->left;
        right = bounds->right;
        top = bounds->top;
        bottom = bounds->bottom;
    }
    if (clip != (void *)0) {
        if (clip->left > left) left = clip->left;
        if (clip->right < right) right = clip->right;
        if (clip->top > top) top = clip->top;
        if (clip->bottom < bottom) bottom = clip->bottom;
    }

    if (left < right && top < bottom) {
        text_context::parse_state_initialize(string, globals().hud_text_draw_column, globals().hud_text_draw_color_or_flags, &state, globals().hud_text_draw_font_tag_id, &globals().hud_text_draw_color_a);
        state.position = start_column;
        while (state.position < end_column) {
            uint32_t glyph_color;
            Font *font;
            FontCharacterTables *page;
            int16_t hardware_index;
            FontCharacter *character;

            glyph_color = (state.position < globals().text_highlight_start ||
                           globals().text_highlight_end <= state.position)
                              ? color : (color ^ k_text_rgb_mask);

            narrow_text_strategy::instance().parse_next_token(&state);

            font = (Font *)state.font_definition;
            page = (FontCharacterTables *)font->character_tables.pointer + (state.character >> 8);
            if (0 < (int32_t)page->character_table.count) {
                int16_t *character_index = (page->character_table.count ==
                    k_text_font_character_table_page_size)
                        ? (int16_t *)page->character_table.pointer + (state.character & 0xff)
                        : (int16_t *)0;
                hardware_index = *character_index;
                if (hardware_index != -1) {
                    character = (FontCharacter *)font->characters.pointer + hardware_index;
                    if (character != (void *)0) {
                        int16_t draw_x, draw_y;

                        int16_t source_x, source_y, width, height;

                        draw_x = (int16_t)(pen->x - character->bitmap_origin_x);
                        draw_y = (int16_t)(pen->y - character->bitmap_origin_y);
                        pen->x = (int16_t)(pen->x + character->character_width);

                        source_x = 0;
                        source_y = 0;
                        width = character->bitmap_width;
                        height = character->bitmap_height;

                        if ((int32_t)draw_x + (int32_t)character->bitmap_width > right) {
                            width = (int16_t)(right - draw_x);
                        }
                        if (draw_x < left) {
                            source_x = (int16_t)(left - draw_x);
                            width = (int16_t)(width - source_x);
                            draw_x = left;
                        }

                        if ((int32_t)draw_y + (int32_t)character->bitmap_height > bottom) {
                            height = (int16_t)(bottom - draw_y);
                        }
                        if (draw_y < top) {
                            source_y = (int16_t)(top - draw_y);
                            height = (int16_t)(height - source_y);
                            draw_y = top;
                        }

                        if (0 < width && 0 < height) {
                            callback(&state, font, character, glyph_color, draw_x, draw_y,
                                source_x, source_y, width, height);
                        }
                    }
                }
            }
        }
    }
}

void narrow_text_strategy::wrap_and_draw(text_glyph_draw_proc callback, Rectangle2D *bounds, Point2DInt *out_final_pen, Rectangle2D *clip, int16_t extra_line_spacing, void *string)
{
    text_parse_state state;
    Font *font;
    int16_t tab_index, line_index, max_wrapped_sub_line_count, wrapped_sub_line_count;
    Point2DInt pen;
    Rectangle2D line_bounds;

    tab_index = 0;
    line_index = 0;
    wrapped_sub_line_count = 0;
    max_wrapped_sub_line_count = 0;

    text_context::parse_state_initialize(string, globals().hud_text_draw_column, globals().hud_text_draw_color_or_flags, &state, globals().hud_text_draw_font_tag_id, &globals().hud_text_draw_color_a);
    font = (Font *)state.font_definition;

    for (;;) {
        int16_t span_start_position = state.position;
        int16_t span_justification = state.justification;
        int16_t span_width = 0;

        int16_t candidate_position = 0;
        int16_t candidate_width = 0;
        int16_t flush_end_position = 0;
        int16_t token, previous_token;
        int16_t pen_y;

        line_bounds = *bounds;

        if (globals().hud_text_draw_background_mode < 1) {
            line_bounds.left = (int16_t)(line_bounds.left +
                (line_index == 0 ? globals().ui_prompt_clip_x : globals().ui_prompt_clip_y));
        } else if (tab_index == 0) {
            line_bounds.left = (int16_t)(line_bounds.left +
                (line_index == 0 ? globals().ui_prompt_clip_x : globals().ui_prompt_clip_y));
            if (tab_index < globals().hud_text_draw_background_mode) {
                line_bounds.right = globals().text_tab_stops[tab_index];
            }
        } else {
            line_bounds.left = (&globals().hud_text_draw_background_mode)[tab_index];
            if (tab_index < globals().hud_text_draw_background_mode) {
                line_bounds.right = globals().text_tab_stops[tab_index];
            }
        }

        pen_y = (int16_t)((font->ascending_height + font->descending_height +
            font->leading_height + extra_line_spacing) * (wrapped_sub_line_count + line_index) +
            font->ascending_height + bounds->top);
        pen.x = (int16_t)(font->leading_width + line_bounds.left);
        pen.y = pen_y;

        previous_token = -1;
        for (;;) {
            int wrapped = 0;

            token = narrow_text_strategy::instance().parse_next_token(&state);
            font = (Font *)state.font_definition;

            if (token == _text_token_break_character || token == _text_token_character) {
                FontCharacterTables *page = (FontCharacterTables *)font->character_tables.pointer +
                    (state.character >> 8);
                if (page->character_table.count >= 1) {
                    int16_t *character_index = (page->character_table.count ==
                        k_text_font_character_table_page_size)
                            ? (int16_t *)page->character_table.pointer + (state.character & 0xff)
                            : (int16_t *)0;
                    if (*character_index != -1) {
                        FontCharacter *character = (FontCharacter *)font->characters.pointer +
                            *character_index;
                        if (character != (FontCharacter *)0) {
                            if (token != _text_token_break_character &&
                                previous_token == _text_token_break_character) {
                                candidate_position = flush_end_position;
                                candidate_width = span_width;
                            }
                            if (character->bitmap_width + pen.x + span_width < line_bounds.right) {
                                span_width = (int16_t)(span_width + character->character_width);
                                flush_end_position = state.position;
                                previous_token = token;
                                continue;
                            }
                            if ((globals().hud_text_draw_unknown_4730 & _text_flag_word_wrap_bit) != 0) {
                                if (candidate_position > 0) {
                                    flush_end_position = candidate_position;
                                    span_width = candidate_width;
                                } else {
                                    flush_end_position = state.position;
                                }
                                previous_token = token;
                                wrapped = 1;
                            }
                        }
                    }
                }
            } else {
                flush_end_position = state.position;
                previous_token = token;
                wrapped = 1;
            }

            if (wrapped) {
                break;
            }

            flush_end_position = state.position;
            previous_token = token;
        }

        if (span_justification == _text_justification_right) {
            pen.x = (int16_t)(line_bounds.right - font->leading_width - span_width);
        } else if (span_justification == _text_justification_center) {
            pen.x = (int16_t)((((int16_t)(line_bounds.right - line_bounds.left) - span_width) >> 1) +
                line_bounds.left);
        }

        if ((globals().hud_text_draw_unknown_4730 & _text_flag_draw_past_bottom_bit) != 0 || pen_y < bounds->bottom) {
            narrow_text_strategy::instance().draw_character_range(&line_bounds, callback, &pen, clip, state.color, string, span_start_position, flush_end_position);
        }

        state.position = flush_end_position;

        switch (token) {
        case _text_token_newline:
            tab_index = 0;
            wrapped_sub_line_count = 0;
            line_index = (int16_t)(line_index + 1 + max_wrapped_sub_line_count);
            break;
        case _text_token_break_character:
        case _text_token_character:
            wrapped_sub_line_count = (int16_t)(wrapped_sub_line_count + 1);
            if (max_wrapped_sub_line_count < wrapped_sub_line_count) {
                max_wrapped_sub_line_count = wrapped_sub_line_count;
            }
            break;
        case _text_token_tab:
            if (tab_index < globals().hud_text_draw_background_mode) {
                tab_index = (int16_t)(tab_index + 1);
                wrapped_sub_line_count = 0;
            }
            break;
        case _text_token_justification:
            wrapped_sub_line_count = 0;
            break;
        default:
            break;
        }

        if (token == _text_token_end) {
            globals().text_highlight_end = 0;
            globals().text_highlight_start = 0;
            if (out_final_pen != (Point2DInt *)0) {
                *out_final_pen = pen;
            }
            return;
        }
    }
}

int16_t wide_text_strategy::parse_next_token(text_parse_state *state)
{
    uint16_t *string;
    int16_t position;
    uint16_t code;

    string = (uint16_t *)state->string;
    position = state->position;
    code = string[position];
    state->character = code;
    state->position = position + 1;

    switch (code) {
    case 0:
        state->token = _text_token_end;
        return state->token;
    case 9:
        state->token = _text_token_tab;
        return state->token;
    case 0xd:
        state->token = _text_token_newline;
        return state->token;
    case 0x7c:
        state->position = position + 2;
        if (string[position + 1] == 'n') {
            state->character = 0xd;
            state->token = _text_token_newline;
            return state->token;
        }
        state->position = position + 1;
        break;
    }
    state->token = _text_token_character;
    return state->token;
}

void wide_text_strategy::draw_character_range(Rectangle2D *bounds, text_glyph_draw_proc callback, Point2DInt *pen, Rectangle2D *clip, uint32_t color, void *string, int16_t start_column, int16_t end_column)
{
    text_parse_state state;

    int16_t left, right, top, bottom;

    left = k_text_coordinate_min;
    right = k_text_coordinate_max;
    top = k_text_coordinate_min;
    bottom = k_text_coordinate_max;
    if (bounds != (void *)0) {
        left = bounds->left;
        right = bounds->right;
        top = bounds->top;
        bottom = bounds->bottom;
    }
    if (clip != (void *)0) {
        if (clip->left > left) left = clip->left;
        if (clip->right < right) right = clip->right;
        if (clip->top > top) top = clip->top;
        if (clip->bottom < bottom) bottom = clip->bottom;
    }

    if (left < right && top < bottom) {
        text_context::parse_state_initialize(string, globals().hud_text_draw_column, globals().hud_text_draw_color_or_flags, &state, globals().hud_text_draw_font_tag_id, &globals().hud_text_draw_color_a);
        state.position = start_column;
        while (state.position < end_column) {
            uint32_t glyph_color;
            Font *font;
            FontCharacterTables *page;
            int16_t hardware_index;
            FontCharacter *character;

            glyph_color = (state.position < globals().text_highlight_start ||
                           globals().text_highlight_end <= state.position)
                              ? color : (color ^ k_text_rgb_mask);

            wide_text_strategy::instance().parse_next_token(&state);

            font = (Font *)state.font_definition;
            page = (FontCharacterTables *)font->character_tables.pointer + (state.character >> 8);
            if (0 < (int32_t)page->character_table.count) {
                int16_t *character_index = (page->character_table.count ==
                    k_text_font_character_table_page_size)
                        ? (int16_t *)page->character_table.pointer + (state.character & 0xff)
                        : (int16_t *)0;
                hardware_index = *character_index;
                if (hardware_index != -1) {
                    character = (FontCharacter *)font->characters.pointer + hardware_index;
                    if (character != (void *)0) {
                        int16_t draw_x, draw_y;

                        int16_t source_x, source_y, width, height;

                        draw_x = (int16_t)(pen->x - character->bitmap_origin_x);
                        draw_y = (int16_t)(pen->y - character->bitmap_origin_y);
                        pen->x = (int16_t)(pen->x + character->character_width);

                        source_x = 0;
                        source_y = 0;
                        width = character->bitmap_width;
                        height = character->bitmap_height;

                        if ((int32_t)draw_x + (int32_t)character->bitmap_width > right) {
                            width = (int16_t)(right - draw_x);
                        }
                        if (draw_x < left) {
                            source_x = (int16_t)(left - draw_x);
                            width = (int16_t)(width - source_x);
                            draw_x = left;
                        }

                        if ((int32_t)draw_y + (int32_t)character->bitmap_height > bottom) {
                            height = (int16_t)(bottom - draw_y);
                        }
                        if (draw_y < top) {
                            source_y = (int16_t)(top - draw_y);
                            height = (int16_t)(height - source_y);
                            draw_y = top;
                        }

                        if (0 < width && 0 < height) {
                            callback(&state, font, character, glyph_color, draw_x, draw_y,
                                source_x, source_y, width, height);
                        }
                    }
                }
            }
        }
    }
}

void wide_text_strategy::wrap_and_draw(text_glyph_draw_proc callback, Rectangle2D *bounds, Point2DInt *out_final_pen, Rectangle2D *clip, int16_t extra_line_spacing, void *string)
{
    text_parse_state state;
    Font *font;
    int16_t tab_index, line_index, max_wrapped_sub_line_count, wrapped_sub_line_count;
    Point2DInt pen;
    Rectangle2D line_bounds;

    tab_index = 0;
    line_index = 0;
    wrapped_sub_line_count = 0;
    max_wrapped_sub_line_count = 0;

    text_context::parse_state_initialize(string, globals().hud_text_draw_column, globals().hud_text_draw_color_or_flags, &state, globals().hud_text_draw_font_tag_id, &globals().hud_text_draw_color_a);
    font = (Font *)state.font_definition;

    for (;;) {
        int16_t span_start_position = state.position;
        int16_t span_justification = state.justification;
        int16_t span_width = 0;

        int16_t candidate_position = 0;
        int16_t candidate_width = 0;
        int16_t flush_end_position = 0;
        int16_t token, previous_token;
        int16_t pen_y;

        line_bounds = *bounds;

        if (globals().hud_text_draw_background_mode < 1) {
            line_bounds.left = (int16_t)(line_bounds.left +
                (line_index == 0 ? globals().ui_prompt_clip_x : globals().ui_prompt_clip_y));
        } else if (tab_index == 0) {
            line_bounds.left = (int16_t)(line_bounds.left +
                (line_index == 0 ? globals().ui_prompt_clip_x : globals().ui_prompt_clip_y));
            if (tab_index < globals().hud_text_draw_background_mode) {
                line_bounds.right = globals().text_tab_stops[tab_index];
            }
        } else {
            line_bounds.left = (&globals().hud_text_draw_background_mode)[tab_index];
            if (tab_index < globals().hud_text_draw_background_mode) {
                line_bounds.right = globals().text_tab_stops[tab_index];
            }
        }

        pen_y = (int16_t)((font->ascending_height + font->descending_height +
            font->leading_height + extra_line_spacing) * (wrapped_sub_line_count + line_index) +
            font->ascending_height + bounds->top);
        pen.x = (int16_t)(font->leading_width + line_bounds.left);
        pen.y = pen_y;

        previous_token = -1;
        for (;;) {
            int wrapped = 0;

            token = wide_text_strategy::instance().parse_next_token(&state);
            font = (Font *)state.font_definition;

            if (token == _text_token_break_character || token == _text_token_character) {
                FontCharacterTables *page = (FontCharacterTables *)font->character_tables.pointer +
                    (state.character >> 8);
                if (page->character_table.count >= 1) {
                    int16_t *character_index = (page->character_table.count ==
                        k_text_font_character_table_page_size)
                            ? (int16_t *)page->character_table.pointer + (state.character & 0xff)
                            : (int16_t *)0;
                    if (*character_index != -1) {
                        FontCharacter *character = (FontCharacter *)font->characters.pointer +
                            *character_index;
                        if (character != (FontCharacter *)0) {
                            if (token != _text_token_break_character &&
                                previous_token == _text_token_break_character) {
                                candidate_position = flush_end_position;
                                candidate_width = span_width;
                            }
                            if (character->bitmap_width + pen.x + span_width < line_bounds.right) {
                                span_width = (int16_t)(span_width + character->character_width);
                                flush_end_position = state.position;
                                previous_token = token;
                                continue;
                            }
                            if ((globals().hud_text_draw_unknown_4730 & _text_flag_word_wrap_bit) != 0) {
                                if (candidate_position > 0) {
                                    flush_end_position = candidate_position;
                                    span_width = candidate_width;
                                } else {
                                    flush_end_position = state.position;
                                }
                                previous_token = token;
                                wrapped = 1;
                            }
                        }
                    }
                }
            } else {
                flush_end_position = state.position;
                previous_token = token;
                wrapped = 1;
            }

            if (wrapped) {
                break;
            }

            flush_end_position = state.position;
            previous_token = token;
        }

        if (span_justification == _text_justification_right) {
            pen.x = (int16_t)(line_bounds.right - font->leading_width - span_width);
        } else if (span_justification == _text_justification_center) {
            pen.x = (int16_t)((((int16_t)(line_bounds.right - line_bounds.left) - span_width) >> 1) +
                line_bounds.left);
        }

        if ((globals().hud_text_draw_unknown_4730 & _text_flag_draw_past_bottom_bit) != 0 || pen_y < bounds->bottom) {
            wide_text_strategy::instance().draw_character_range(&line_bounds, callback, &pen, clip, state.color, string, span_start_position, flush_end_position);
        }

        state.position = flush_end_position;

        switch (token) {
        case _text_token_newline:
            tab_index = 0;
            wrapped_sub_line_count = 0;
            line_index = (int16_t)(line_index + 1 + max_wrapped_sub_line_count);
            break;
        case _text_token_break_character:
        case _text_token_character:
            wrapped_sub_line_count = (int16_t)(wrapped_sub_line_count + 1);
            if (max_wrapped_sub_line_count < wrapped_sub_line_count) {
                max_wrapped_sub_line_count = wrapped_sub_line_count;
            }
            break;
        case _text_token_tab:
            if (tab_index < globals().hud_text_draw_background_mode) {
                tab_index = (int16_t)(tab_index + 1);
                wrapped_sub_line_count = 0;
            }
            break;
        case _text_token_justification:
            wrapped_sub_line_count = 0;
            break;
        default:
            break;
        }

        if (token == _text_token_end) {
            globals().text_highlight_end = 0;
            globals().text_highlight_start = 0;
            if (out_final_pen != (Point2DInt *)0) {
                *out_final_pen = pen;
            }
            return;
        }
    }
}

static narrow_text_strategy g_narrow_text_strategy;

narrow_text_strategy &narrow_text_strategy::instance()
{
    return g_narrow_text_strategy;
}

static wide_text_strategy g_wide_text_strategy;

wide_text_strategy &wide_text_strategy::instance()
{
    return g_wide_text_strategy;
}

}  // namespace halo::text
