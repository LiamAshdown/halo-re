// game_engine_post_rasterize_post_game  (Ghidra: game_engine_post_rasterize_post_game, already
// named)
// address 0x45d700, size 3024 bytes
// VERIFIED against disassembly 0x45d700..0x45e2d2 (2026-09-30)
// name confidence: 0.8   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Renders the postgame carnage report / scoreboard
// overlay by formatting per-player or per-team score columns"); types/game.h game_engine_definition
// (build_team_score_text +0x5c, get_team_score, build_score_header_text +0x58, build_player_text +0x54); scoreboard_entry;
// select_players_to_display.c, hud_draw_scoreboard_row_text.c, game_engine_get_scoreboard_place.c (this batch).
// REWRITTEN 2026-09-30 from the disassembly (the earlier draft was "rewrite confidence 0.2"). What was wrong / is now pinned:
//  - The hud text draw state's tab-stop globals (0x6e474a / 0x6e474e / 0x6e4752) receive the VALUES of three int16 pairs
//    (main set (0x32,0x7d) (0xfa,0x15e) (0x19a,0x1f4); team-line set (0x32,0xc8) (0x12c,0x15e) (0x19a,0x1f4)), and the background mode at
//    0x6e4748 is a WORD 6; the draft stored addresses of locals into them.
//  - Row indices: the two team lines are rows 4 and 5 (in winner-first order), the header row is row 7 and the per-player rows start at
//    row 8 and count up by one per player (all cells of a player share the row); the draft drew everything at row 0.
//  - Each string is `tag_lookup + text_string_list_get_string(N)`: team names 0x41 / 0x42, header columns 0x43..0x47, place 0x24 + min(place & 0x7f, 15),
//    prompt 0x48 (hosting) / 0x49 (client). The winner query is game_engine_is_tracked_object_winner(team 0) (EBX = 0).
//  - The banner is drawn with ui_draw_screen_quad(EAX = ECX = screen rect {0,0,0x1e0,0x280}, bitmap = hud_globals->[+0x3d4 tag]->[+0x64], 0, -1); the
//    draft skipped the +0x3d4 tag hop and dropped the rect arguments.
//  - The prompt is ui_widget_draw_formatted_prompt_string(EDX = text, stack (&rect, 0)) with rect {top 0x19a - viewport_top, left = safe_right >> 16,
//    bottom = safe_bottom_low - viewport_top_low, right = 0x118 (hosting) or 0x1a4}; the draft passed an invalid "banner" dword and no text.
//  - string_format_wide_va_bounded takes its count in EDX (0x100 here).
// The first-listed team is the winner unless game_engine_is_tracked_object_winner(0) returns 0, in which case team 1 is listed first.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>
#include "objects.h"
#include "units.h"
#include "networking.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern void *hud_globals_tag_data;                      // 0x0071941c, the hud_globals tag data
extern uint32_t hud_text_draw_font_tag_id;                // 0x006e472c
extern uint32_t hud_text_draw_color_a;             // 0x006e4738 (float bits are copied verbatim)
extern uint32_t hud_text_draw_color_r;             // 0x006e473c
extern uint32_t hud_text_draw_color_g;             // 0x006e4740
extern uint32_t hud_text_draw_color_b;             // 0x006e4744
extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734, two separate int16 slots in the
extern int16_t hud_text_draw_column;         // 0x006e4736  binary, never one dword
extern uint32_t hud_text_draw_unknown_4730;         // 0x006e4730
extern int16_t hud_text_draw_background_mode;         // 0x006e4748 (word store, 6 here)
extern uint32_t text_tab_stops;               // 0x006e474a (two int16 tab stops)
extern uint32_t hud_text_draw_box_field_474e;               // 0x006e474e
extern uint32_t hud_text_draw_tabstop_c;               // 0x006e4752
extern Globals *global_globals;                     // 0x00746fa0
extern tag_instance *tag_instances;                 // 0x0087bc14
extern game_variant game_engine_variant;             // 0x006f1c88 (::teams at +0x34, 0x006f1cbc)
extern data_array *player_data;                      // 0x0087a480
extern uint32_t render_viewport_top;             // 0x007c3140
extern uint32_t screen_safe_area_right;              // 0x007c3148
extern uint32_t screen_safe_area_bottom;             // 0x007c314c
extern float game_engine_post_game_fade;             // 0x0087aa0c
extern network_server_globals *network_server;      // 0x0071c2d4
extern wchar_t empty_string;                          // 0x00660c34

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, EDX count
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0, blam-cc: ECX tag_id, DX index
// tag_lookup("ui\\multiplayer_game_text") + text_string_list_get_string(tag, index), or the empty string when the tag is missing.
// (This is the inlined `lookup + get_string(N)` pair the disassembly shows at every use; NOT the function at 0x45ce90, which
// is game_engine_get_default_multiplayer_string(entry) == get_place_string.)
static wchar_t *multiplayer_game_text_string(int16_t index)
{
    datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text"); // 'ustr'

    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return text_string_list_get_string(tag_id, index);
}
extern int32_t game_engine_get_scoreboard_place(datum_index player, int32_t mode,
    uint8_t invert_low_stat); // 0x45d440; blam-cc: EDI -> player, EAX -> mode (1 score, 2 kills, 3 assists, 4 deaths), stack -> invert_low_stat
