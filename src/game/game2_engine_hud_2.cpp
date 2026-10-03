#include "halo/game/game2_engine_hud.hpp"
#include "halo/game/constants.hpp"
#include "halo/game/records.hpp"
#include "halo/game/multiplayer_game_text.hpp"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"

static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &hud_globals_tag_data = halo::link::ref<void *>(halo::ui::vars().hud_globals_tag_data);
static auto &hud_text_draw_font_tag_id = halo::link::ref<uint32_t>(halo::ui::vars().hud_text_draw_font_tag_id);
static auto &hud_text_draw_color_a = halo::link::ref<uint32_t>(halo::ui::vars().hud_text_draw_color_a);
static auto &hud_text_draw_color_r = halo::link::ref<uint32_t>(halo::ui::vars().hud_text_draw_color_r);
static auto &hud_text_draw_color_g = halo::link::ref<uint32_t>(halo::ui::vars().hud_text_draw_color_g);
static auto &hud_text_draw_color_b = halo::link::ref<uint32_t>(halo::ui::vars().hud_text_draw_color_b);
static auto &hud_text_draw_color_or_flags = halo::link::ref<uint16_t>(halo::ui::vars().hud_text_draw_color_or_flags);
static auto &text_tab_stops = halo::link::ref<uint32_t>(halo::game::vars().text_tab_stops);
static auto &hud_text_draw_box_field_474e = halo::link::ref<uint32_t>(halo::game::vars().hud_text_draw_box_field_474e);
static auto &hud_text_draw_tabstop_c = halo::link::ref<uint32_t>(halo::game::vars().hud_text_draw_tabstop_c);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &game_engine_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_variant);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &render_viewport_top = halo::link::ref<uint32_t>(halo::ui::vars().render_viewport_top);
static auto &screen_safe_area_right = halo::link::ref<uint32_t>(halo::game::vars().screen_safe_area_right);
static auto &screen_safe_area_bottom = halo::link::ref<uint32_t>(halo::game::vars().screen_safe_area_bottom);
static auto &game_engine_post_game_fade = halo::link::ref<float>(halo::game::vars().game_engine_post_game_fade);

