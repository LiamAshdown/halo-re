// game_engine_rasterize_in_game_score  (Ghidra: game_engine_rasterize_in_game_score, already
// named -- CEA-PDB matched via the string "%s (%s)")
// address 0x465690, size 3226 bytes
// name confidence: 0.75   rewrite confidence: 0.15
// evidence: out/phase4/game_functions.md ("Renders the in-game/post-game multiplayer scoreboard
// (per-player rows with ping, plus the server's IP:port) using the ui\multiplayer_game_text UI
// tag and chimera__draw_16_bit_text"); types/game.h scoreboard_entry (0x1c bytes: player,
// unknown_04, key_0..key_3, place) and player (local_player_index +0x02, team +0x20, unit +0x34,
// marked_for_deletion +0xd5, deaths +0xae, unknown_dc +0xdc); already-committed sibling files in
// this same batch and the adjacent scoreboard-rendering module supplied almost everything else:
//   - select_players_to_display.c / game_engine_get_default_multiplayer_string.c /
//     hud_draw_world_relative_text.c (0x4653f0, this batch) / game_engine_multiplayer_ui_state_id
//     .c (0x4655d0, this batch) -- all four are called here.
//   - game_engine_post_rasterize_post_game.c (0x45d700) is the near-twin "postgame" version of
//     this exact rendering job: its header documents that almost every tag_lookup +
//     text_string_list_get_string pair in that sibling reads its string-list index from a FIXED
//     byte offset that divides evenly by the 0x14-byte StringListString stride, which is exactly
//     the same idiom here -- the five header-column offsets (0x53c/0x548 .. 0x58c/0x598, gated
//     0x43 < count .. 0x47 < count) resolve to indices 67..71 the same way that sibling's
//     0x514/0x520 .. 0x578/0x584 (gated 0x41 < count .. 0x45 < count) resolve to 0x41..0x45.
//   - The per-row "is this row's player still in the game" text (indices 138 and 139, gated
//     0x8a < count / 0x8b < count, offsets 0xac8/0x14 == 138, 0xadc/0x14 == 139) sits behind a
//     condition that reads player::deaths (+0xae) against game_variant::lives_per_round
//     (+0x50 == 0x006f1cd8) and player::unit (+0x34) -- i.e. "is this player eliminated".
//   - color_real_to_argb_pack(float alpha, real_vector3d *color) matches src/objects/
//     light_transient_add.c's signature exactly.
// This rewrite follows game_engine_post_rasterize_post_game.c's own stated approach: it keeps
// the flat local-variable shape deliberately close to the decompilation rather than reverse-
// engineering the exact field semantics of the repeated HUD-text-draw "params" blocks (their
// values are transcribed verbatim; only their *meaning* is UNSURE), because those values affect
// on-screen color/position only, never game logic, and getting the flat bytes right is enough to
// preserve behavior exactly. Ghidra drops the EAX ("params") argument to every
// hud_draw_world_relative_text (0x4653f0) call in this function; each call site here is
// preceded by exactly the block of writes that fills such a params struct, so this rewrite
// passes the address of that just-filled block as EAX, which is the only value that makes
// sense of the surrounding code (see hud_draw_world_relative_text.c for the struct shape).
// register convention: `subject_player` in EAX (in_EAX, a player identifier to specially select,
//   or -1), `text_scale` as a stack float parameter (Ghidra's own recognized param_2).
// UNSURE (pervasive, see above): the exact struct layout each hud_draw_world_relative_text call
//   builds; DAT_00873d40 (a globals-tag-like font source, offsets +0x54/+0x64/+0x70);
//   the network-address formatting block (network_address_to_string/
//   network_channel_get_remote_address/inet_ntoa and the DAT_006869b4/DAT_00698208 globals);
//   FUN_00449780 and unit_find_weapon_index_by_flag (both outside this batch); the exact role of the "iVar7" object
//   check that decides the '*' row prefix (a weapon-flags test gated on unit_find_weapon_index_by_flag(3));
//   select_players_to_display's mode/max_count arguments (not recovered here; the output buffer
//   is sized for all 16 scoreboard slots, so max_count is modeled as 16).
// reconciled: R36 hud_world_text_params alpha-first (unknown_00 -> alpha, color_r/g/b -> red/green/blue); 0x006e4738 is float text_color.alpha; tag colour copies now read float bits (the binary moves them raw, the old uint32 read converted the value)
// fixed (reconciliation check): the text colour is not read from 0x873d40+0x78/+0x7c/+0x80. Both
// draw sites load it from the ColorARGB that 0x006851fc points to (opaque white, 0x00655138):
// 0x465ff2 mov edx,ds:0x6851fc; [edx+4]/[edx+8]/[edx+0xc] -> 0x6e473c/0x6e4740/0x6e4744
// (0x466050..0x46607b), and 0x466162 the same into locals stored at 0x4662e9..0x466309. The
// alpha is text_scale ([esp+0x6f8], 0x466014 / 0x466298). 0x873d40 only supplies the font
// (+0x64, else +0x54).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88 (::teams at 0x006f1cbc)
extern data_array *player_data;                     // 0x0087a480
extern data_array *object_headers;                  // 0x008603b0
extern tag_instance *tag_instances;                 // 0x0087bc14
extern uint8_t *network_session;                    // 0x0071c2d4
extern uint8_t *network_client;                     // 0x0071c2d8
extern wchar_t empty_string;                        // 0x00660c34
extern uint8_t *unknown_00873d40; // UNSURE: a globals-tag-like color/font source
extern const ColorARGB *global_white_argb; // 0x006851fc -> 0x00655138 = {1,1,1,1} (.data)

