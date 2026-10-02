// game_engine_rasterize_in_game_score  (Ghidra: game_engine_rasterize_in_game_score, already
// named -- CEA-PDB matched via the string "%s (%s)")
// address 0x465690, size 3226 bytes
// VERIFIED against disassembly 0x465690..0x46632a (2026-09-30)
// name confidence: 0.75   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Renders the in-game/post-game multiplayer scoreboard
// (per-player rows with ping, plus the server's IP:port) using the ui\multiplayer_game_text UI
// tag and chimera__draw_16_bit_text"); types/game.h scoreboard_entry (0x1c bytes: player,
// single_sort_key, key_0..key_3, place) and player (local_player_index +0x02, team +0x20, unit +0x34,
// marked_for_deletion +0xd5, deaths +0xae, ping +0xdc); game_engine_post_rasterize_post_game.c is the near-twin.
// register convention: both inputs are stack arguments (subject_player, opacity).
// REWRITTEN 2026-09-30 from the disassembly (the earlier draft was "rewrite confidence 0.15", built around a single reused
// params block and guessed string indices). What was wrong / is now pinned:
//  - Order: the translucent background rectangle is drawn FIRST (color_real_to_argb_pack(opacity * 0.69, {0.125,0.125,0.125}) ->
//    ui_draw_filled_rectangle(EAX packed color, ECX rect {top 0x3c, left 0xa, bottom 0x186, right 0x276})), then the result text.
//  - Five hud_world_text_params blocks (alpha is always the opacity): result text (0.7 grey), the header row (0.5 grey), the
//    default row colour (the messaging globals' color at +0x74/+0x78/+0x7c), and one per team (team 0 = 0.6/0.3/0.3 red,
//    team 1 = 0.3/0.3/0.6 blue, chosen by the player's team clamped to 0..1 when teams are on). The draft drew every row with the grey block.
//  - Every string is `tag_lookup + text_string_list_get_string(N)` (0x43..0x47 header columns, 0x8a / 0x8b eliminated / marked,
//    place = 0x24 + min(place & 0x7f, 15) taken from entry.PLACE (the draft used entry.player), 0xc / 0xd team/non-team word, 0xbe server label
//    (not 0xbf)); the prompt line is always formatted "%s (%s)" (prompt, word 0xc when teams == 1 else 0xd).
//  - string_format_wide_va takes its destination in EDX; the draft never passed one.
//  - The '*' row prefix is `unit_find_weapon_index_by_flag(unit, 3) != 0` for a validated live unit (type mask 3 with data); it is not a
//    weapon-tag test.
//  - Text draw state: the font is the messaging globals' +0x64 only when more than one local player exists (else +0x54), the column word is 1
//    (the draft stored 0), and the client address line hands network_channel_get_remote_address (ESI address, EDI the channel's
//    endpoint) / network_address_to_string (EAX address) a real s_network_address.
//  - The header row text draws with the 0.5 grey block; rows are drawn at row_count + 2; the subject is `highlighted`.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include <wchar.h>
#include <string.h>
#include "units.h"
#include "networking.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88 (::teams at 0x006f1cbc)
extern data_array *player_data;                     // 0x0087a480
extern data_array *object_data;                  // 0x008603b0
extern tag_instance *tag_instances;                 // 0x0087bc14
extern network_server_globals *network_server;      // 0x0071c2d4
extern network_client_globals *network_client;      // 0x0071c2d8
extern player_globals *local_player_globals;        // 0x0087a478
extern wchar_t empty_string;                        // 0x00660c34
extern uint8_t *hud_messaging_parameters;           // 0x00873d40, the HUDGlobals messaging block (raw offsets used below)
extern const ColorARGB *global_white_argb;          // 0x006851fc -> 0x00655138 = {1,1,1,1} (.data)
extern uint32_t scoreboard_server_address_raw;      // 0x006869b4 (packed host address)
extern uint32_t scoreboard_server_port;             // 0x00698208 (only the low word is tested / printed)

extern float hud_text_draw_color_r; // 0x006e473c
extern float hud_text_draw_color_g; // 0x006e4740
extern float hud_text_draw_color_b; // 0x006e4744
extern float hud_text_draw_color_a; // 0x006e4738, text.h text_color.alpha
extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734, two separate int16 slots in the
extern int16_t hud_text_draw_column;         // 0x006e4736  binary, never one dword
extern uint32_t hud_text_draw_unknown_4730;   // 0x006e4730
extern int32_t hud_text_draw_font_tag_id;     // 0x006e472c

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
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
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, EDX count
extern uint16_t *string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest
extern uint32_t color_real_to_argb_pack(float alpha, float *rgb); // 0x44da60, cdecl
extern void ui_draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect); // 0x449780, blam-cc: EAX packed_color, ECX rect
extern int32_t select_players_to_display(int32_t mode, int32_t max_count,
    scoreboard_entry *out); // 0x45d4a0, blam-cc: EAX mode, EBX max_count, stack out