namespace halo::game {

/**
 * Renders the postgame carnage report / scoreboard overlay by formatting per-player or per-team score columns.
 */
void EngineHud::post_game_set_text_color(const float *color)
{
    hud_text_draw_color_a = color[0];
    hud_text_draw_color_r = color[1];
    hud_text_draw_color_g = color[2];
    hud_text_draw_color_b = color[3];
}

/**
 * The tab-stop / background state every scoreboard cell is drawn with (three int16 pairs, mode 6).
 */
void EngineHud::post_game_set_tab_stops(uint32_t stops_a, uint32_t stops_b, uint32_t stops_c)
{
    halo::text::globals().hud_text_draw_background_mode = 6;
    text_tab_stops = stops_a;
    hud_text_draw_box_field_474e = stops_b;
    hud_text_draw_tabstop_c = stops_c;
}

/**
 * Renders the postgame carnage-report overlay: an optional banner quad, two team-score lines when playing with
 * teams, the scoreboard column headers, one row per visible player (place, name, score text, kills / assists /
 * deaths), and a bottom prompt whose text depends on whether this machine hosts the session.
 *
 * @address 0x45d700
 */
void EngineHud::post_rasterize_post_game(void)
{
    float color_normal[4] = { 1.0f, 0.45882353f, 0.7294118f, 1.0f };
    float color_best[4] = { 1.0f, 0.98f, 0.96f, 0.96f };
    float color_local[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
    float color_team[2][4] = { { 1.0f, 0.8f, 0.4f, 0.4f },
                                  { 1.0f, 0.4f, 0.4f, 0.8f } };
    const uint32_t tab_a = 0x007d0032u, tab_b = 0x015e00fau, tab_c = 0x01f4019au;
    const uint32_t team_tab_a = 0x00c80032u, team_tab_b = 0x015e012cu, team_tab_c = 0x01f4019au;
    wchar_t line[0x100];
    wchar_t score_text[0x100];
    scoreboard_entry visible[16];
    int32_t visible_count;
    int32_t i;
    int32_t row;
    Rectangle2D rect;
    GlobalsInterfaceBitmaps *interface_bitmaps;
    HUDGlobals *hud_globals;
    Bitmap *quad_tag;
    wchar_t *col_a, *col_b, *col_c, *col_d, *col_e;

    if (current_game_engine == 0) {
        return;
    }

    hud_text_draw_font_tag_id = *(uint32_t *)&((HUDGlobals *)hud_globals_tag_data)->fullscreen_font.tag_id;
    post_game_set_text_color(color_normal);
    hud_text_draw_color_or_flags = 0xffffu;
    halo::text::globals().hud_text_draw_column = 0;
    halo::text::globals().hud_text_draw_unknown_4730 = 0;

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
        ? (GlobalsInterfaceBitmaps *)0
        : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    hud_globals = (HUDGlobals *)halo::cache::globals().tag_instances[interface_bitmaps->hud_globals.tag_id.index].data;
    quad_tag = (Bitmap *)halo::game::tag_data_at(halo::tag_id_bits<uint32_t>(hud_globals->carnage_report_bitmap.tag_id));
    rect.top = 0;
    rect.left = 0;
    rect.bottom = 0x1e0;
    rect.right = 0x280;
    if (quad_tag != 0 && (int32_t)quad_tag->bitmap_data.count > 0 && quad_tag->bitmap_data.pointer != 0) {
        halo::interface::ui_draw_screen_quad((int16_t *)&rect, (int16_t *)&rect, (int32_t)quad_tag->bitmap_data.pointer, 0, halo::k_dword_none);
    }

    if (game_engine_variant.teams != 0) {
        int32_t order[2];
        wchar_t *team_name[2];

        order[0] = 0;
        order[1] = 1;
        if (halo::game::game_engine_is_tracked_object_winner(0) == 0) {
            order[0] = 1;
            order[1] = 0;
        }
        team_name[0] = multiplayer_game_text_string(halo::game::mp_text::k_post_game_team_score_format_a);
        team_name[1] = multiplayer_game_text_string(halo::game::mp_text::k_post_game_team_score_format_b);

        halo::text::globals().hud_text_draw_background_mode = 6;
        text_tab_stops = team_tab_a;
        hud_text_draw_box_field_474e = team_tab_b;
        hud_text_draw_tabstop_c = team_tab_c;
        for (i = 0; i < 2; i++) {
            int32_t team = order[i];

            ((void (*)(int32_t, wchar_t *))current_game_engine->build_team_score_text)(team, score_text);
            halo::text::string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)team_name[team], score_text);
            line[0xff] = 0;
            halo::game::hud_draw_scoreboard_row_text((int16_t)(i + 4), line, 0);
        }
    }