extern int32_t select_players_to_display(int32_t mode, int32_t max_count,
    scoreboard_entry *out); // 0x45d4a0; blam-cc: EAX mode, EBX max_count, stack out
extern void hud_draw_scoreboard_row_text(int16_t row, wchar_t *text, int16_t column); // 0x45d670; blam-cc: CX row, stack (text, column)
extern uint32_t game_engine_is_tracked_object_winner(int32_t team); // 0x463730, blam-cc: EBX team
extern void ui_draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data,
    int16_t *clip_rect, uint32_t vertex_color); // 0x498b20; blam-cc: EAX source_rect, ECX dest_rect, stack (bitmap_data, clip_rect, vertex_color)
extern void ui_widget_draw_formatted_prompt_string(Rectangle2D *bounds, uint8_t use_text_color, const uint16_t *text); // 0x49ade0, blam-cc: stack (bounds, use_text_color), EDX text

static void post_game_set_text_color(const uint32_t *color)
{
    hud_text_draw_color_a = color[0];
    hud_text_draw_color_r = color[1];
    hud_text_draw_color_g = color[2];
    hud_text_draw_color_b = color[3];
}

// The tab-stop / background state every scoreboard cell is drawn with (three int16 pairs, mode 6).
static void post_game_set_tab_stops(uint32_t stops_a, uint32_t stops_b, uint32_t stops_c)
{
    hud_text_draw_background_mode = 6;
    text_tab_stops = stops_a;
    hud_text_draw_box_field_474e = stops_b;
    hud_text_draw_tabstop_c = stops_c;
}

