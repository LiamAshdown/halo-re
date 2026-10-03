#include "halo/interface/ifr1_hud_messaging.hpp"
#include <string.h>
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"

extern "C" {
extern cinematic_globals *cinematic_globals_ptr;
extern player_globals *local_player_globals;
extern HUDGlobals *hud_messaging_parameters;
extern HUDGlobals *hud_globals_tag_data;
extern hud_messaging_globals *hud_messaging;
extern hud_globals_flags *hud_flags;
extern game_time_globals *game_time;
extern Scenario *global_scenario;
extern int16_t current_local_player_index;
extern int32_t hud_splitscreen_message_raise;
extern int16_t render_viewport_left;
extern Rectangle2D screen_safe_area_right;
extern int8_t hud_message_button_icon_table[0x1d];
extern const uint16_t *empty_wide_string_pointer;
extern int32_t hud_text_draw_font_tag_id;
extern uint32_t hud_text_draw_unknown_4730;
extern uint16_t hud_text_draw_color_or_flags;
extern int16_t hud_text_draw_column;
extern float hud_text_draw_color_a;
extern float hud_text_draw_color_r;
extern float hud_text_draw_color_g;
extern float hud_text_draw_color_b;
extern int16_t ui_prompt_clip_x;
extern int16_t ui_prompt_clip_y;
extern const uint16_t hud_text_quote[];
extern const uint16_t hud_text_unbound[];
extern const uint16_t hud_text_unknown[];
extern const uint16_t hud_text_no_button_icon[];
extern int32_t __ftol(double x);
extern double pow(double base, double exponent);
extern uint8_t game_engine_local_player_score_is_nonpositive(datum_index player_index);
extern void color_argb_int_to_real(ColorARGB *out, uint32_t packed);
extern uint32_t color_pack_argb_from_real(ColorARGB *color);
extern void hud_anchor_offset_to_screen_position(uint16_t *anchor, uint8_t has_scale, float scale,
                                                 const int16_t *offset, int16_t *out, int32_t selector);
extern uint32_t hud_meter_flash_color_blend(const hud_flash_parameters *flash, int32_t start_time);
extern void hud_draw_message_text_span(Rectangle2D *cursor, Rectangle2D *origin, const uint16_t *text,
                                       uint8_t allow_button_prompts);
extern void hud_draw_message_icon(const hud_messaging_information *information, Rectangle2D *cursor,
                                  uint32_t color);
extern int32_t hud_message_compare(const void *a, const void *b);
extern void text_measure_string_extents(Rectangle2D *origin, Rectangle2D *cursor, Rectangle2D *out_bounds,
                         const uint16_t *text);
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0,
                                      int32_t unknown_1, const uint16_t *text);
extern uint16_t *text_string_list_get_string(datum_index string_list_tag, int16_t index);
extern wchar_t *string_format_wide_va(wchar_t *dest, const wchar_t *format, ...);
}

static void hud_messaging_set_text_state(datum_index font, const ColorARGB *color)
{
    hud_text_draw_font_tag_id = (int32_t)font;
    hud_text_draw_color_a = color->alpha;
    hud_text_draw_color_r = color->red;
    hud_text_draw_color_g = color->green;
    hud_text_draw_color_b = color->blue;
    hud_text_draw_color_or_flags = 0xffff;
    hud_text_draw_column = 0;
    hud_text_draw_unknown_4730 = 0;
}

static void hud_messaging_draw_button_icon(int16_t button_icon, Rectangle2D *cursor, Rectangle2D *line)
{
    uint8_t binding[12];
    uint16_t name[0x40];

    if ((int32_t)button_icon >= (int32_t)hud_globals_tag_data->button_icons.count) {
        hud_draw_message_text_span(cursor, line, hud_text_no_button_icon, 0);
        return;
    }
    if (halo::input::input_get_last_used_binding(button_icon, (control_binding_descriptor *)binding) == 0) {
        hud_draw_message_text_span(cursor, line, hud_text_unbound, 0);
        return;
    }
    halo::input::input_get_binding_display_name((control_binding_descriptor *)binding, name);
    hud_draw_message_text_span(cursor, line, hud_text_quote, 0);
    hud_draw_message_text_span(cursor, line, name, 0);
    hud_draw_message_text_span(cursor, line, hud_text_quote, 0);
}