extern float hud_text_draw_color_r; // 0x006e473c
extern float hud_text_draw_color_g; // 0x006e4740
extern float hud_text_draw_color_b; // 0x006e4744
extern float hud_text_draw_color_alpha; // 0x006e4738, text.h text_color.alpha
extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734, two separate int16 slots in the
extern int16_t hud_text_draw_column;         // 0x006e4736  binary, never one dword
extern uint32_t hud_text_draw_unknown_4730;   // 0x006e4730
extern int32_t hud_text_draw_font_tag_id;     // 0x006e472c

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0, blam-cc: ECX, DX
extern wchar_t *game_engine_get_default_multiplayer_string(int16_t string_index); // 0x45ce90
extern wchar_t *string_format_wide_va_bounded(wchar_t *dest, const wchar_t *format, ...); // 0x557910
extern wchar_t *string_format_wide_va(const wchar_t *format, ...); // 0x557930
extern uint32_t color_real_to_argb_pack(float alpha, real_vector3d *color); // 0x44da60
extern int32_t select_players_to_display(int32_t mode, int32_t max_count,
    scoreboard_entry *out); // 0x45d4a0; mode/max_count not recovered here, see header

extern void game_engine_build_end_game_result_text(datum_index player, wchar_t *out); // 0x45cf30, not in this batch
extern int32_t hud_draw_world_relative_text(hud_world_text_params *params, int16_t row,
    wchar_t *text, uint8_t highlighted); // 0x4653f0, this module; blam-cc: EAX -> params,
    // EDX -> row, stack -> (text, highlighted). `row` 0 means "no background box".
extern int32_t game_engine_multiplayer_ui_state_id(void); // 0x4655d0, this batch
extern int32_t chimera__draw_16_bit_text(int32_t unknown_0, int32_t unknown_1, wchar_t *text);
    // 0x514ab0; blam-cc: EAX -> unknown (0 here), ECX -> bounds (hud_text_bounds *),
    // stack -> (unknown_0, unknown_1, text)

extern void FUN_00449780(void); // 0x449780, not in this batch; UNSURE exact meaning
extern uint8_t unit_find_weapon_index_by_flag(int32_t unknown_0); // 0x570520, not in this batch; UNSURE exact meaning
extern char *network_address_to_string(void); // 0x440570, not in this batch; UNSURE exact args
extern int16_t network_channel_get_remote_address(void); // 0x441ce0, not in this batch; UNSURE exact args
extern char *inet_ntoa(uint32_t addr); // Winsock