// Renders the postgame carnage-report overlay: an optional banner quad, two team-score lines when playing with teams, the scoreboard
// column headers, one row per visible player (place, name, score text, kills / assists / deaths), and a bottom prompt whose text depends on
// whether this machine hosts the session.
void game_engine_post_rasterize_post_game(void)
{
    // F+0x18 / F+0x28 / F+0x70: (alpha, red, green, blue) as raw float bits
    uint32_t color_normal[4] = { 0x3f800000, 0x3eeaeaeb, 0x3f3ababb, 0x3f800000 };
    uint32_t color_best[4] = { 0x3f800000, 0x3f7ae148, 0x3f75c28f, 0x3f75c28f };
    uint32_t color_local[4] = { 0x3f800000, 0x3f800000, 0x3f800000, 0 };
    uint32_t color_team[2][4] = { { 0x3f800000, 0x3f4ccccd, 0x3ecccccd, 0x3ecccccd },
                                  { 0x3f800000, 0x3ecccccd, 0x3ecccccd, 0x3f4ccccd } };
    const uint32_t tab_a = 0x007d0032u, tab_b = 0x015e00fau, tab_c = 0x01f4019au;           // (0x32,0x7d) (0xfa,0x15e) (0x19a,0x1f4)
    const uint32_t team_tab_a = 0x00c80032u, team_tab_b = 0x015e012cu, team_tab_c = 0x01f4019au; // (0x32,0xc8) (0x12c,0x15e) (0x19a,0x1f4)
    wchar_t line[0x100];
    wchar_t score_text[0x100];
    scoreboard_entry visible[16];
    int32_t visible_count;
    int32_t i;
    int32_t row;
    Rectangle2D rect;
    GlobalsInterfaceBitmaps *interface_bitmaps;
    uint8_t *hud_globals;
    uint8_t *quad_tag;
    wchar_t *col_a, *col_b, *col_c, *col_d, *col_e;

    if (current_game_engine == 0) {
        return;
    }

    hud_text_draw_font_tag_id = *(uint32_t *)((uint8_t *)hud_globals_tag_data + 0x54);
    post_game_set_text_color(color_normal);
    hud_text_draw_color_or_flags = 0xffffu;
    hud_text_draw_column = 0;
    hud_text_draw_unknown_4730 = 0;

    // Banner: globals -> interface_bitmaps[0].hud_globals -> tag at +0x3d4 -> reflexive at +0x60 whose pointer (+0x64) is the bitmap
    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
        ? (GlobalsInterfaceBitmaps *)0
        : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    hud_globals = (uint8_t *)tag_instances[interface_bitmaps->hud_globals.tag_id.index].data;
    quad_tag = (uint8_t *)tag_instances[*(uint32_t *)(hud_globals + 0x3d4) & 0xffff].data;
    rect.top = 0;
    rect.left = 0;
    rect.bottom = 0x1e0;
    rect.right = 0x280;
    if (quad_tag != 0 && *(int32_t *)(quad_tag + 0x60) > 0 && *(int32_t *)(quad_tag + 0x64) != 0) {
        ui_draw_screen_quad((int16_t *)&rect, (int16_t *)&rect, *(int32_t *)(quad_tag + 0x64), 0, 0xffffffffu);
    }

    if (game_engine_variant.teams != 0) {
        int32_t order[2];
        wchar_t *team_name[2];

        order[0] = 0;
        order[1] = 1;
        if (game_engine_is_tracked_object_winner(0) == 0) {
            order[0] = 1;
            order[1] = 0;
        }
        team_name[0] = multiplayer_game_text_string(0x41);
        team_name[1] = multiplayer_game_text_string(0x42);

        hud_text_draw_background_mode = 6;
        text_tab_stops = team_tab_a;
        hud_text_draw_box_field_474e = team_tab_b;
        hud_text_draw_tabstop_c = team_tab_c;
        for (i = 0; i < 2; i++) {
            int32_t team = order[i];

            ((void (*)(int32_t, wchar_t *))current_game_engine->build_team_score_text)(team, score_text);
            string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)team_name[team], score_text);
            line[0xff] = 0;
            hud_draw_scoreboard_row_text((int16_t)(i + 4), line, 0);
        }
    }

    col_a = multiplayer_game_text_string(0x43);
    col_b = multiplayer_game_text_string(0x44);
    col_c = multiplayer_game_text_string(0x45);
    col_d = multiplayer_game_text_string(0x46);
    col_e = multiplayer_game_text_string(0x47);
    ((void (*)(wchar_t *))current_game_engine->build_score_header_text)(score_text);
    string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L"\t%s\t%s\t%s\t%s\t%s\t%s"), col_a, col_b, score_text, col_c, col_d, col_e);
    post_game_set_tab_stops(tab_a, tab_b, tab_c);
    line[0xff] = 0;
    hud_draw_scoreboard_row_text(7, line, 0);

    visible_count = select_players_to_display(0, 0xc, visible);

    row = 8;
    for (i = 0; i < visible_count; i++) {
        datum_index player_handle = visible[i].player;
        player *p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
        int32_t place_index;
        wchar_t *place_text;

        // place text: color depends on whether this is a local player
        post_game_set_text_color(p->local_player_index == -1 ? color_normal : color_local);
        post_game_set_tab_stops(tab_a, tab_b, tab_c);
        place_index = visible[i].place & 0x7f;
        if (place_index > 0xf) {
            place_index = 0xf;
        }
        place_text = multiplayer_game_text_string((int16_t)(place_index + 0x24));
        string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t%s"), place_text);
        line[0xff] = 0;
        hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        // name, tinted by team in team games
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
        string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t \t%s"), p->name);
        line[0xff] = 0;
        hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        // score text (the engine's own player text), highlighted when this player leads the score column
        post_game_set_text_color(color_normal);
        if (game_engine_get_scoreboard_place(player_handle, 1, 0) == 0) {
            post_game_set_text_color(color_best);
        }
        ((void (*)(datum_index, wchar_t *))current_game_engine->build_player_text)(player_handle, score_text);
        string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t \t \t%s"), score_text);
        line[0xff] = 0;
        hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        // kills, assists, deaths
        post_game_set_text_color(color_normal);
        if (game_engine_get_scoreboard_place(player_handle, 2, 0) == 0) {
            post_game_set_text_color(color_best);
        }
        string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t \t \t \t%d"), (int32_t)p->kills);
        line[0xff] = 0;
        hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        post_game_set_text_color(color_normal);
        if (game_engine_get_scoreboard_place(player_handle, 3, 0) == 0) {
            post_game_set_text_color(color_best);
        }
        string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t \t \t \t \t%d"), (int32_t)p->assists);
        line[0xff] = 0;
        hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        post_game_set_text_color(color_normal);
        if (game_engine_get_scoreboard_place(player_handle, 4, 0) == 0) {
            post_game_set_text_color(color_best);
        }
        string_format_wide_va_bounded(0x100, (uint16_t *)line, (const uint16_t *)(L" \t \t \t \t \t \t%d"), (int32_t)p->deaths);
        line[0xff] = 0;
        hud_draw_scoreboard_row_text((int16_t)row, line, 0);

        post_game_set_tab_stops(tab_a, tab_b, tab_c);
        row = row + 1;
    }

    // Prompt: fades in with the post-game fade, no background, drawn through the formatted-prompt path
    hud_text_draw_color_a = *(uint32_t *)&game_engine_post_game_fade;
    hud_text_draw_color_r = color_normal[1];
    hud_text_draw_color_g = color_normal[2];
    hud_text_draw_color_b = color_normal[3];
    hud_text_draw_background_mode = 0;

    {
        wchar_t *prompt;

        rect.top = (int16_t)(0x19a - (int32_t)render_viewport_top);
        rect.left = (int16_t)(screen_safe_area_right >> 16);
        rect.bottom = (int16_t)((int16_t)screen_safe_area_bottom - (int16_t)render_viewport_top);
        rect.right = (int16_t)((int16_t)(screen_safe_area_bottom >> 16) - (int16_t)(render_viewport_top >> 16));
        if (network_server != 0) {
            rect.right = 0x118;
            prompt = multiplayer_game_text_string(0x48);
        } else {
            rect.right = 0x1a4;
            prompt = multiplayer_game_text_string(0x49);
        }
        ui_widget_draw_formatted_prompt_string(&rect, 0, (const uint16_t *)prompt);
    }
}