    col_a = multiplayer_game_text_string(halo::game::mp_text::k_scoreboard_column_a);
    col_b = multiplayer_game_text_string(halo::game::mp_text::k_scoreboard_column_b);
    col_c = multiplayer_game_text_string(halo::game::mp_text::k_scoreboard_column_c);
    col_d = multiplayer_game_text_string(halo::game::mp_text::k_scoreboard_column_d);
    col_e = multiplayer_game_text_string(halo::game::mp_text::k_scoreboard_column_e);
    ((void (*)(wchar_t *))current_game_engine->build_score_header_text)(score_text);
    halo::text::string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L"\t%s\t%s\t%s\t%s\t%s\t%s"), col_a, col_b, score_text, col_c, col_d, col_e);
    post_game_set_tab_stops(tab_a, tab_b, tab_c);
    line[0xff] = 0;
    halo::game::hud_draw_scoreboard_row_text(7, line, 0);

    visible_count = halo::game::select_players_to_display(0, 0xc, visible);

    row = 8;
    for (i = 0; i < visible_count; i++) {
        datum_index player_handle = visible[i].player;
        player *p = halo::game::player_at(player_handle);
        int32_t place_index;
        wchar_t *place_text;

        post_game_set_text_color(p->local_player_index == -1 ? color_normal : color_local);
        post_game_set_tab_stops(tab_a, tab_b, tab_c);
        place_index = visible[i].place & 0x7f;
        if (place_index > 0xf) {
            place_index = 0xf;
        }
        place_text = multiplayer_game_text_string((int16_t)(place_index + 0x24));
        halo::text::string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t%s"), place_text);
        line[0xff] = 0;
        halo::game::hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        post_game_set_text_color(color_normal);
        if (game_engine_variant.teams != 0) {
            int32_t team = p->team;

            if (team < 0) {
                team = 0;
            } else if (team > 1) {
                team = 1;
            }
            post_game_set_text_color(color_team[team]);
        }
        halo::text::string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t \t%s"), p->name);
        line[0xff] = 0;
        halo::game::hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        post_game_set_text_color(color_normal);
        if (halo::game::game_engine_get_scoreboard_place(player_handle, 1, 0) == 0) {
            post_game_set_text_color(color_best);
        }
        ((void (*)(datum_index, wchar_t *))current_game_engine->build_player_text)(player_handle, score_text);
        halo::text::string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t \t \t%s"), score_text);
        line[0xff] = 0;
        halo::game::hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        post_game_set_text_color(color_normal);
        if (halo::game::game_engine_get_scoreboard_place(player_handle, 2, 0) == 0) {
            post_game_set_text_color(color_best);
        }
        halo::text::string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t \t \t \t%d"), (int32_t)p->kills);
        line[0xff] = 0;
        halo::game::hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        post_game_set_text_color(color_normal);
        if (halo::game::game_engine_get_scoreboard_place(player_handle, 3, 0) == 0) {
            post_game_set_text_color(color_best);
        }
        halo::text::string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t \t \t \t \t%d"), (int32_t)p->assists);
        line[0xff] = 0;
        halo::game::hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        post_game_set_text_color(color_normal);
        if (halo::game::game_engine_get_scoreboard_place(player_handle, 4, 0) == 0) {
            post_game_set_text_color(color_best);
        }
        halo::text::string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t \t \t \t \t \t%d"), (int32_t)p->deaths);
        line[0xff] = 0;
        halo::game::hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        post_game_set_tab_stops(tab_a, tab_b, tab_c);
        row = row + 1;
    }

    hud_text_draw_color_a = game_engine_post_game_fade;
    hud_text_draw_color_r = color_normal[1];
    hud_text_draw_color_g = color_normal[2];
    hud_text_draw_color_b = color_normal[3];
    halo::text::globals().hud_text_draw_background_mode = 0;

    {
        wchar_t *prompt;

        rect.top = (int16_t)(0x19a - (int32_t)render_viewport_top);
        rect.left = (int16_t)(screen_safe_area_right >> 16);
        rect.bottom = (int16_t)((int16_t)screen_safe_area_bottom - (int16_t)render_viewport_top);
        rect.right = (int16_t)((int16_t)(screen_safe_area_bottom >> 16) - (int16_t)(render_viewport_top >> 16));
        if (halo::networking::globals().server != 0) {
            rect.right = 0x118;
            prompt = multiplayer_game_text_string(halo::game::mp_text::k_post_game_prompt_host);
        } else {
            rect.right = 0x1a4;
            prompt = multiplayer_game_text_string(halo::game::mp_text::k_post_game_prompt_client);
        }
        halo::interface::ui_widget_draw_formatted_prompt_string(&rect, 0, (const uint16_t *)prompt);
    }
}

}