namespace halo::interface {

/**
 * Original engine function hud_messaging_update; the author notes are in
 * docs/original/interface/hud_messaging_update.txt.
 * blam-cc: local_player_index -> AX
 *
 * @address 0x4ae550
 */
void HudMessaging::messaging_update(int16_t local_player_index)
{
    HUDGlobals *parameters;
    hud_player_messaging_state *record;
    datum_index player_index;
    datum_index font;
    Font *font_tag;
    Point2DInt origin;
    Rectangle2D cursor;
    Rectangle2D line;
    Rectangle2D bounds;
    ColorARGB color;
    uint32_t packed_color;
    int32_t line_height;
    int16_t y;
    int16_t max_lines;
    int16_t i;
    uint8_t split_screen;
    uint8_t objective_shown;
    uint8_t help_shown;
    uint8_t action_shown;
    int32_t now;

    if (cinematic_globals_ptr->in_progress != 0 || local_player_index == -1) {
        return;
    }
    player_index = local_player_index < 1 ? local_player_globals->local_players[local_player_index]
                                          : (datum_index)-1;
    if (game_engine_local_player_score_is_nonpositive(player_index) == 0) {
        return;
    }

    parameters = hud_messaging_parameters;
    split_screen = local_player_globals->local_player_count > 1;
    font = *(datum_index *)&parameters->fullscreen_font.tag_id;
    if (local_player_globals->local_player_count > 1 &&
        *(datum_index *)&parameters->splitscreen_font.tag_id != (datum_index)-1) {
        font = *(datum_index *)&parameters->splitscreen_font.tag_id;
    }
    hud_anchor_offset_to_screen_position((uint16_t *)&parameters->anchor, split_screen, 0.0f,
                                         &parameters->anchor_offset.x, &origin.x, 0);
    font_tag = (Font *)halo::cache::globals().tag_instances[font & 0xffff].data;
    y = origin.y;
    if (split_screen) {
        line_height = (uint16_t)(font_tag->leading_height + font_tag->ascending_height);
        y = (int16_t)(y - hud_splitscreen_message_raise);
    } else {
        line_height = (uint16_t)(font_tag->leading_height + font_tag->descending_height + font_tag->ascending_height);
    }

    record = &hud_messaging->players[current_local_player_index];
    max_lines = (int16_t)(4 - (local_player_globals->local_player_count > 1));
    objective_shown = hud_messaging->objective_text != 0 && hud_messaging->objective_text_ticks != 0;
    help_shown = hud_flags->help_text_shown != 0 && hud_messaging->help_text != 0;
    action_shown = record->message_shown != 0 && (record->message != 0 || record->action_text[0] != 0);

    if (objective_shown || help_shown || action_shown) {
        HUDMessageText *messages_tag = 0;
        HUDMessageTextMessage *message = 0;
        uint16_t text_offset;
        int32_t element_index;

        if (objective_shown) {
            HUDGlobals *globals = hud_globals_tag_data;
            float fraction;

            now = game_time->game_time;
            packed_color = hud_meter_flash_color_blend(
                (const hud_flash_parameters *)&globals->objective_default_color,
                hud_messaging->objective_text_ticks - globals->objective_uptime_ticks - globals->objective_fade_ticks + now);
            color_argb_int_to_real(&color, packed_color);
            fraction = (float)hud_messaging->objective_text_ticks / (float)globals->objective_fade_ticks;
            if (fraction > 1.0f) {
                fraction = 1.0f;
            }
            color.alpha = fraction * color.alpha;
            packed_color = color_pack_argb_from_real(&color);
        } else if (help_shown) {
            if (hud_messaging->help_text_flashing != 0) {
                packed_color = hud_meter_flash_color_blend(
                    (const hud_flash_parameters *)&hud_globals_tag_data->hud_help_default_color,
                    hud_messaging->help_text_flash_start_time);
            } else if ((*(uint8_t *)&hud_globals_tag_data->hud_help_flash_flags & 1) != 0) {
                packed_color = *(uint32_t *)&hud_globals_tag_data->hud_help_flashing_color;
            } else {
                packed_color = *(uint32_t *)&hud_globals_tag_data->hud_help_default_color;
            }
            color_argb_int_to_real(&color, packed_color);
        } else {
            color = parameters->icon_color;
            packed_color = color_pack_argb_from_real(&color);
        }

        line.top = y;
        line.left = origin.x;
        line.bottom = (int16_t)(y + line_height * 5);
        line.right = (int16_t)(screen_safe_area_right.right - render_viewport_left);
        cursor = line;
        hud_messaging_set_text_state(font, &color);

        if (objective_shown) {
            int32_t remaining = hud_messaging->objective_text_ticks - game_time->ticks_this_frame;
            messages_tag = (HUDMessageText *)halo::cache::globals().tag_instances[*(datum_index *)&global_scenario->hud_messages.tag_id & 0xffff].data;
            message = hud_messaging->objective_text;
            hud_messaging->objective_text_ticks = (int16_t)(remaining > 0 ? remaining : 0);
        } else if (help_shown) {
            messages_tag = (HUDMessageText *)halo::cache::globals().tag_instances[*(datum_index *)&global_scenario->hud_messages.tag_id & 0xffff].data;
            message = hud_messaging->help_text;
        } else if (record->message != 0) {
            messages_tag = (HUDMessageText *)halo::cache::globals().tag_instances[*(datum_index *)&hud_globals_tag_data->hud_messages.tag_id & 0xffff].data;
            message = record->message;
        } else if (record->action_text[0] != 0) {
            hud_draw_message_text_span(&cursor, &line, record->action_text, 1);
        }

        if (message != 0 && message->panel_count != 0) {
            text_offset = message->start_index_into_text_blob;
            for (element_index = 0; (int16_t)element_index < (int32_t)message->panel_count; element_index++) {
                HUDMessageTextElement *element = (HUDMessageTextElement *)messages_tag->message_elements.pointer +
                                                 (message->start_index_of_message_block + element_index);
                uint8_t data = element->data;

                if (element->type == 0) {
                    const uint16_t *text = (const uint16_t *)messages_tag->text_data.pointer + text_offset;

                    ui_prompt_clip_x = (int16_t)(cursor.left - line.left);
                    ui_prompt_clip_y = 0;
                    text_measure_string_extents(&line, &cursor, &bounds, text);
                    cursor.left = (int16_t)(cursor.left - 3);
                    bounds.left = line.left;
                    chimera__draw_16_bit_text(0, &bounds, 0, 0, text);
                    line.top = cursor.top;
                    text_offset = (uint16_t)(text_offset + data);
                } else if (data <= 0x11) {
                    if (data == 0) {
                        hud_messaging_draw_button_icon(2, &cursor, &line);
                    } else if (data == 1) {
                        hud_messaging_draw_button_icon(9, &cursor, &line);
                    }
                } else if (data <= 0x1f) {
                    if (data <= 0x1c && hud_message_button_icon_table[data] != -1) {
                        hud_messaging_draw_button_icon(hud_message_button_icon_table[data], &cursor, &line);
                    }
                } else if (hud_flags->help_text_shown == 0) {
                    int16_t argument = (int16_t)(data - 0x20);

                    if ((record->argument_is_string & (uint8_t)(1 << argument)) != 0) {
                        uint8_t *reference = (uint8_t *)&record->arguments[argument];
                        uint16_t string_index = *(uint16_t *)reference;

                        if (string_index == 0xffff) {
                            hud_draw_message_text_span(&cursor, &line, hud_text_unknown, 0);
                        } else if (reference[2] != 0) {
                            hud_draw_message_text_span(&cursor, &line,
                                text_string_list_get_string(*(datum_index *)&global_scenario->custom_object_names.tag_id,
                                                            (int16_t)string_index), 0);
                        } else {
                            hud_draw_message_text_span(&cursor, &line,
                                text_string_list_get_string(*(datum_index *)&hud_globals_tag_data->alternate_icon_text.tag_id,
                                                            (int16_t)string_index), 0);
                        }
                    } else if (record->arguments[argument] != 0) {
                        hud_draw_message_icon((const hud_messaging_information *)record->arguments[argument], &cursor,
                                              packed_color);
                    }
                }
            }
        }
        ui_prompt_clip_x = 0;
        ui_prompt_clip_y = 0;
        y = cursor.bottom;
    }

    if (!objective_shown && !help_shown && (record->message_shown != 0 || record->prompt_changed != 0)) {
        if (split_screen) {
            y = (int16_t)__ftol((double)((float)(origin.y - hud_splitscreen_message_raise) +
                                         (float)line_height * parameters->text_spacing));
        } else {
            y = (int16_t)__ftol((double)((float)line_height * parameters->text_spacing + (float)origin.y));
        }
        max_lines--;
    }

    qsort(record->messages, 4, sizeof(hud_message_slot), hud_message_compare);
    for (i = 0; i < max_lines; i++) {
        hud_message_slot *slot = &record->messages[i];
        float elapsed;
        float up_ticks;

        if (slot->active == 0) {
            return;
        }
        now = game_time->game_time;
        color = parameters->text_color;
        elapsed = (float)(now - slot->timestamp);
        up_ticks = parameters->up_time * 30.0f;
        if (elapsed > up_ticks) {
            float t = 1.0f - (elapsed - up_ticks) / (parameters->fade_time * 30.0f);
            if (t < 0.0f) {
                t = 0.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
            }
            color.alpha = (float)pow((double)t, 1.899999976158142) * color.alpha;
        }

        cursor = screen_safe_area_right;
        cursor.right = (int16_t)(screen_safe_area_right.right - render_viewport_left);
        cursor.left = origin.x;
        cursor.top = y;
        cursor.bottom = (int16_t)(line_height + y);
        y = (int16_t)__ftol((double)((float)line_height * parameters->text_spacing + (float)y));
        hud_messaging_set_text_state(font, &color);

        if (slot->source == -1) {
            chimera__draw_16_bit_text(0, &cursor, 0, 0, slot->text);
        } else {
            Item *item;
            const uint16_t *text;
            uint8_t plural;
            int32_t string_index;
            datum_index strings = *(datum_index *)&hud_globals_tag_data->item_message_text.tag_id;

            if (slot->source_kind == 0xff) {
                plural = slot->count > 1;
            } else {
                plural = slot->source_kind;
            }
            item = (Item *)halo::cache::globals().tag_instances[slot->source & 0xffff].data;
            string_index = (int16_t)((int8_t)plural + item->pickup_text_index);
            text = empty_wide_string_pointer;
            if (strings != (datum_index)-1) {
                int32_t *string_list = (int32_t *)halo::cache::globals().tag_instances[strings & 0xffff].data;
                if (string_list != 0 && string_index >= 0 && string_index < string_list[0]) {
                    text = text_string_list_get_string(strings, (int16_t)string_index);
                }
            }
            if ((slot->source_kind == 0xff && plural != 0) || slot->count != 0) {
                wchar_t formatted[0x80];
                int32_t value_scale = (int16_t)item->hud_message_value_scale;
                if (value_scale <= 1) {
                    value_scale = 1;
                }
                string_format_wide_va(formatted, (const wchar_t *)text, slot->count / value_scale);
                chimera__draw_16_bit_text(0, &cursor, 0, 0, (const uint16_t *)formatted);
            } else {
                chimera__draw_16_bit_text(0, &cursor, 0, 0, text);
            }
        }

        slot->active = (parameters->fade_time + parameters->up_time) * 30.0f > (float)(now - slot->timestamp);
        if (slot->active == 0) {
            slot->timestamp = -1;
        }
    }
}

}
