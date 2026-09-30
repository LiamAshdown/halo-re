// game_engine_post_rasterize_post_game  (Ghidra: game_engine_post_rasterize_post_game, already
// named)
// address 0x45d700, size 3024 bytes
// name confidence: 0.8   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Renders the postgame carnage report / scoreboard
// overlay by formatting per-player or per-team score columns"); types/game.h game_engine_definition
// (build_team_score_text +0x5c, get_team_score, build_score_header_text +0x58,
// unknown_54_build_player_text +0x54); scoreboard_entry; select_players_to_display.c,
// hud_draw_scoreboard_row_text.c, game_engine_get_scoreboard_place.c (this batch).
// UNSURE (pervasive): almost every `tag_lookup` + `text_string_list_get_string` pair in this
// function reads its string-table index from a FIXED byte offset into the resolved entries
// array rather than a visible register argument. Those offsets divide evenly by the 0x14-byte
// entry stride (confirmed against the matching `0x41 < count` .. `0x49 < count` bounds checks,
// e.g. 0x514 / 0x14 == 0x41), so each one is transcribed as
// `multiplayer_game_text_string(N)` for the derived index N -- high confidence for
// the index *values*, low confidence for what each one actually says (team name / column header
// / bottom prompt -- inferred only from where it is used).
// This rewrite keeps the original's flat local-variable shape deliberately close to the
// decompilation (rather than nesting the four repeated corner-color / tab-stop writes into a
// shared helper) to minimize the chance of silently changing which write happens on which of the
// many near-identical branches.

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
extern void *hud_globals_tag_data;                      // 0x0071941c, UNSURE owning module
extern uint32_t hud_text_draw_font_tag_id;                // 0x006e472c, UNSURE identity
extern uint32_t hud_text_draw_color_a;             // 0x006e4738
extern uint32_t hud_text_draw_color_r;             // 0x006e473c
extern uint32_t hud_text_draw_color_g;             // 0x006e4740
extern uint32_t hud_text_draw_color_b;             // 0x006e4744
extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734, two separate int16 slots in the
extern int16_t hud_text_draw_column;         // 0x006e4736  binary, never one dword
extern uint32_t hud_text_draw_unknown_4730;         // 0x006e4730
extern uint32_t hud_text_draw_background_mode;        // 0x006e4748
extern void *text_tab_stops;               // 0x006e474a
extern void *hud_text_draw_box_field_474e;               // 0x006e474e
extern void *hud_text_draw_tabstop_c;               // 0x006e4752
extern Globals *global_globals;                     // 0x00746fa0
extern tag_instance *tag_instances;                 // 0x0087bc14
extern game_variant game_engine_variant;             // 0x006f1c88 (::teams at +0x34, 0x006f1cbc)
extern data_array *player_data;                      // 0x0087a480
extern uint32_t render_viewport_top;             // 0x007c3140
extern uint32_t screen_safe_area_right;              // 0x007c3148, UNSURE: unused in this function
extern uint32_t screen_safe_area_bottom;             // 0x007c314c
extern float game_engine_post_game_fade;             // 0x0087aa0c
extern network_server_globals *network_server;
extern wchar_t empty_string;                          // 0x00660c34
// 0x00671fac is not an empty string and not a pointer: it holds the characters of
// L"<missing string>", the engine-wide placeholder a failed unicode_string_list lookup
// falls back to. Ghidra prints it as `&PTR_DAT_00671fac` only because its first four
// bytes ("<m") happen to look like a pointer value.
extern wchar_t missing_string_text[];          // 0x00671fac, L"<missing string>"

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, EDX count
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0, blam-cc: ECX tag_id, DX index
// tag_lookup("ui\\multiplayer_game_text") + text_string_list_get_string(tag, index), or the empty string when the tag is missing.
// (This is the inlined `lookup + get_string(N)` pair the disassembly shows at every use; NOT the function at 0x45ce90, which
// is game_engine_get_default_multiplayer_string(entry) == get_place_string.)
static wchar_t *multiplayer_game_text_string(int16_t index)
{
    datum_index tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text"); // 'ustr'

    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return text_string_list_get_string(tag_id, index);
}
extern int32_t game_engine_get_scoreboard_place(datum_index player, int32_t mode,
    uint8_t invert_low_stat); // 0x45d440, this module; blam-cc: EDI -> player, EAX -> mode,
    // stack -> invert_low_stat. CORRECTED (phase 4 review): Ghidra shows no arguments at all
    // at the four call sites below, but objdump 0x45def0 / 0x45df9f / 0x45e03b / 0x45e0d7 sets
    // EAX to 1, 2, 3 and 4 respectively (the score / kills / assists / deaths column) and
    // pushes 0 for invert_low_stat, with EDI = the player handle.