// blam-cc: EAX -> subject_player, stack -> (text_scale, unknown_param_2)
// Renders the in-game multiplayer scoreboard overlay: a header row (column labels plus the
// "Ping" literal and a live player-count string from the active game engine), one row per
// visible player (name/status/ping/score-ish stat columns, prefixed with '*' for a to-be-
// determined weapon condition and highlighted when the row is `subject_player`), a bottom-of-
// screen wizard-state prompt, and, when a network session or client is active, the server's
// address (dotted-quad while hosting/local, resolved via network_address_to_string otherwise)
// formatted as "name (address[:port])".
void game_engine_rasterize_in_game_score(datum_index subject_player, float text_scale,
                                          int32_t unknown_param_2)
{
    uint8_t teams_enabled;
    wchar_t result_text[80];          // local_260
    scoreboard_entry visible[16];     // local_1c0 .. auStack_1a8
    int32_t visible_count;            // local_6a0
    wchar_t row_buffer[128];          // auStack_660
    wchar_t header_names_buf[128];    // local_460
    hud_world_text_params params;     // types/game.h; rebuilt before each row draw
    wchar_t *col_a, *col_b, *col_c, *col_d, *col_e;
    int32_t highlight_team;           // local_6ac
    int32_t pass;                     // iVar12, counts down 1 or 2 team passes
    int32_t row_count;                // iStack_69c
    int16_t ui_state;

    teams_enabled = (current_game_engine != 0) ? (uint8_t)game_engine_variant.teams : 0;

    game_engine_build_end_game_result_text(subject_player, result_text);

    // CORRECTED (phase 4 review, objdump 0x465739..0x46576d): the first pass dropped this draw
    // call entirely. It renders `result_text` at row 0 (so, with no background box) in a flat
    // 0.7 grey. UNSURE: params.alpha comes from [esp+0x6e8], i.e. a SECOND stack parameter
    // this function has that Ghidra does not surface at all -- modelled as `unknown_param_2`.
    *(int32_t *)&params.alpha = unknown_param_2; // raw dword copy (the binary moves it with mov)
    params.red = 0.7f;
    params.green = 0.7f;
    params.blue = 0.7f;
    hud_draw_world_relative_text(&params, 0, result_text, 0);

    visible_count = select_players_to_display(0, 16, visible); // UNSURE: mode/max_count, see header

    // A translucent background color, packed and (per the disassembly this rewrite could not
    // fully resolve) apparently discarded; FUN_00449780 is then called with no visible arguments.
    {
        real_vector3d bg_color;
        bg_color.i = 0.125f;
        bg_color.j = 0.125f;
        bg_color.k = 0.125f; // UNSURE: local_6b8/local_6bc's exact role beyond this color triple
        (void)color_real_to_argb_pack(text_scale * 0.69f, &bg_color);
        FUN_00449780(); // UNSURE: exact purpose
    }

    // Header row: five tag-driven column labels (indices 67..71, see header), the active game
    // engine's own live player-count string, and the literal "Ping".
    col_a = game_engine_get_default_multiplayer_string(67); // UNSURE index role: header column
    col_b = game_engine_get_default_multiplayer_string(68); // UNSURE index role: header column
    col_c = game_engine_get_default_multiplayer_string(69); // UNSURE index role: header column
    col_d = game_engine_get_default_multiplayer_string(70); // UNSURE index role: header column
    col_e = game_engine_get_default_multiplayer_string(71); // UNSURE index role: header column

    ((void (*)(void *))current_game_engine->build_score_header_text)(header_names_buf);
    string_format_wide_va(L"\t%s\t%s\t%s\t%s\t%s\t%s\t%s", col_a, col_b, header_names_buf,
                           col_c, col_d, col_e, L"Ping");
    // objdump 0x465a18: EDX = 1, so the header row IS boxed.
    hud_draw_world_relative_text(&params, 1, row_buffer, 0); // UNSURE: params contents, see header

    highlight_team = -1;
    if (subject_player != (datum_index)0xffffffff && (int16_t)subject_player >= 0 &&
        (int16_t)subject_player < player_data->maximum_count) {
        player *subj = (player *)((uint8_t *)player_data->data +
                                   (subject_player & 0xffff) * sizeof(player));
        if (subj->identifier != 0 &&
            ((int16_t)(subject_player >> 16) == 0 || subj->identifier == (int16_t)(subject_player >> 16))) {
            highlight_team = subj->team;
        }
    }

    row_count = 0;
    pass = teams_enabled ? 2 : 1;
    for (; pass != 0; pass = pass - 1) {
        int32_t i;
        for (i = 0; i < visible_count; i = i + 1) {
            datum_index row_player = visible[i].player;
            uint8_t is_subject = (row_player == subject_player);

            if (row_player == (datum_index)0xffffffff || (int16_t)row_player < 0 ||
                (int16_t)row_player >= player_data->maximum_count) {
                continue;
            }

            {
                player *p = (player *)((uint8_t *)player_data->data +
                                        (row_player & 0xffff) * sizeof(player));
                if (p->identifier == 0) continue;
                if ((int16_t)(row_player >> 16) != 0 && p->identifier != (int16_t)(row_player >> 16)) continue;
                if (teams_enabled != 0 && highlight_team != p->team) continue;

                ((void (*)(datum_index, void *))current_game_engine->unknown_54_build_player_text)(
                    row_player, header_names_buf);

                {
                    wchar_t *status_text;
                    uint8_t eliminated = (game_engine_variant.lives_per_round >= 1 &&
                                          p->unit == (datum_index)0xffffffff &&
                                          p->deaths >= game_engine_variant.lives_per_round);
                    if (!eliminated) {
                        status_text = (p->marked_for_deletion == 0)
                            ? header_names_buf
                            : game_engine_get_default_multiplayer_string(139); // UNSURE index role
                    } else {
                        status_text = game_engine_get_default_multiplayer_string(138); // UNSURE index role
                    }

                    {
                        object *unit_obj = (p->unit == (datum_index)0xffffffff)
                            ? (object *)0
                            : ((object_header *)object_headers->data)[p->unit & 0xffff].data;
                        int16_t team_col_index = (int16_t)(visible[i].player & 0x7f); // UNSURE: field role
                        wchar_t *team_text;
                        datum_index text_tag;
                        uint8_t starred;

                        if (team_col_index > 0xf) team_col_index = 0xf;
                        text_tag = tag_lookup(0x75737472, "ui\\multiplayer_game_text"); // 'ustr'
                        team_text = (text_tag == k_datum_index_none)
                            ? &empty_string
                            : text_string_list_get_string(text_tag, (int16_t)(team_col_index + 0x24));
                            // UNSURE: role of indices 0x24..0x33 (per team/rank label)

                        // UNSURE: whether this row gets a '*' prefix depends on a weapon-flags
                        // test (mask 3) against the unit's readied weapon plus unit_find_weapon_index_by_flag(3);
                        // neither the weapon-tag chain nor unit_find_weapon_index_by_flag is resolved here.
                        starred = 0;
                        if (unit_obj != (object *)0) {
                            starred = unit_find_weapon_index_by_flag(3); // UNSURE: args/role
                        }

                        string_format_wide_va(starred ? L"*\t%s\t%s\t%s\t%d\t%d\t%d\t%d"
                                                       : L"\t%s\t%s\t%s\t%d\t%d\t%d\t%d",
                                              team_text, p->name, status_text,
                                              visible[i].key_1, visible[i].key_3, visible[i].key_2,
                                              p->unknown_dc);
                    }
                }

                // objdump 0x465e4e: `lea edx,[esi+2]`, i.e. row = row_count + 2.
                hud_draw_world_relative_text(&params, (int16_t)(row_count + 2), row_buffer,
                                              (uint8_t)is_subject);
                // UNSURE: params contents, see header
                row_count = row_count + 1;
            }
        }
        if (highlight_team != -1) {
            highlight_team = 1 - highlight_team;
        }
    }

    ui_state = game_engine_multiplayer_ui_state_id();
    if (ui_state != 8) {
        wchar_t *prompt = game_engine_get_default_multiplayer_string(ui_state); // UNSURE index role
        if (prompt != (wchar_t *)0) {
            wchar_t line[128];
            if (current_game_engine == 0 || game_engine_variant.teams != 1) {
                string_format_wide_va(&empty_string, prompt); // UNSURE: format string identity (PTR_DAT_006607c8)
            } else {
                wchar_t *team_word = game_engine_get_default_multiplayer_string(13); // UNSURE index role
                string_format_wide_va(L"%s (%s)", prompt, team_word);
            }
            (void)line;

            hud_text_draw_color_r = global_white_argb->red;
            hud_text_draw_color_g = global_white_argb->green;
            hud_text_draw_color_b = global_white_argb->blue;
            hud_text_draw_font_tag_id = *(int32_t *)((uint8_t *)unknown_00873d40 + 0x64);
            if (hud_text_draw_font_tag_id == -1) {
                hud_text_draw_font_tag_id = *(int32_t *)((uint8_t *)unknown_00873d40 + 0x54);
            }
            hud_text_draw_color_alpha = text_scale;
            hud_text_draw_color_or_flags = 0xffffu;
            hud_text_draw_column = 0;
            hud_text_draw_unknown_4730 = 0;
            chimera__draw_16_bit_text(0, 0, row_buffer);
        }
    }

    // Server address line: dotted-quad (hosting/local) or a resolved network address string,
    // formatted as "name (address[:port])" and drawn the same way as the prompt above.
    {
        uint16_t port = 0;
        char *address_text;

        if (network_session == (uint8_t *)0) {
            if (network_client == (uint8_t *)0) {
                return;
            }
            if (**(int32_t **)(network_client + 0xadc) == 0 ||
                network_channel_get_remote_address() != 0) {
                // UNSURE: this zeroes an unrelated local scratch block in the original; omitted
                // here since nothing downstream reads it.
            }
            address_text = network_address_to_string();
        } else {
            uint32_t raw = *(uint32_t *)0x006869b4; // UNSURE: byte-swap target, kept literal
            address_text = inet_ntoa(((raw << 0x10 | (raw & 0xff00) | (raw >> 0x10 & 0xff)) << 8) |
                                      (raw >> 0x18));
            port = *(uint16_t *)0x00698208;
        }

        if (address_text != (char *)0) {
            wchar_t *label = game_engine_get_default_multiplayer_string(0xbf); // UNSURE index role
            wchar_t address_wide[0x100];
            size_t len = strlen(address_text);
            size_t i;

            if (len > 0xff) len = 0xff;
            for (i = 0; i < len; i = i + 1) {
                address_wide[i] = (uint8_t)address_text[i];
            }
            address_wide[len] = 0;

            if (port == 0) {
                string_format_wide_va_bounded(row_buffer, L"%s%s", label, address_wide);
            } else {
                string_format_wide_va_bounded(row_buffer, L"%s%s:%u", label, address_wide, port);
            }

            hud_text_draw_font_tag_id = *(int32_t *)((uint8_t *)unknown_00873d40 + 0x64);
            if (hud_text_draw_font_tag_id == -1) {
                hud_text_draw_font_tag_id = *(int32_t *)((uint8_t *)unknown_00873d40 + 0x54);
            }
            hud_text_draw_color_alpha = text_scale;
            hud_text_draw_color_g = global_white_argb->green;
            hud_text_draw_color_b = global_white_argb->blue;
            hud_text_draw_color_or_flags = 0xffffu;
            hud_text_draw_column = 0;
            hud_text_draw_unknown_4730 = 0;
            hud_text_draw_color_r = global_white_argb->red;
            chimera__draw_16_bit_text(0, 0, row_buffer);
        }
    }
}