extern void game_engine_build_end_game_result_text(datum_index player, wchar_t *out); // 0x45cf30
extern wchar_t *game_engine_get_default_multiplayer_string(const scoreboard_entry *entry); // 0x45ce90 (really get_place_string), EAX entry
extern int32_t hud_draw_world_relative_text(hud_world_text_params *params, int16_t row,
    wchar_t *text, uint8_t highlighted); // 0x4653f0, this module; blam-cc: EAX -> params,
    // EDX -> row, stack -> (text, highlighted). `row` 0 means "no background box".
extern int32_t game_engine_multiplayer_ui_state_id(void); // 0x4655d0, this batch
extern void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override,
    uint32_t position_or_color1, uint32_t position_or_color2, const int16_t *text); // 0x514ab0, EAX clip, ECX dest rect, stack (0, 0, text)
    // 0x514ab0; blam-cc: EAX -> unknown (0 here), ECX -> bounds (hud_text_bounds *),
    // stack -> (unknown_0, unknown_1, text)

extern uint16_t unit_find_weapon_index_by_flag(uint32_t unit_index, uint8_t flag_bit); // 0x570520, blam-cc: EAX unit_index, stack flag_bit
extern char *network_address_to_string(s_network_address *addr); // 0x440570, blam-cc: EAX addr
extern int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue); // 0x441ce0, ESI address, EDI queue

// The font the scoreboard text uses: the messaging globals' +0x64 font when more than one local player exists (and it is set),
// otherwise +0x54 (0x466043..0x466050, 0x4662c1..0x4662ce).
static int32_t scoreboard_text_font(void)
{
    int32_t font = *(int32_t *)((uint8_t *)hud_messaging_parameters + 0x54);

    if (local_player_globals->local_player_count > 1) {
        int32_t preferred = *(int32_t *)((uint8_t *)hud_messaging_parameters + 0x64);

        if (preferred != -1) {
            font = preferred;
        }
    }
    return font;
}

// Draws one line of already-formatted text in white through the shared HUD text state, alpha = opacity.
static void scoreboard_draw_white_line(Rectangle2D *rect, wchar_t *text, float opacity)
{
    hud_text_draw_font_tag_id = scoreboard_text_font();
    hud_text_draw_color_a = opacity;
    hud_text_draw_color_r = global_white_argb->red;
    hud_text_draw_color_g = global_white_argb->green;
    hud_text_draw_color_b = global_white_argb->blue;
    hud_text_draw_color_or_flags = 0xffffu;
    hud_text_draw_column = 1;
    hud_text_draw_unknown_4730 = 0;
    chimera__draw_16_bit_text(0, (int32_t *)rect, 0, 0, (const int16_t *)text);
}