#if 0
Original Ghidra decompilation (0x45d700), from tools/pack.py 0x45d700:

/* WARNING: Removing unreachable block (ram,0x0045dd28) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void game_engine_post_rasterize_post_game(void)

{
  uint *puVar1;
  int *piVar2;
  undefined *puVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  short sVar7;
  undefined4 uVar8;
  undefined **ppuVar9;
  undefined **ppuVar10;
  undefined4 local_630;
  undefined4 local_62c;
  undefined **local_628;
  undefined **local_624;
  undefined **local_620;
  undefined **local_61c;
  undefined **local_618;
  undefined **local_614;
  undefined **local_610;
  undefined **local_60c;
  undefined **local_608;
  undefined4 local_604;
  undefined4 local_600;
  undefined **local_5fc [2];
  uint uStack_5f4;
  undefined **local_5f0 [12];
  undefined1 auStack_5c0 [510];
  undefined2 uStack_3c2;
  undefined1 local_3c0 [512];
  undefined1 auStack_1c0 [24];
  undefined *apuStack_1a8 [106];

  if (DAT_006f1d20 != 0) {
    DAT_006e472c = *(undefined4 *)(DAT_0071941c + 0x54);
    local_628 = (undefined **)0x3f800000;
    local_624 = (undefined **)0x3eeaeaeb;
    DAT_006e4738 = (undefined **)0x3f800000;
    local_620 = (undefined **)0x3f3ababb;
    local_61c = (undefined **)0x3f800000;
    DAT_006e473c = (undefined **)0x3eeaeaeb;
    DAT_006e4740 = (undefined **)0x3f3ababb;
    DAT_006e4744 = (undefined **)0x3f800000;
    DAT_006e4734._0_2_ = 0xffff;
    DAT_006e4734._2_2_ = 0;
    _DAT_006e4730 = 0;
    local_608 = (undefined **)0x7d0032;
    local_604 = 0x15e00fa;
    local_600 = 0x1f4019a;
    local_5f0[9] = (undefined **)0x3f800000;
    local_5f0[10] = (undefined **)0x3f800000;
    local_5f0[0xb] = (undefined **)0x0;
    local_5f0[8] = (undefined **)0x3f800000;
    local_614 = (undefined **)0x3f7ae148;
    local_610 = (undefined **)0x3f75c28f;
    local_60c = (undefined **)0x3f75c28f;
    local_618 = (undefined **)0x3f800000;
    if (*(int *)(DAT_00746fa0 + 0x140) == 0) {
      iVar4 = 0;
    }
    else {
      iVar4 = *(int *)(DAT_00746fa0 + 0x144);
    }
    iVar4 = *(int *)((*(uint *)(iVar4 + 0x6c) & 0xffff) * 0x20 + 0x14 +
                    DAT_0087bc14);
    local_630 = 0;
    local_62c = 0x28001e0;
    if (((iVar4 != 0) && (0 < *(int *)(iVar4 + 0x60))) && (*(int *)(iVar4 + 100) != 0)) {
      uVar8 = 0;
      if ((iVar4 != 0) && (0 < *(int *)(iVar4 + 0x60))) {
        uVar8 = *(undefined4 *)(iVar4 + 100);
      }
      FUN_00498b20(uVar8,0,0xffffffff);
    }
    if (DAT_006f1cbc != '\0') {
      local_630 = 0;
      local_5f0[0] = (undefined **)0xc80032;
      local_5f0[1] = (undefined **)0x15e012c;
      local_5f0[2] = (undefined **)0x1f4019a;
      local_62c = 1;
      iVar4 = FUN_00463730();
      uVar5 = tag_lookup("ui\\multiplayer_game_text");
      if (uVar5 == 0xffffffff) {
        local_5fc[0] = (undefined **)&DAT_00660c34;
      }
      else {
        piVar2 = *(int **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        local_5fc[0] = &PTR_DAT_00671fac;
        if (0x41 < *piVar2) {
          iVar6 = piVar2[1];
          uVar5 = *(uint *)(iVar6 + 0x514);
          if (0 < (int)uVar5) {
            local_5fc[0] = *(undefined ***)(iVar6 + 0x520);
            *(undefined2 *)((int)local_5fc[0] + ((uVar5 & 0xfffffffe) - 2)) = 0;
          }
        }
      }
      uVar5 = tag_lookup("ui\\multiplayer_game_text");
      if (uVar5 == 0xffffffff) {
        local_5fc[1] = (undefined **)&DAT_00660c34;
      }
      else {
        piVar2 = *(int **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        local_5fc[1] = &PTR_DAT_00671fac;
        if (0x42 < *piVar2) {
          iVar6 = piVar2[1];
          uVar5 = *(uint *)(iVar6 + 0x528);
          if (0 < (int)uVar5) {
            local_5fc[1] = *(undefined ***)(iVar6 + 0x534);
            *(undefined2 *)((int)local_5fc[1] + ((uVar5 & 0xfffffffe) - 2)) = 0;
          }
        }
      }
      if (iVar4 == 0) {
        local_630 = 1;
        local_62c = 0;
      }
      DAT_006e4748 = 6;
      _DAT_006e474a = local_5f0[0];
      _DAT_006e474e = local_5f0[1];
      _DAT_006e4752 = local_5f0[2];
      iVar4 = 0;
      do {
        iVar6 = (&local_630)[iVar4];
        (**(code **)(DAT_006f1d20 + 0x5c))(iVar6,local_3c0);
        string_format_wide_va_bounded(auStack_5c0,local_5fc[iVar6],local_3c0);
        uStack_3c2 = 0;
        FUN_0045d670(auStack_5c0,0);
        iVar4 = iVar4 + 1;
      } while (iVar4 < 2);
    }
    uVar5 = tag_lookup("ui\\multiplayer_game_text");
    if (uVar5 == 0xffffffff) {
      local_5f0[0] = (undefined **)&DAT_00660c34;
    }
    else {
      piVar2 = *(int **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      local_5f0[0] = &PTR_DAT_00671fac;
      if (0x43 < *piVar2) {
        iVar4 = piVar2[1];
        uVar5 = *(uint *)(iVar4 + 0x53c);
        if (0 < (int)uVar5) {
          local_5f0[0] = *(undefined ***)(iVar4 + 0x548);
          *(undefined2 *)((int)local_5f0[0] + ((uVar5 & 0xfffffffe) - 2)) = 0;
        }
      }
    }
    uVar5 = tag_lookup("ui\\multiplayer_game_text");
    if (uVar5 == 0xffffffff) {
      local_5f0[1] = (undefined **)&DAT_00660c34;
    }
    else {
      piVar2 = *(int **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      local_5f0[1] = &PTR_DAT_00671fac;
      if (0x44 < *piVar2) {
        iVar4 = piVar2[1];
        uVar5 = *(uint *)(iVar4 + 0x550);
        if (0 < (int)uVar5) {
          local_5f0[1] = *(undefined ***)(iVar4 + 0x55c);
          *(undefined2 *)((int)local_5f0[1] + ((uVar5 & 0xfffffffe) - 2)) = 0;
        }
      }
    }
    uVar5 = tag_lookup("ui\\multiplayer_game_text");
    if (uVar5 == 0xffffffff) {
      local_5f0[2] = (undefined **)&DAT_00660c34;
    }
    else {
      piVar2 = *(int **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      local_5f0[2] = &PTR_DAT_00671fac;
      if (0x45 < *piVar2) {
        iVar4 = piVar2[1];
        uVar5 = *(uint *)(iVar4 + 0x564);
        if (0 < (int)uVar5) {
          local_5f0[2] = *(undefined ***)(iVar4 + 0x570);
          *(undefined2 *)((int)local_5f0[2] + ((uVar5 & 0xfffffffe) - 2)) = 0;
        }
      }
    }
    uVar5 = tag_lookup("ui\\multiplayer_game_text");
    if (uVar5 == 0xffffffff) {
      ppuVar9 = (undefined **)&DAT_00660c34;
    }
    else {
      piVar2 = *(int **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      ppuVar9 = &PTR_DAT_00671fac;
      if (0x46 < *piVar2) {
        iVar4 = piVar2[1];
        uVar5 = *(uint *)(iVar4 + 0x578);
        if (0 < (int)uVar5) {
          ppuVar9 = *(undefined ***)(iVar4 + 0x584);
          *(undefined2 *)((int)ppuVar9 + ((uVar5 & 0xfffffffe) - 2)) = 0;
        }
      }
    }
    uVar5 = tag_lookup("ui\\multiplayer_game_text");
    if (uVar5 == 0xffffffff) {
      ppuVar10 = (undefined **)&DAT_00660c34;
    }
    else {
      piVar2 = *(int **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      ppuVar10 = &PTR_DAT_00671fac;
      if (0x47 < *piVar2) {
        iVar4 = piVar2[1];
        uVar5 = *(uint *)(iVar4 + 0x58c);
        if (0 < (int)uVar5) {
          ppuVar10 = *(undefined ***)(iVar4 + 0x598);
          *(undefined2 *)((int)ppuVar10 + ((uVar5 & 0xfffffffe) - 2)) = 0;
        }
      }
    }
    (**(code **)(DAT_006f1d20 + 0x58))(local_3c0);
    string_format_wide_va_bounded
              (auStack_5c0,L"\t%s\t%s\t%s\t%s\t%s\t%s",local_5f0[0],local_5f0[1],local_3c0,
               local_5f0[2],ppuVar9,ppuVar10);
    _DAT_006e474a = local_608;
    uStack_3c2 = 0;
    DAT_006e4748 = 6;
    _DAT_006e474e = local_604;
    _DAT_006e4752 = local_600;
    FUN_0045d670(auStack_5c0,0);
    local_630 = select_players_to_display(auStack_1c0);
    if (0 < local_630) {
      local_5fc[0] = apuStack_1a8;
      do {
        puVar3 = local_5fc[0][-6];
        iVar4 = ((uint)puVar3 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
        if (*(short *)(iVar4 + 2) == -1) {
          DAT_006e4738 = local_628;
          DAT_006e473c = local_624;
          DAT_006e4740 = local_620;
          DAT_006e4744 = local_61c;
        }
        else {
          DAT_006e4738 = local_5f0[8];
          DAT_006e473c = local_5f0[9];
          DAT_006e4740 = local_5f0[10];
          DAT_006e4744 = local_5f0[0xb];
        }
        _DAT_006e474a = local_608;
        DAT_006e4748 = 6;
        _DAT_006e474e = local_604;
        _DAT_006e4752 = local_600;
        uStack_5f4 = 0xf;
        if (((uint)*local_5fc[0] & 0x7f) < 0x10) {
          uStack_5f4 = (uint)*local_5fc[0] & 0x7f;
        }
        uVar5 = tag_lookup("ui\\multiplayer_game_text");
        if (uVar5 == 0xffffffff) {
          ppuVar9 = (undefined **)&DAT_00660c34;
        }
        else {
          sVar7 = (short)uStack_5f4 + 0x24;
          piVar2 = *(int **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          ppuVar9 = &PTR_DAT_00671fac;
          if ((-1 < sVar7) && ((int)sVar7 < *piVar2)) {
            puVar1 = (uint *)(piVar2[1] + sVar7 * 0x14);
            uVar5 = *puVar1;
            if (0 < (int)uVar5) {
              ppuVar9 = (undefined **)puVar1[3];
              *(undefined2 *)((int)ppuVar9 + ((uVar5 & 0xfffffffe) - 2)) = 0;
            }
          }
        }
        string_format_wide_va_bounded(auStack_5c0,L" \t%s",ppuVar9);
        uStack_3c2 = 0;
        FUN_0045d670(auStack_5c0,0);
        DAT_006e4738 = local_628;
        DAT_006e4744 = local_61c;
        DAT_006e473c = local_624;
        DAT_006e4740 = local_620;
        if (DAT_006f1cbc != '\0') {
          iVar6 = *(int *)(iVar4 + 0x20);
          local_5f0[1] = (undefined **)0x3f4ccccd;
          local_5f0[2] = (undefined **)0x3ecccccd;
          local_5f0[3] = (undefined **)0x3ecccccd;
          local_5f0[0] = (undefined **)0x3f800000;
          local_5f0[5] = (undefined **)0x3ecccccd;
          local_5f0[6] = (undefined **)0x3ecccccd;
          local_5f0[7] = (undefined **)0x3f4ccccd;
          local_5f0[4] = (undefined **)0x3f800000;
          if (iVar6 < 0) {
            iVar6 = 0;
          }
          else if (1 < iVar6) {
            iVar6 = 1;
          }
          DAT_006e4738 = local_5f0[iVar6 * 4];
          DAT_006e473c = local_5f0[iVar6 * 4 + 1];
          DAT_006e4740 = local_5f0[iVar6 * 4 + 2];
          DAT_006e4744 = local_5f0[iVar6 * 4 + 3];
        }
        string_format_wide_va_bounded(auStack_5c0,L" \t \t%s",iVar4 + 4);
        uStack_3c2 = 0;
        FUN_0045d670(auStack_5c0,0);
        DAT_006e4738 = local_628;
        DAT_006e4740 = local_620;
        DAT_006e473c = local_624;
        DAT_006e4744 = local_61c;
        iVar6 = FUN_0045d440(0);
        if (iVar6 == 0) {
          DAT_006e4738 = local_618;
          DAT_006e473c = local_614;
          DAT_006e4740 = local_610;
          DAT_006e4744 = local_60c;
        }
        (**(code **)(DAT_006f1d20 + 0x54))(puVar3,local_3c0);
        string_format_wide_va_bounded(auStack_5c0,L" \t \t \t%s",local_3c0);
        uStack_3c2 = 0;
        FUN_0045d670(auStack_5c0,0);
        DAT_006e4738 = local_628;
        DAT_006e473c = local_624;
        DAT_006e4740 = local_620;
        DAT_006e4744 = local_61c;
        iVar6 = FUN_0045d440(0);
        if (iVar6 == 0) {
          DAT_006e4738 = local_618;
          DAT_006e473c = local_614;
          DAT_006e4740 = local_610;
          DAT_006e4744 = local_60c;
        }
        string_format_wide_va_bounded(auStack_5c0,L" \t \t \t \t%d",(int)*(short *)(iVar4 + 0x9c));
        uStack_3c2 = 0;
        FUN_0045d670(auStack_5c0,0);
        DAT_006e4738 = local_628;
        DAT_006e4740 = local_620;
        DAT_006e473c = local_624;
        DAT_006e4744 = local_61c;
        iVar6 = FUN_0045d440(0);
        if (iVar6 == 0) {
          DAT_006e4738 = local_618;
          DAT_006e473c = local_614;
          DAT_006e4740 = local_610;
          DAT_006e4744 = local_60c;
        }
        string_format_wide_va_bounded
                  (auStack_5c0,L" \t \t \t \t \t%d",(int)*(short *)(iVar4 + 0xa4));
        uStack_3c2 = 0;
        FUN_0045d670(auStack_5c0,0);
        DAT_006e4738 = local_628;
        DAT_006e4744 = local_61c;
        DAT_006e473c = local_624;
        DAT_006e4740 = local_620;
        iVar6 = FUN_0045d440(0);
        if (iVar6 == 0) {
          DAT_006e4738 = local_618;
          DAT_006e473c = local_614;
          DAT_006e4740 = local_610;
          DAT_006e4744 = local_60c;
        }
        string_format_wide_va_bounded
                  (auStack_5c0,L" \t \t \t \t \t \t%d",(int)*(short *)(iVar4 + 0xae));
        uStack_3c2 = 0;
        FUN_0045d670(auStack_5c0,0);
        _DAT_006e4752 = local_600;
        _DAT_006e474e = local_604;
        local_5fc[0] = local_5fc[0] + 7;
        local_630 = local_630 + -1;
        _DAT_006e474a = local_608;
      } while (local_630 != 0);
    }
    local_5f0[8] = (undefined **)DAT_0087aa0c;
    local_62c._0_2_ = (short)DAT_007c314c;
    sVar7 = -(short)DAT_007c3140 + 0x19a;
    local_62c = CONCAT22((short)((uint)DAT_007c314c >> 0x10) - DAT_007c3140._2_2_,
                         (short)local_62c + -(short)DAT_007c3140);
    DAT_006e4738 = (undefined **)DAT_0087aa0c;
    DAT_006e4748 = 0;
    DAT_006e473c = local_624;
    DAT_006e4740 = local_620;
    DAT_006e4744 = local_61c;
    if (DAT_0071c2d4 == 0) {
      local_630 = CONCAT22(0x1a4,sVar7);
      uVar5 = tag_lookup("ui\\multiplayer_game_text");
      if ((uVar5 != 0xffffffff) &&
         (piVar2 = *(int **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 0x49 < *piVar2)) {
        iVar4 = piVar2[1];
        uVar5 = *(uint *)(iVar4 + 0x5b4);
        if (0 < (int)uVar5) {
          *(undefined2 *)(*(int *)(iVar4 + 0x5c0) + -2 + (uVar5 & 0xfffffffe)) = 0;
        }
      }
    }
    else {
      local_630 = CONCAT22(0x118,sVar7);
      uVar5 = tag_lookup("ui\\multiplayer_game_text");
      if ((uVar5 != 0xffffffff) &&
         (piVar2 = *(int **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 0x48 < *piVar2)) {
        iVar4 = piVar2[1];
        uVar5 = *(uint *)(iVar4 + 0x5a0);
        if (0 < (int)uVar5) {
          *(undefined2 *)(*(int *)(iVar4 + 0x5ac) + -2 + (uVar5 & 0xfffffffe)) = 0;
        }
      }
    }
    ui_widget_draw_formatted_prompt_string(&local_630,0);
  }
  return;
}
#endif