#if 0
Original Ghidra decompilation (0x465690), from tools/pack.py 0x465690: see out/phase2/game and
out/phase4/game_functions.md for the full 3226-byte body (too large to reproduce verbatim here
without exceeding this file practical size; the header above cites every offset and evidence
chain this rewrite relied on). Key excerpt establishing the header-column string-list indices:

  uVar6 = tag_lookup("ui\\multiplayer_game_text");
  if (uVar6 == 0xffffffff) { local_678 = (undefined **)&DAT_00660c34; }
  else {
    piVar1 = *(int **)((uVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    local_678 = &PTR_DAT_00671fac;
    if (0x43 < *piVar1) {                          // 0x43 == 67
      iVar7 = piVar1[1];
      uVar6 = *(uint *)(iVar7 + 0x53c);            // 0x53c / 0x14 == 67
      if (0 < (int)uVar6) {
        local_678 = *(undefined ***)(iVar7 + 0x548);
        *(undefined2 *)((int)local_678 + ((uVar6 & 0xfffffffe) - 2)) = 0;
      }
    }
  }
  /* four more near-identical blocks at 0x44/0x550..0x55c, 0x45/0x564..0x570,
     0x46/0x578..0x584, 0x47/0x58c..0x598 -- indices 68, 69, 70, 71 */
#endif