// blam-cc: stack -> subject_player, opacity
// Renders the in-game multiplayer scoreboard overlay: a background panel, the result line, a header row (column labels plus
// "Ping" and the engine's live header text), one row per visible player (place / name / status / four stats / ping, prefixed with '*'
// when the player's unit has the flagged weapon, coloured by team, highlighted when the row is `subject_player`), a bottom-of-screen
// prompt, and, when a network session or client is active, the server's address line.
void game_engine_rasterize_in_game_score(datum_index subject_player, float opacity)
{
    uint8_t teams_enabled;
    wchar_t result_text[0x50];
    scoreboard_entry visible[16];
    int32_t visible_count;
    wchar_t row_buffer[0x200];
    wchar_t header_names_buf[0x100];
    hud_world_text_params params_result;    // 0x465749..0x46576d: (opacity, 0.7, 0.7, 0.7)
    hud_world_text_params params_default;   // S+0x1c: (opacity, messaging color +0x74/+0x78/+0x7c)
    hud_world_text_params params_header;    // S+0x34: (opacity, 0.5, 0.5, 0.5)
    hud_world_text_params team_params[2];   // S+0x58 / S+0x68: red-ish / blue-ish
    real_vector3d bg_color;
    Rectangle2D bg_rect;
    wchar_t *col_a, *col_b, *col_c, *col_d, *col_e;
    int32_t highlight_team;
    int32_t pass;
    int32_t row_count;
    int32_t ui_state;
    wchar_t *prompt;

    teams_enabled = (current_game_engine != 0) ? (uint8_t)game_engine_variant.teams : 0;

    game_engine_build_end_game_result_text(subject_player, result_text);
    visible_count = select_players_to_display(0, 16, visible);

    // 0x4656ee..0x465734: translucent panel behind everything
    bg_color.i = 0.125f;
    bg_color.j = 0.125f;
    bg_color.k = 0.125f;
    bg_rect.top = 0x3c;
    bg_rect.left = 0xa;
    bg_rect.bottom = 0x186;
    bg_rect.right = 0x276;
    ui_draw_filled_rectangle(color_real_to_argb_pack(opacity * 0.69f, &bg_color.i), &bg_rect);

    // result text, row 0 (no box), flat 0.7 grey
    params_result.alpha = opacity;
    params_result.red = 0.7f;
    params_result.green = 0.7f;
    params_result.blue = 0.7f;
    hud_draw_world_relative_text(&params_result, 0, result_text, 0);

    params_default.alpha = opacity;
    params_default.red = *(float *)((uint8_t *)hud_messaging_parameters + 0x74);
    params_default.green = *(float *)((uint8_t *)hud_messaging_parameters + 0x78);
    params_default.blue = *(float *)((uint8_t *)hud_messaging_parameters + 0x7c);
    team_params[0].alpha = opacity;
    team_params[0].red = 0.6f;
    team_params[0].green = 0.3f;
    team_params[0].blue = 0.3f;
    team_params[1].alpha = opacity;
    team_params[1].red = 0.3f;
    team_params[1].green = 0.3f;
    team_params[1].blue = 0.6f;
    params_header.alpha = opacity;
    params_header.red = 0.5f;
    params_header.green = 0.5f;
    params_header.blue = 0.5f;

    // Header row: five tag-driven column labels (0x43..0x47), the engine's own header text, and the literal "Ping".
    col_a = multiplayer_game_text_string(0x43);
    col_b = multiplayer_game_text_string(0x44);
    col_c = multiplayer_game_text_string(0x45);
    col_d = multiplayer_game_text_string(0x46);
    col_e = multiplayer_game_text_string(0x47);
    ((void (*)(wchar_t *))current_game_engine->build_score_header_text)(header_names_buf);
    string_format_wide_va((uint16_t *)row_buffer, (const uint16_t *)(L"\t%s\t%s\t%s\t%s\t%s\t%s\t%s"), col_a, col_b, header_names_buf,
                          col_c, col_d, col_e, L"Ping");
    hud_draw_world_relative_text(&params_header, 1, row_buffer, 0);

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
    for (pass = teams_enabled ? 2 : 1; pass != 0; pass = pass - 1) {
        int32_t i;

        for (i = 0; i < visible_count; i = i + 1) {
            datum_index row_player = visible[i].player;
            uint8_t is_subject = (uint8_t)(subject_player == row_player);
            player *p;

            if (row_player == (datum_index)0xffffffff || (int16_t)row_player < 0 ||
                (int16_t)row_player >= player_data->maximum_count) {
                continue;
            }
            p = (player *)((uint8_t *)player_data->data + (row_player & 0xffff) * sizeof(player));
            if (p->identifier == 0) {
                continue;
            }
            if ((int16_t)(row_player >> 16) != 0 && p->identifier != (int16_t)(row_player >> 16)) {
                continue;
            }
            if (teams_enabled != 0 && highlight_team != p->team) {
                continue;
            }

            {
                wchar_t *status_text;
                wchar_t *place_text;
                uint8_t starred = 0;
                hud_world_text_params *row_params;

                ((void (*)(datum_index, wchar_t *))current_game_engine->build_player_text)(row_player, header_names_buf);

                if (game_engine_variant.lives_per_round >= 1 && p->unit == (datum_index)0xffffffff &&
                    (int32_t)(int16_t)p->deaths >= game_engine_variant.lives_per_round) {
                    status_text = multiplayer_game_text_string(0x8a);        // eliminated
                } else if (p->marked_for_deletion != 0) {
                    status_text = multiplayer_game_text_string(0x8b);        // marked for deletion
                } else {
                    status_text = header_names_buf;
                }

                // 0x465c1f..0x465c9e: '*' when the (validated) live unit has the flagged weapon
                if (p->unit != (datum_index)0xffffffff) {
                    int16_t unit_index = (int16_t)p->unit;
                    int16_t unit_salt = (int16_t)((uint32_t)p->unit >> 16);

                    if (unit_index >= 0 && unit_index < object_data->maximum_count) {
                        object_header *unit_header = (object_header *)((uint8_t *)object_data->data +
                                                                        (int32_t)object_data->size * unit_index);

                        if (unit_header->identifier != 0 && (unit_salt == 0 || unit_header->identifier == unit_salt) &&
                            (((1u << (unit_header->type & 0x1f)) & 3) != 0) && unit_header->data != 0) {
                            starred = (uint8_t)unit_find_weapon_index_by_flag(p->unit, 3);
                        }
                    }
                }

                place_text = game_engine_get_default_multiplayer_string(&visible[i]);
                string_format_wide_va((uint16_t *)row_buffer, (const uint16_t *)(starred ? L"*\t%s\t%s\t%s\t%d\t%d\t%d\t%d"
                                                          : L"\t%s\t%s\t%s\t%d\t%d\t%d\t%d"),
                                      place_text, p->name, status_text,
                                      visible[i].key_1, visible[i].key_3, visible[i].key_2,
                                      p->ping);

                if (teams_enabled != 0) {
                    int32_t team = p->team;

                    if (team < 0) {
                        team = 0;
                    } else if (team > 1) {
                        team = 1;
                    }
                    row_params = &team_params[team];
                } else {
                    row_params = &params_default;
                }
                hud_draw_world_relative_text(row_params, (int16_t)(row_count + 2), row_buffer, is_subject);
                row_count = row_count + 1;
            }
        }
        if (highlight_team != -1) {
            highlight_team = 1 - highlight_team;
        }
    }

    ui_state = game_engine_multiplayer_ui_state_id();
    if ((int16_t)ui_state != 8) {
        wchar_t *word;
        Rectangle2D prompt_rect;

        prompt = multiplayer_game_text_string((int16_t)ui_state);
        if (prompt != (wchar_t *)0) {
            if (current_game_engine != 0 && game_engine_variant.teams == 1) {
                word = multiplayer_game_text_string(0xc);
            } else {
                word = multiplayer_game_text_string(0xd);
            }
            string_format_wide_va((uint16_t *)row_buffer, (const uint16_t *)(L"%s (%s)"), prompt, word);

            prompt_rect.top = 0x1b8;
            prompt_rect.left = 0xa;
            prompt_rect.bottom = 0x1cc;
            prompt_rect.right = 0x27b;
            scoreboard_draw_white_line(&prompt_rect, row_buffer, opacity);
        }
    }

    // Server address line: dotted-quad (hosting) or the connected channel's remote address, drawn below the prompt.
    {
        uint32_t port = 0;
        char *address_text;
        s_network_address address;

        if (network_server != 0) {
            struct in_addr in;
            uint32_t raw = scoreboard_server_address_raw;

            in.s_addr = ((raw << 0x10 | (raw & 0xff00) | (raw >> 0x10 & 0xff)) << 8) | (raw >> 0x18);
            address_text = inet_ntoa(in);
            port = scoreboard_server_port;
        } else {
            network_receive_queue *queue;

            if (network_client == 0) {
                return;
            }
            queue = network_client->channel->endpoint;
            if (queue != 0) {
                if (network_channel_get_remote_address(&address, queue) != 0) {
                    memset(&address, 0, 0x18);
                    address.size = 4;
                }
            } else {
                memset(&address, 0, 0x18);
                address.size = 4;
            }
            address_text = network_address_to_string(&address);
        }

        if (address_text != (char *)0) {
            wchar_t *label = multiplayer_game_text_string(0xbe);
            wchar_t address_wide[0x100];
            Rectangle2D address_rect;
            int32_t len = (int32_t)strlen(address_text);
            int32_t n;

            if (len * 2 + 2 > 0x200) {
                len = 0xff;
            }
            for (n = 0; n < len; n = n + 1) {
                address_wide[n] = (uint8_t)address_text[n];
            }
            address_wide[len] = 0;

            if ((uint16_t)port != 0) {
                string_format_wide_va_bounded(0x200, (uint16_t *)row_buffer, (const uint16_t *)(L"%s%s:%u"), label, address_wide, (uint32_t)(uint16_t)port);
            } else {
                string_format_wide_va_bounded(0x200, (uint16_t *)row_buffer, (const uint16_t *)(L"%s%s"), label, address_wide);
            }

            address_rect.top = 0x1cc;
            address_rect.left = 0xa;
            address_rect.bottom = 0x1e0;
            address_rect.right = 0x27b;
            scoreboard_draw_white_line(&address_rect, row_buffer, opacity);
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