extern int32_t select_players_to_display(int32_t mode, int32_t max_count,
    scoreboard_entry *out); // 0x45d4a0, this batch; mode in EAX, max_count in EBX
extern void hud_draw_scoreboard_row_text(int16_t row, wchar_t *text, int16_t column); // 0x45d670, this batch
extern char game_engine_is_tracked_object_winner(void); // 0x463730, not in this batch; UNSURE exact meaning
extern void ui_draw_screen_quad(uint32_t unknown_0, int32_t unknown_1, int32_t unknown_2); // 0x498b20
extern void ui_widget_draw_formatted_prompt_string(void *prompt, int32_t unknown); // 0x49ade0

// Renders the postgame carnage-report overlay: an optional hosting/lobby banner widget, a
// team-score header row when playing with teams, the scoreboard column headers, one row per
// visible player (rank label, name, score text, kills/assists/deaths), and a bottom prompt
// string that differs depending on whether a network session is active.
void game_engine_post_rasterize_post_game(void)
{
    uint32_t color_tl, color_tr, color_bl, color_br;             // local_628/624/620/61c
    uint32_t alt_color_tl, alt_color_tr, alt_color_bl, alt_color_br; // local_618/614/610/60c
    uint32_t tabstop_a, tabstop_b, tabstop_c;                    // local_608/604/600
    uint32_t team_color[12];                                     // local_5f0[12]
    wchar_t *team_name[2];                                       // local_5fc[2]
    uint32_t rank_index;                                         // uStack_5f4
    wchar_t line[256];                                           // auStack_5c0
    wchar_t score_text[256];                                     // local_3c0
    scoreboard_entry visible[16];                                // auStack_1c0 (16 entries)
    int32_t banner_a, banner_b;                                  // local_630/62c (also reused later)
    datum_index tag_id;
    wchar_t *col_a, *col_b, *col_c, *col_d, *col_e;
    int32_t visible_count, i;

    if (current_game_engine == 0) {
        return;
    }

    hud_text_draw_font_tag_id = *(uint32_t *)((uint8_t *)hud_globals_tag_data + 0x54);
    color_tl = 0x3f800000;
    color_tr = 0x3eeaeaeb;
    hud_text_draw_color_a = 0x3f800000;
    color_bl = 0x3f3ababb;
    color_br = 0x3f800000;
    hud_text_draw_color_r = 0x3eeaeaeb;
    hud_text_draw_color_g = 0x3f3ababb;
    hud_text_draw_color_b = 0x3f800000;
    hud_text_draw_color_or_flags = 0xffffu;
    hud_text_draw_column = 0;
    hud_text_draw_unknown_4730 = 0;
    tabstop_a = 0x7d0032;
    tabstop_b = 0x15e00fa;
    tabstop_c = 0x1f4019a;
    team_color[9] = 0x3f800000;
    team_color[10] = 0x3f800000;
    team_color[11] = 0;
    team_color[8] = 0x3f800000;
    alt_color_tl = 0x3f7ae148;
    alt_color_tr = 0x3f75c28f;
    alt_color_bl = 0x3f75c28f;
    alt_color_br = 0x3f800000;

    {
        // RESOLVED (phase 4 review): globals+0x140/+0x144 is Globals::interface_bitmaps and
        // the +0x6c inside its first element is GlobalsInterfaceBitmaps::hud_globals.tag_id
        // (element 6 of sixteen 0x10-byte TagDependency slots), so `nested` is the hud_globals
        // tag's data. Its own +0x60 is still an unnamed reflexive count.
        GlobalsInterfaceBitmaps *interface_bitmaps;
        uint32_t nested;

        interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
            ? (GlobalsInterfaceBitmaps *)0
            : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
        nested = (uint32_t)tag_instances[interface_bitmaps->hud_globals.tag_id.index].data;

        banner_a = 0;
        banner_b = 0x28001e0;
        if (nested != 0 && 0 < *(int32_t *)((uint8_t *)nested + 0x60) &&
            *(int32_t *)((uint8_t *)nested + 100) != 0) {
            uint32_t arg = 0;
            if (nested != 0 && 0 < *(int32_t *)((uint8_t *)nested + 0x60)) {
                arg = *(uint32_t *)((uint8_t *)nested + 100);
            }
            ui_draw_screen_quad(arg, 0, -1);
        }
    }

    if (game_engine_variant.teams != 0) {
        int32_t winner;

        banner_a = 0;
        team_color[0] = 0xc80032;
        team_color[1] = 0x15e012c;
        team_color[2] = 0x1f4019a;
        banner_b = 1;
        winner = game_engine_is_tracked_object_winner();

        tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text"); // 'ustr'
        team_name[0] = (tag_id == k_datum_index_none) ? &empty_string
            : multiplayer_game_text_string(0x41); // UNSURE index role: team0 name

        tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text");
        team_name[1] = (tag_id == k_datum_index_none) ? &empty_string
            : multiplayer_game_text_string(0x42); // UNSURE index role: team1 name

        if (winner == 0) {
            banner_a = 1;
            banner_b = 0;
        }

        hud_text_draw_background_mode = 6;
        text_tab_stops = &team_color[0];
        hud_text_draw_box_field_474e = &team_color[1];
        hud_text_draw_tabstop_c = &team_color[2];

        for (i = 0; i < 2; i++) {
            int32_t team = i == 0 ? banner_a : banner_b;
            ((void (*)(int32_t, wchar_t *))current_game_engine->build_team_score_text)(team, score_text);
            string_format_wide_va_bounded(0x100, line, team_name[team], score_text);
            text_tab_stops = &tabstop_a;
            hud_text_draw_background_mode = 6;
            hud_text_draw_box_field_474e = &tabstop_b;
            hud_text_draw_tabstop_c = &tabstop_c;
            hud_draw_scoreboard_row_text(0, line, 0);
        }
    }

    col_a = multiplayer_game_text_string(0x43); // UNSURE index role: header column
    col_b = multiplayer_game_text_string(0x44); // UNSURE index role: header column
    col_c = multiplayer_game_text_string(0x45); // UNSURE index role: header column
    col_d = multiplayer_game_text_string(0x46); // UNSURE index role: header column
    col_e = multiplayer_game_text_string(0x47); // UNSURE index role: header column

    ((void (*)(wchar_t *))current_game_engine->build_score_header_text)(score_text);
    string_format_wide_va_bounded(0x100, line, L"\t%s\t%s\t%s\t%s\t%s\t%s",
        col_a, col_b, score_text, col_c, col_d, col_e);
    text_tab_stops = &tabstop_a;
    hud_text_draw_unknown_4730 = hud_text_draw_unknown_4730; // no-op, keeps analyzer quiet
    hud_text_draw_background_mode = 6;
    hud_text_draw_box_field_474e = &tabstop_b;
    hud_text_draw_tabstop_c = &tabstop_c;
    hud_draw_scoreboard_row_text(0, line, 0);

    visible_count = select_players_to_display(0, 0xc, visible); // xor eax,eax / mov ebx,0xc at
                                                             // call site) not recovered

    for (i = 0; i < visible_count; i++) {
        datum_index player_handle = visible[i].player;
        player *p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
        wchar_t *rank_text;
        int32_t place;

        if (p->local_player_index == -1) {
            hud_text_draw_color_a = color_tl;
            hud_text_draw_color_r = color_tr;
            hud_text_draw_color_g = color_bl;
            hud_text_draw_color_b = color_br;
        } else {
            hud_text_draw_color_a = team_color[8];
            hud_text_draw_color_r = team_color[9];
            hud_text_draw_color_g = team_color[10];
            hud_text_draw_color_b = team_color[11];
        }
        text_tab_stops = &tabstop_a;
        hud_text_draw_background_mode = 6;
        hud_text_draw_box_field_474e = &tabstop_b;
        hud_text_draw_tabstop_c = &tabstop_c;

        rank_index = visible[i].place & 0x7f;
        if (0x10 <= rank_index) {
            rank_index = 0xf;
        }

        tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text");
        rank_text = (tag_id == k_datum_index_none) ? &empty_string
            : multiplayer_game_text_string((int16_t)rank_index + 0x24);
            // UNSURE: role of indices 0x24..0x33 (per-rank label, e.g. "1st")

        string_format_wide_va_bounded(0x100, line, L" \t%s", rank_text);
        hud_draw_scoreboard_row_text(0, line, 0);

        hud_text_draw_color_a = color_tl;
        hud_text_draw_color_b = color_br;
        hud_text_draw_color_r = color_tr;
        hud_text_draw_color_g = color_bl;
        if (game_engine_variant.teams != 0) {
            int32_t team = p->team;
            team_color[1] = 0x3f4ccccd;
            team_color[2] = 0x3ecccccd;
            team_color[3] = 0x3ecccccd;
            team_color[0] = 0x3f800000;
            team_color[5] = 0x3ecccccd;
            team_color[6] = 0x3ecccccd;
            team_color[7] = 0x3f4ccccd;
            team_color[4] = 0x3f800000;
            if (team < 0) {
                team = 0;
            } else if (1 < team) {
                team = 1;
            }
            hud_text_draw_color_a = team_color[team * 4];
            hud_text_draw_color_r = team_color[team * 4 + 1];
            hud_text_draw_color_g = team_color[team * 4 + 2];
            hud_text_draw_color_b = team_color[team * 4 + 3];
        }
        string_format_wide_va_bounded(0x100, line, L" \t \t%s", p->name);
        hud_draw_scoreboard_row_text(0, line, 0);

        hud_text_draw_color_a = color_tl;
        hud_text_draw_color_g = color_bl;
        hud_text_draw_color_r = color_tr;
        hud_text_draw_color_b = color_br;
        place = game_engine_get_scoreboard_place(player_handle, 1, 0);
        if (place == 0) {
            hud_text_draw_color_a = alt_color_tl;
            hud_text_draw_color_r = alt_color_tr;
            hud_text_draw_color_g = alt_color_bl;
            hud_text_draw_color_b = alt_color_br;
        }
        ((void (*)(datum_index, wchar_t *))current_game_engine->build_player_text)(
            player_handle, score_text);
        string_format_wide_va_bounded(0x100, line, L" \t \t \t%s", score_text);
        hud_draw_scoreboard_row_text(0, line, 0);

        hud_text_draw_color_a = color_tl;
        hud_text_draw_color_g = color_bl;
        hud_text_draw_color_r = color_tr;
        hud_text_draw_color_b = color_br;
        place = game_engine_get_scoreboard_place(player_handle, 2, 0);
        if (place == 0) {
            hud_text_draw_color_a = alt_color_tl;
            hud_text_draw_color_r = alt_color_tr;
            hud_text_draw_color_g = alt_color_bl;
            hud_text_draw_color_b = alt_color_br;
        }
        string_format_wide_va_bounded(0x100, line, L" \t \t \t \t%d", (int32_t)p->kills);
        hud_draw_scoreboard_row_text(0, line, 0);

        hud_text_draw_color_a = color_tl;
        hud_text_draw_color_r = color_tr;
        hud_text_draw_color_g = color_bl;
        hud_text_draw_color_b = color_br;
        place = game_engine_get_scoreboard_place(player_handle, 3, 0);
        if (place == 0) {
            hud_text_draw_color_a = alt_color_tl;
            hud_text_draw_color_r = alt_color_tr;
            hud_text_draw_color_g = alt_color_bl;
            hud_text_draw_color_b = alt_color_br;
        }
        string_format_wide_va_bounded(0x100, line, L" \t \t \t \t \t%d", (int32_t)p->assists);
        hud_draw_scoreboard_row_text(0, line, 0);

        hud_text_draw_color_a = color_tl;
        hud_text_draw_color_b = color_br;
        hud_text_draw_color_r = color_tr;
        hud_text_draw_color_g = color_bl;
        place = game_engine_get_scoreboard_place(player_handle, 4, 0);
        if (place == 0) {
            hud_text_draw_color_a = alt_color_tl;
            hud_text_draw_color_r = alt_color_tr;
            hud_text_draw_color_g = alt_color_bl;
            hud_text_draw_color_b = alt_color_br;
        }
        string_format_wide_va_bounded(0x100, line, L" \t \t \t \t \t \t%d", (int32_t)p->deaths);
        hud_draw_scoreboard_row_text(0, line, 0);

        hud_text_draw_box_field_474e = &tabstop_b;
        hud_text_draw_tabstop_c = &tabstop_c;
        text_tab_stops = &tabstop_a;
    }

    team_color[8] = *(uint32_t *)&game_engine_post_game_fade;
    hud_text_draw_color_a = *(uint32_t *)&game_engine_post_game_fade;
    hud_text_draw_background_mode = 0;
    hud_text_draw_color_r = color_tr;
    hud_text_draw_color_g = color_bl;
    hud_text_draw_color_b = color_br;

    {
        // Bottom prompt rect: packed from the screen-safe-area globals; not fully resolved, see
        // the file header note. `banner_a` is reused here for the packed rect/flags argument.
        int16_t origin_low = (int16_t)render_viewport_top;
        int16_t bottom_low = (int16_t)screen_safe_area_bottom;
        int16_t rel_y = (int16_t)(-origin_low + 0x19a);
        (void)bottom_low;

        if (network_server == 0) {
            banner_a = ((int32_t)0x1a4 << 16) | (uint16_t)rel_y;
            tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text");
            if (tag_id != k_datum_index_none) {
                (void)multiplayer_game_text_string(0x49); // UNSURE index role
            }
        } else {
            banner_a = ((int32_t)0x118 << 16) | (uint16_t)rel_y;
            tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text");
            if (tag_id != k_datum_index_none) {
                (void)multiplayer_game_text_string(0x48); // UNSURE index role
            }
        }
    }

    ui_widget_draw_formatted_prompt_string(&banner_a, 0);
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
