// multiplayer_game_variant_description_generate  (Ghidra: already named)
// address 0x4b8da0, size 5301 bytes
// name confidence: 0.8   rewrite confidence: 0.7
// evidence: modules.json / out/phase4/networking_types_notes.md ("misattributed", moved to game
// per PLAN.md's networking-s3 catch-up); its only caller,
// src/networking/server_browser_selected_variant_description_build.c (0x4b74e0), already declares
// this function's real cdecl parameter list from its own call-site disassembly:
//     extern void multiplayer_game_variant_description_generate(ticker_text_buffer *ticker,
//         int32_t fraglimit, wchar_t *game_flags_wide, wchar_t *player_flags_wide); // 0x4b8da0
//     // its first cdecl argument is 0x006b5e74 itself (push 0x6b5e74 at 0x4b7588) and the
//     // variant name string rides in EDX (mov edx,ebp)
// That caller's evidence plus this file's own disassembly (bin/halo.exe 0x4b8da0..0x4b9db5,
// disassembled with capstone since Ghidra's decompile of this function is badly broken -- see
// below) together pin every argument and every "uninitialized" local Ghidra printed.
//
// Ghidra's decompile (out/phase2/networking/01.md, tools/pack.py 0x4b8da0) declares this function
// as four stack parameters and then reads ~20 locals (local_268 .. local_224, local_18 ..
// local_4) that are never assigned anywhere in its own text -- a sign that Ghidra's stack-frame
// analysis lost track of writes into those slots. Disassembling the real bytes shows why: this
// function opens by calling four small decoder functions (already rewritten elsewhere in this
// tree, under src/networking/server_browser_custom_options_unpack.c and its three
// server_browser_gametypeN_flags_unpack.c siblings) that take their destination buffer in a
// register (ESI/ECX/EDX) Ghidra never resolved, so their writes never show up as assignments --
// only the later plain `[esp+N]` reads survive in the decompile, looking uninitialized. The two
// decoder destination regions are, in this function's own stack frame:
//   - `engine_extra` (8 bytes) at one register value, immediately followed in memory by
//   - `options` (server_browser_custom_options, ESI's target) at the next register value,
// and, for the oddball branch only, a third, unrelated region
//   - `oddball` (server_browser_gametype3_options) at yet another register value.
// (Addresses recovered from a CFG-validated ESP-symbolic-execution pass over the capstone
// disassembly; every branch merge point checked bit-for-bit consistent, so the offsets below are
// exact, not guessed.) Ghidra's "local_NNN" names are kept in comments next to each field below
// so this rewrite can be cross-checked against out/phase2/networking/01.md line by line.
//
// Also broken in the same way: every one of the ~63 calls to unicode_string_list_get_string()
// this function makes passes its `path` tag name in EAX and its `index` in CX -- both register
// arguments Ghidra dropped, printing every call as a bare `unicode_string_list_get_string()`.
// Recovered the same way (ESP/registers symbolically executed and cross-checked against the
// disassembly at every call site); the `path`/index pairs below are the actual immediates.
//
// register convention: EDX = variant_name (in_EDX; sscanf source for
// server_browser_custom_options_unpack, "%d,%d") is this function's only register argument, in
// that convention's first-available slot; ticker, fraglimit, game_flags_wide, player_flags_wide
// are ordinary cdecl stack parameters (Ghidra's param_1..param_4) in that order, matching the
// caller's own extern declaration.
//
// NOTE: options.teams is game_variant+0x34 (the record is the variant seen from +0x34, see
// types/networking.h). UNSURE: `options.unknown_00[0]` (Ghidra's local_260, tested identically as
// "if (local_260 != 0)" in all five engine branches) is read but never written by any of the
// four decoder calls this function makes, nor by anything else in its own body -- it is either a
// field this function's caller is expected to have pre-filled (nothing in
// server_browser_selected_variant_description_build.c does), or genuinely reads whatever
// leftover stack byte happens to sit there. Left as a literal read of that byte to preserve
// behaviour; not resolved further.
//
// REVIEW (Opus, phase-4 game module review): all 46 unicode_string_list_get_string call sites and
// all 53 swprintf call sites in 0x4b8da0..0x4ba255 were re-extracted from the disassembly and
// checked one by one against this file. Six defects were found and fixed:
//   1. Every "%s%s"-shaped format takes the wide separator at 0x0066af68 (L" | ") as its FIRST
//      %s and the fetched label as its second -- at all 45 such sites the push order is
//      `push <label>; push 0x66af68; push <format>`. The rewrite had been passing the label
//      twice and dropping the separator.
//   2. The ctf branch's `options.unknown_00[0]` block was given an extra
//      unicode_string_list_get_string(..., 25) fetch that the binary never makes; the block's
//      swprintf is (separator, scratch buffer) with no label fetch at all (0x4b8ee7).
//   3. `missing_string_text` (0x00671fac) was read as a pointer rather than taken as the string
//      itself -- see its declaration below.
//   4. Ghidra's `-1 < x` tests are `x >= 0`, not `x >= -1`; oddball.value_10, race[0] and
//      race[1] were each one value too permissive (0x4b943f, 0x4b990e, 0x4b9979: `cmp r,ebx`
//      with ebx == 0, `jl` out).
//   5. The "%d+%d" pair at 0x4b9b0f accepts 0 / 0x96 / 0x12c / 0x1c2 on EACH side; the fourth
//      value 0x1c2 lives in the `jg 0x4b9b29` arm Ghidra folds into a bVar9 flag, and had been
//      dropped from both sets.
//   6. var_vehicles_respawn is indexed by a 0..6 ordinal derived from options.unknown_34, not
//      by unknown_34's own tick count (0x4b9f65..0x4b9fc9).
// Verified correct and left alone: every string index constant (24..46 for the per-engine
// blocks, 1..23 for the shared tail), the `(%s)` / `(%s - "%s")` / L" %s %s" argument lists
// (game_flags_wide and player_flags_wide are the missing second %s Ghidra dropped), the
// engine_index jump table (cases 0/1/3/5 take the body, 2/4 skip -- byte table at 0x4ba260,
// target table at 0x4ba258), the `speed_scale * 100.0f` __ftol (0x672bc4 holds 100.0f) and its
// {50,100,150,200,300,400} set, the friendly-fire penalty's esi = 1/2/3 selection from
// 0x96/0x12c/0x1c2, and the frame layout (`options` at esp+0x20, `engine_extra` at esp+0x18,
// `line` at esp+0x68, `is_custom_variant` at esp+0x13, the oddball block at esp+0x268).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern wchar_t *unicode_string_list_get_string(char *path, int16_t index); // 0x4b8d30, this module (src/game/unicode_string_list_get_string.c)
extern void ticker_text_buffer_append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self); // 0x4b8a60, networking module

// The four packed-option decoders, already rewritten in the networking module. Each takes its
// destination buffer by pointer/register; see those files for the exact blam-cc register
// mapping (EDX/ESI, EAX/ECX, EAX/ECX, ECX/EDX respectively).
extern void server_browser_custom_options_unpack(char *text, server_browser_custom_options *out); // 0x5764a0
extern void server_browser_gametype1_flags_unpack(uint32_t code, server_browser_gametype1_decoded *out); // 0x576890 (ctf)
extern void server_browser_gametype3_flags_unpack(uint32_t code, server_browser_gametype3_options *out); // 0x576a20 (oddball)
extern void server_browser_gametype5_flags_unpack(uint32_t code, int32_t *out); // 0x576990 (race), out[0]/out[1]

// Same globals unicode_string_list_get_string.c declares extern ownership of; this function
// duplicates that function's tag-walk once inline (see the repeated "if (options.unknown_00[0])"
// block below), writing the identical scratch buffer, so it needs the same declarations.
extern tag_instance *tag_instances;            // 0x0087bc14
// Every "%s%s"-shaped line this function builds starts with the same wide separator, held in
// .data at 0x0066af68 (bytes 20 00 7c 00 20 00 00 00, i.e. L" | ") and pushed as the format's
// first %s at all 45 of those sites. Ghidra prints it as `&PTR_DAT_0066af68` because its first
// four bytes happen to look like a pointer; it is the characters themselves.
extern wchar_t ticker_field_separator[];       // 0x0066af68, L" | "
extern wchar_t missing_string_text[];          // 0x00671fac, the characters of L"<missing string>"
extern wchar_t unicode_string_list_scratch_buffer; // 0x006b5c58

// blam-cc: EDX -> variant_name; ticker, fraglimit, game_flags_wide, player_flags_wide are
// ordinary cdecl stack parameters (this function's disassembly: param_1=ticker, param_2=fraglimit,
// param_3=game_flags_wide, param_4=player_flags_wide).
// Builds the multiline server-browser ticker text describing one multiplayer game variant.
// `fraglimit` is GameSpy's packed "fraglimit" key: bits 0-2 select the game engine (1=ctf,
// 2=slayer, 3=oddball, 4=king, 5=race; anything else shows nothing) and the remaining bits are a
// handful of per-engine boolean/enum extras tested directly against `fraglimit` in the ctf/
// slayer/king branches. `variant_name` is GameSpy's packed "gamevariant" key: despite the name it
// is *not* shown as text here -- it is parsed as "%d,%d" into the bulk of the option fields.
void multiplayer_game_variant_description_generate(char *variant_name, ticker_text_buffer *ticker,
    int32_t fraglimit, wchar_t *game_flags_wide, wchar_t *player_flags_wide)
{
    uint32_t engine_index = (uint32_t)fraglimit & 7;
    int is_custom_variant = 0; // Ghidra's bVar3: set once an engine branch shows a non-default extra
    wchar_t line[256];         // Ghidra's local_218[255]; Ghidra's separate local_1a is line[255]
    wchar_t *label_text;       // Ghidra's uVar4 (fetched label/value string)
    wchar_t *suffix_text;      // Ghidra's pwVar6 (second fetch appended with wcscat)

    // Ghidra's local_268/local_264 union: written by the ctf or race decoder below, 8 bytes
    // immediately before `options` in the real stack frame.
    union {
        server_browser_gametype1_decoded ctf;  // engine_index == 1
        int32_t race[2];                       // engine_index == 5
    } engine_extra;

    // Ghidra's local_25c..local_224 (server_browser_custom_options is 0x41 bytes; ESI points at
    // its start, immediately after engine_extra above).
    server_browser_custom_options options;

    // Ghidra's local_18/local_10/local_c/local_8/local_4 (oddball only; a separate stack region,
    // not adjacent to engine_extra/options).
    server_browser_gametype3_options oddball;

    server_browser_custom_options_unpack(variant_name, &options); // blam-cc: EDX=text, ESI=&options
    ticker_text_buffer_append((wchar_t *)L"  ---  ", 0, ticker);

    if (engine_index == 1) { // ctf
        server_browser_gametype1_flags_unpack((uint32_t)fraglimit, &engine_extra.ctf); // blam-cc: EAX=fraglimit, ECX=&engine_extra
        is_custom_variant = 1;
        if (game_flags_wide == 0 || game_flags_wide[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 24);
            swprintf(line, 0xff, L"(%s)", label_text);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 24);
            swprintf(line, 0xff, L"(%s - \"%s\")", label_text, game_flags_wide);
        }
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        // UNSURE: options.unknown_00[0] -- see file header.
        if (options.teams != 0) {
            // Inlined equivalent of unicode_string_list_get_string(path, 0): same tag walk, same
            // scratch buffer (0x006b5c58), just without the round-trip through that function.
            datum_index tag_id = tag_lookup(0x75737472, // 'ustr'
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings");
            wchar_t *rules_text = missing_string_text;
            if (tag_id != k_datum_index_none) {
                UnicodeStringList *list = (UnicodeStringList *)tag_instances[tag_id & 0xffff].data;
                if (list->strings.count > 0) {
                    UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer;
                    int32_t char_count = (int32_t)entry->string.size;
                    if (char_count > 0) {
                        rules_text = (wchar_t *)entry->string.pointer;
                        rules_text[(char_count / 2) - 1] = 0;
                    }
                }
            }
            wcscpy(&unicode_string_list_scratch_buffer, rules_text);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, &unicode_string_list_scratch_buffer);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        // Ghidra's local_268 four flag bytes (engine_extra.ctf.flags[0..3]).
        if (engine_extra.ctf.flags[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 25);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 26);
        }
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        if (engine_extra.ctf.flags[1] != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 27);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (engine_extra.ctf.flags[2] != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 28);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (engine_extra.ctf.flags[3] != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 29);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        // Ghidra's local_264: the packed {low,high} pair read back as one int32 (low | high<<8);
        // the five decoded (low,high) pairs give exactly 1800/3600/5400/9000/18000.
        {
            int32_t packed_low_high = *(int32_t *)&engine_extra.ctf.low;
            if (packed_low_high == 0x1518 || packed_low_high == 0x708 || packed_low_high == 0xe10 ||
                packed_low_high == 9000 || packed_low_high == 18000) {
                label_text = unicode_string_list_get_string(
                    (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 30);
                swprintf(line, 0xff, L"%s%s %d", ticker_field_separator, label_text, packed_low_high / 30);
                line[255] = 0;
                ticker_text_buffer_append(line, 0, ticker);
                is_custom_variant = 1;
            }
        }
    } else if (engine_index == 2) { // slayer
        if (game_flags_wide == 0 || game_flags_wide[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 31);
            swprintf(line, 0xff, L"(%s)", label_text);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 31);
            swprintf(line, 0xff, L"(%s - \"%s\")", label_text, game_flags_wide);
        }
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        if (options.teams != 0) {
            is_custom_variant = 1;
            datum_index tag_id = tag_lookup(0x75737472,
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings");
            wchar_t *rules_text = missing_string_text;
            if (tag_id != k_datum_index_none) {
                UnicodeStringList *list = (UnicodeStringList *)tag_instances[tag_id & 0xffff].data;
                if (list->strings.count > 0) {
                    UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer;
                    int32_t char_count = (int32_t)entry->string.size;
                    if (char_count > 0) {
                        rules_text = (wchar_t *)entry->string.pointer;
                        rules_text[(char_count / 2) - 1] = 0;
                    }
                }
            }
            wcscpy(&unicode_string_list_scratch_buffer, rules_text);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, &unicode_string_list_scratch_buffer);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        if (((uint32_t)fraglimit >> 3 & 1) != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 32);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (((uint32_t)fraglimit >> 4 & 1) != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 33);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (((uint32_t)fraglimit >> 5 & 1) != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 34);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    } else if (engine_index == 3) { // oddball
        server_browser_gametype3_flags_unpack((uint32_t)fraglimit, &oddball); // blam-cc: EAX=fraglimit, ECX=&oddball
        if (game_flags_wide == 0 || game_flags_wide[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 37);
            swprintf(line, 0xff, L"(%s)", label_text);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 37);
            swprintf(line, 0xff, L"(%s - \"%s\")", label_text, game_flags_wide);
        }
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        if (options.teams != 0) {
            is_custom_variant = 1;
            datum_index tag_id = tag_lookup(0x75737472,
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings");
            wchar_t *rules_text = missing_string_text;
            if (tag_id != k_datum_index_none) {
                UnicodeStringList *list = (UnicodeStringList *)tag_instances[tag_id & 0xffff].data;
                if (list->strings.count > 0) {
                    UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer;
                    int32_t char_count = (int32_t)entry->string.size;
                    if (char_count > 0) {
                        rules_text = (wchar_t *)entry->string.pointer;
                        rules_text[(char_count / 2) - 1] = 0;
                    }
                }
            }
            wcscpy(&unicode_string_list_scratch_buffer, rules_text);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, &unicode_string_list_scratch_buffer);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        // 0x4b944f: `mov ecx,ebx` -- the index is oddball.value_10 itself, which picks which of
        // var_speed_with_ball's own strings shows.
        if (oddball.value_10 >= 0 && oddball.value_10 < 3) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\oddball_edit\\var_speed_with_ball",
                (int16_t)oddball.value_10);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (oddball.value_14 > 1) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 38);
            swprintf(line, 0xff, L"%s%s %d", ticker_field_separator, label_text, oddball.value_14);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (oddball.flag0 != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 39);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (oddball.value_10 == 0 || oddball.value_10 == 2) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 41);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            // 0x4b955e: `mov ecx,ebx` -- index is oddball.value_10, not the 41 above.
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\oddball_edit\\var_speed_with_ball",
                (int16_t)oddball.value_10);
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (oddball.value_08 > 0 && oddball.value_08 < 4) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 42);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\oddball_edit\\var_trait_with_ball",
                (int16_t)oddball.value_08);
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (oddball.value_0c > 0 && oddball.value_0c < 4) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 43);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\oddball_edit\\var_trait_with_ball",
                (int16_t)oddball.value_0c);
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    } else if (engine_index == 4) { // king
        if (game_flags_wide == 0 || game_flags_wide[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 35);
            swprintf(line, 0xff, L"(%s)", label_text);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 35);
            swprintf(line, 0xff, L"(%s - \"%s\")", label_text, game_flags_wide);
        }
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        if (options.teams != 0) {
            is_custom_variant = 1;
            datum_index tag_id = tag_lookup(0x75737472,
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings");
            wchar_t *rules_text = missing_string_text;
            if (tag_id != k_datum_index_none) {
                UnicodeStringList *list = (UnicodeStringList *)tag_instances[tag_id & 0xffff].data;
                if (list->strings.count > 0) {
                    UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer;
                    int32_t char_count = (int32_t)entry->string.size;
                    if (char_count > 0) {
                        rules_text = (wchar_t *)entry->string.pointer;
                        rules_text[(char_count / 2) - 1] = 0;
                    }
                }
            }
            wcscpy(&unicode_string_list_scratch_buffer, rules_text);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, &unicode_string_list_scratch_buffer);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        if (((uint32_t)fraglimit >> 3 & 1) != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 36);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    } else if (engine_index == 5) { // race
        server_browser_gametype5_flags_unpack((uint32_t)fraglimit, engine_extra.race); // blam-cc: ECX=fraglimit, EDX=&engine_extra
        if (game_flags_wide == 0 || game_flags_wide[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 44);
            swprintf(line, 0xff, L"(%s)", label_text);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 44);
            swprintf(line, 0xff, L"(%s - \"%s\")", label_text, game_flags_wide);
        }
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        if (options.teams != 0) {
            is_custom_variant = 1;
            datum_index tag_id = tag_lookup(0x75737472,
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings");
            wchar_t *rules_text = missing_string_text;
            if (tag_id != k_datum_index_none) {
                UnicodeStringList *list = (UnicodeStringList *)tag_instances[tag_id & 0xffff].data;
                if (list->strings.count > 0) {
                    UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer;
                    int32_t char_count = (int32_t)entry->string.size;
                    if (char_count > 0) {
                        rules_text = (wchar_t *)entry->string.pointer;
                        rules_text[(char_count / 2) - 1] = 0;
                    }
                }
            }
            wcscpy(&unicode_string_list_scratch_buffer, rules_text);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, &unicode_string_list_scratch_buffer);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        if (engine_extra.race[0] >= 0 && engine_extra.race[0] < 3) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 45);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            // 0x4b9947: `mov ecx,esi` -- index is engine_extra.race[0], not the 45 above.
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\race_edit\\var_race_type",
                (int16_t)engine_extra.race[0]);
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (engine_extra.race[1] >= 0 && engine_extra.race[1] < 3) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 46);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\race_edit\\var_team_scoring",
                (int16_t)engine_extra.race[1]);
            wcscat(line, suffix_text);
            // Ghidra's "goto LAB_004b99ce" lands here: that label's entire body is the single
            // ticker_text_buffer_append call below, then falls through to the shared tail, so a
            // plain call (no jump) reproduces it exactly.
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    }

    ticker_text_buffer_append((wchar_t *)L"  ---  ", 0, ticker);
    label_text = unicode_string_list_get_string(
        (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 1);
    swprintf(line, 0xff, L" %s %s", label_text, player_flags_wide);
    line[255] = 0;
    ticker_text_buffer_append(line, 0, ticker);

    switch (options.lives_per_round) { // Ghidra's local_244
    case 0: case 1: case 3: case 5:
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 2);
        swprintf(line, 0xff, L"%s%s %d", ticker_field_separator, label_text, options.lives_per_round);
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);
        break;
    default:
        break;
    }

    {
        // Ghidra's `iVar5 = __ftol()`: options.float_bits_20 is the speed_scale float's raw bits
        // (0.25 .. 4.0), truncated here as a whole percentage (25 .. 400).
        float speed_scale = *(float *)&options.health_bits;
        int32_t speed_percent = (int32_t)(speed_scale * 100.0f);
        if (speed_percent == 200 || speed_percent == 50 || speed_percent == 100 ||
            speed_percent == 150 || speed_percent == 300 || speed_percent == 400) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 3);
            swprintf(line, 0xff, L"%s%s %d", ticker_field_separator, label_text, speed_percent);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    }

    // Ghidra's local_24c/local_250 "%d+%d" pair, with the same odd short-circuit shape (an
    // unmatched value on either side blanks the line instead of skipping the display).
    {
        // 0x4b9b0f..0x4b9b5f: each side is accepted when it is one of 0 / 0x96 / 0x12c / 0x1c2
        // (0 / 150 / 300 / 450 ticks). The `jg 0x4b9b29` -> `cmp edi,0x1c2` arm supplies the
        // fourth value; Ghidra folds it into a bVar9 flag and it is easy to drop.
        int show_pair = 0;
        if (options.respawn_time == 0 || options.respawn_time == 0x96 ||
            options.respawn_time == 0x12c || options.respawn_time == 0x1c2) {
            show_pair = (options.respawn_time_growth == 0 || options.respawn_time_growth == 0x96 ||
                         options.respawn_time_growth == 0x12c || options.respawn_time_growth == 0x1c2);
        } else {
            line[0] = 0;
        }
        if (show_pair) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 4);
            swprintf(line, 0xff, L"%s%s %d+%d", ticker_field_separator, label_text,
                options.respawn_time / 30, options.respawn_time_growth / 30);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    }

    if (options.suicide_penalty == 0x96 || options.suicide_penalty == 300 || options.suicide_penalty == 0x1c2) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 5);
        swprintf(line, 0xff, L"%s%s %d", ticker_field_separator, label_text, options.suicide_penalty / 30);
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);
    }
    if ((options.flags & 8) == 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 6);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }
    if (options.odd_man_out != 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 7);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }
    if ((options.flags & 0x10) != 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 8);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }
    if ((options.flags & 4) != 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 9);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }

    if (is_custom_variant) {
        if (options.friendly_fire < 4) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 11);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            // 0x4b9d86: `movzx ecx,bl` -- index is options.unknown_38, not the 11 above.
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\teamplay_options_edit\\var_friendly_fire",
                (int16_t)options.friendly_fire);
            wcscat(line, suffix_text);
            if ((options.friendly_fire == 1 || options.friendly_fire == 3) &&
                (options.betrayal_penalty == 0x96 || options.betrayal_penalty == 300 || options.betrayal_penalty == 0x1c2)) {
                // Ghidra's esi=1/2/3 selection, recovered from the disassembly directly (there is
                // no join_game_rules_strings label fetch for this one -- it goes straight to
                // var_friendly_fire_penalty with an index chosen by which constant matched).
                int16_t penalty_display_index =
                    (options.betrayal_penalty == 0x96) ? 1 : (options.betrayal_penalty == 300) ? 2 : 3;
                wcscat(line, L" (+");
                suffix_text = unicode_string_list_get_string(
                    (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\teamplay_options_edit\\var_friendly_fire_penalty",
                    penalty_display_index);
                wcscat(line, suffix_text);
                wcscat(line, L")");
            }
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if ((options.red_vehicle_set & 0xf) < 9) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 12);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\vehicle_options_edit\\var_vehicle_set",
                (int16_t)(options.red_vehicle_set & 0xf));
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if ((options.blue_vehicle_set & 0xf) < 9) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 13);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\vehicle_options_edit\\var_vehicle_set",
                (int16_t)(options.blue_vehicle_set & 0xf));
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    } else if ((options.red_vehicle_set & 0xf) < 9) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 10);
        swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
        suffix_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\vehicle_options_edit\\var_vehicle_set",
            (int16_t)(options.red_vehicle_set & 0xf));
        wcscat(line, suffix_text);
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);
    }

    {
        // 0x4b9f65..0x4b9fc9. unknown_34 is a tick count, but the string it selects is indexed by
        // its *ordinal* in the allowed set: the compiler emits a seven-way compare chain whose
        // arms each load a different ESI (0x4b9f96 `xor esi,esi` .. 0x4b9fb6 `mov esi,6`), and
        // `mov ecx,esi` at 0x4b9ff6 is what reaches var_vehicles_respawn. Passing the raw tick
        // count here would index far off the end of that string list. Any other value skips the
        // line entirely (`jne 0x4ba02a`).
        int32_t vehicle_respawn_index = -1;
        switch (options.vehicle_respawn_time) {
        case 0:      vehicle_respawn_index = 0; break;
        case 0x384:  vehicle_respawn_index = 1; break;  /* 900  */
        case 0x708:  vehicle_respawn_index = 2; break;  /* 1800 */
        case 0xa8c:  vehicle_respawn_index = 3; break;  /* 2700 */
        case 0xe10:  vehicle_respawn_index = 4; break;  /* 3600 */
        case 0x1518: vehicle_respawn_index = 5; break;  /* 5400 */
        case 0x2328: vehicle_respawn_index = 6; break;  /* 9000 */
        default: break;
        }
        if (vehicle_respawn_index >= 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 14);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\vehicle_options_edit\\var_vehicles_respawn",
                (int16_t)vehicle_respawn_index);
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    }

    if (options.weapon_set < 0xe) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 15);
        swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
        suffix_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\item_options_edit\\var_weapon_set",
            (int16_t)options.weapon_set);
        wcscat(line, suffix_text);
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);
    }

    if ((options.flags & 0x20) == 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 16);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
    } else {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 17);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
    }
    ticker_text_buffer_append(line, 0, ticker);

    // Ghidra's local_258/local_258==1 pair, both fetching label 18 and then, either way, a
    // trailing shared value string at index 19.
    if (options.objective_indicator == 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 18);
        swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
    } else {
        if (options.objective_indicator != 1) goto shared_tail; // LAB_004ba19f
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 18);
        swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
    }
    suffix_text = unicode_string_list_get_string(
        (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 19);
    wcscat(line, suffix_text);
    ticker_text_buffer_append(line, 0, ticker);

shared_tail: // LAB_004ba19f
    if ((options.flags & 1) != 0) {
        if ((options.flags & 0x40) == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 21);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 22);
        }
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }
    if ((options.flags & 2) != 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 23);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }
    return;
}

#if 0
Original Ghidra decompilation (0x4b8da0), from tools/pack.py 0x4b8da0 (truncated batch copy in
out/phase2/networking/01.md; full copy below is from `python tools/pack.py 0x4b8da0`):

void multiplayer_game_variant_description_generate
               (undefined4 param_1,uint param_2,short *param_3,undefined4 param_4)

{
  int *piVar1;
  uint *puVar2;
  bool bVar3;
  undefined4 uVar4;
  int iVar5;
  wchar_t *pwVar6;
  undefined **ppuVar7;
  uint uVar8;
  bool bVar9;
  undefined4 local_268;
  int local_264;
  char local_260;
  byte local_25c;
  int local_258;
  char local_254;
  int local_250;
  int local_24c;
  int local_248;
  undefined4 local_244;
  int local_238;
  uint local_234;
  uint local_230;
  int local_22c;
  byte local_228;
  int local_224;
  wchar_t local_218 [255];
  undefined2 local_1a;
  char local_18;
  int local_10;
  int local_c;
  int local_8;
  int local_4;

  uVar8 = param_2 & 7;
  bVar3 = false;
  FUN_005764a0();
  ticker_text_buffer_append(L"  ---  ",0);
  if (uVar8 == 1) {
    FUN_00576890();
    bVar3 = true;
    if ((param_3 == (short *)0x0) || (*param_3 == 0)) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"(%s)",uVar4);
    }
    else {
      uVar4 = unicode_string_list_get_string(param_3);
      FID_conflict_swprintf(local_218,0xff,L"(%s - \"%s\")",uVar4);
    }
    local_1a = 0;
    ticker_text_buffer_append(local_218,0);
    if (local_260 != '\0') {
      uVar8 = tag_lookup(
                        "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings"
                        );
      ppuVar7 = &PTR_DAT_00671fac;
      if ((uVar8 != 0xffffffff) &&
         (piVar1 = *(int **)((uVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 0 < *piVar1)) {
        puVar2 = (uint *)piVar1[1];
        uVar8 = *puVar2;
        if (0 < (int)uVar8) {
          ppuVar7 = (undefined **)puVar2[3];
          *(undefined2 *)((int)ppuVar7 + ((uVar8 & 0xfffffffe) - 2)) = 0;
        }
      }
      _wcscpy((wchar_t *)&DAT_006b5c58,(wchar_t *)ppuVar7);
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,&DAT_006b5c58);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((char)local_268 == '\0') {
      uVar4 = unicode_string_list_get_string();
    }
    else {
      uVar4 = unicode_string_list_get_string();
    }
    FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
    local_1a = 0;
    ticker_text_buffer_append(local_218,0);
    if (local_268._1_1_ != '\0') {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if (local_268._2_1_ != '\0') {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if (local_268._3_1_ != '\0') {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if (local_264 < 0x1519) {
      if ((local_264 == 0x1518) || ((local_264 == 0x708 || (local_264 == 0xe10)))) {
LAB_004b9086:
        uVar4 = unicode_string_list_get_string();
        FID_conflict_swprintf(local_218,0xff,L"%s%s %d",&PTR_DAT_0066af68,uVar4,local_264 / 0x1e);
        local_1a = 0;
        ticker_text_buffer_append(local_218,0);
        bVar3 = true;
      }
    }
    else if ((local_264 == 9000) || (bVar3 = true, local_264 == 18000)) goto LAB_004b9086;
  }
  else if (uVar8 == 2) {
    if ((param_3 == (short *)0x0) || (*param_3 == 0)) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"(%s)",uVar4);
    }
    else {
      uVar4 = unicode_string_list_get_string(param_3);
      FID_conflict_swprintf(local_218,0xff,L"(%s - \"%s\")",uVar4);
    }
    local_1a = 0;
    ticker_text_buffer_append(local_218,0);
    if (local_260 != '\0') {
      bVar3 = true;
      uVar8 = tag_lookup(
                        "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings"
                        );
      ppuVar7 = &PTR_DAT_00671fac;
      if ((uVar8 != 0xffffffff) &&
         (piVar1 = *(int **)((uVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 0 < *piVar1)) {
        puVar2 = (uint *)piVar1[1];
        uVar8 = *puVar2;
        if (0 < (int)uVar8) {
          ppuVar7 = (undefined **)puVar2[3];
          *(undefined2 *)((int)ppuVar7 + ((uVar8 & 0xfffffffe) - 2)) = 0;
        }
      }
      _wcscpy((wchar_t *)&DAT_006b5c58,(wchar_t *)ppuVar7);
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,&DAT_006b5c58);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((param_2 >> 3 & 1) != 0) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((param_2 >> 4 & 1) != 0) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((param_2 >> 5 & 1) != 0) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
  }
  else if (uVar8 == 3) {
    FUN_00576a20();
    if ((param_3 == (short *)0x0) || (*param_3 == 0)) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"(%s)",uVar4);
    }
    else {
      uVar4 = unicode_string_list_get_string(param_3);
      FID_conflict_swprintf(local_218,0xff,L"(%s - \"%s\")",uVar4);
    }
    local_1a = 0;
    ticker_text_buffer_append(local_218,0);
    if (local_260 != '\0') {
      bVar3 = true;
      uVar8 = tag_lookup(
                        "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings"
                        );
      ppuVar7 = &PTR_DAT_00671fac;
      if ((uVar8 != 0xffffffff) &&
         (piVar1 = *(int **)((uVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 0 < *piVar1)) {
        puVar2 = (uint *)piVar1[1];
        uVar8 = *puVar2;
        if (0 < (int)uVar8) {
          ppuVar7 = (undefined **)puVar2[3];
          *(undefined2 *)((int)ppuVar7 + ((uVar8 & 0xfffffffe) - 2)) = 0;
        }
      }
      _wcscpy((wchar_t *)&DAT_006b5c58,(wchar_t *)ppuVar7);
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,&DAT_006b5c58);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((-1 < local_8) && (local_8 < 3)) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if (1 < local_4) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s %d",&PTR_DAT_0066af68,uVar4,local_4);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if (local_18 != '\0') {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((local_8 == 0) || (local_8 == 2)) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
      pwVar6 = (wchar_t *)unicode_string_list_get_string();
      _wcscat(local_218,pwVar6);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((0 < local_10) && (local_10 < 4)) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
      pwVar6 = (wchar_t *)unicode_string_list_get_string();
      _wcscat(local_218,pwVar6);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((0 < local_c) && (local_c < 4)) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
      pwVar6 = (wchar_t *)unicode_string_list_get_string();
      _wcscat(local_218,pwVar6);
LAB_004b99ce:
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
  }
  else if (uVar8 == 4) {
    if ((param_3 == (short *)0x0) || (*param_3 == 0)) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"(%s)",uVar4);
    }
    else {
      uVar4 = unicode_string_list_get_string(param_3);
      FID_conflict_swprintf(local_218,0xff,L"(%s - \"%s\")",uVar4);
    }
    local_1a = 0;
    ticker_text_buffer_append(local_218,0);
    if (local_260 != '\0') {
      bVar3 = true;
      uVar8 = tag_lookup(
                        "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings"
                        );
      ppuVar7 = &PTR_DAT_00671fac;
      if ((uVar8 != 0xffffffff) &&
         (piVar1 = *(int **)((uVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 0 < *piVar1)) {
        puVar2 = (uint *)piVar1[1];
        uVar8 = *puVar2;
        if (0 < (int)uVar8) {
          ppuVar7 = (undefined **)puVar2[3];
          *(undefined2 *)((int)ppuVar7 + ((uVar8 & 0xfffffffe) - 2)) = 0;
        }
      }
      _wcscpy((wchar_t *)&DAT_006b5c58,(wchar_t *)ppuVar7);
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,&DAT_006b5c58);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((param_2 >> 3 & 1) != 0) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
  }
  else if (uVar8 == 5) {
    FUN_00576990();
    if ((param_3 == (short *)0x0) || (*param_3 == 0)) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"(%s)",uVar4);
    }
    else {
      uVar4 = unicode_string_list_get_string(param_3);
      FID_conflict_swprintf(local_218,0xff,L"(%s - \"%s\")",uVar4);
    }
    local_1a = 0;
    ticker_text_buffer_append(local_218,0);
    if (local_260 != '\0') {
      bVar3 = true;
      uVar8 = tag_lookup(
                        "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings"
                        );
      ppuVar7 = &PTR_DAT_00671fac;
      if ((uVar8 != 0xffffffff) &&
         (piVar1 = *(int **)((uVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 0 < *piVar1)) {
        puVar2 = (uint *)piVar1[1];
        uVar8 = *puVar2;
        if (0 < (int)uVar8) {
          ppuVar7 = (undefined **)puVar2[3];
          *(undefined2 *)((int)ppuVar7 + ((uVar8 & 0xfffffffe) - 2)) = 0;
        }
      }
      _wcscpy((wchar_t *)&DAT_006b5c58,(wchar_t *)ppuVar7);
      FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,&DAT_006b5c58);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((-1 < local_268) && (local_268 < 3)) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
      pwVar6 = (wchar_t *)unicode_string_list_get_string();
      _wcscat(local_218,pwVar6);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((-1 < local_264) && (local_264 < 3)) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
      pwVar6 = (wchar_t *)unicode_string_list_get_string();
      _wcscat(local_218,pwVar6);
      goto LAB_004b99ce;
    }
  }
  ticker_text_buffer_append(L"  ---  ",0);
  uVar4 = unicode_string_list_get_string(param_4);
  FID_conflict_swprintf(local_218,0xff,L" %s %s",uVar4);
  local_1a = 0;
  ticker_text_buffer_append(local_218,0);
  switch(local_244) {
  case 0:
  case 1:
  case 3:
  case 5:
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s %d",&PTR_DAT_0066af68,uVar4,local_244);
    local_1a = 0;
    ticker_text_buffer_append(local_218,0);
  }
  iVar5 = __ftol();
  if (iVar5 < 0xc9) {
    if ((iVar5 == 200) || (((iVar5 == 0x32 || (iVar5 == 100)) || (iVar5 == 0x96)))) {
LAB_004b9aca:
      uVar4 = unicode_string_list_get_string(iVar5);
      FID_conflict_swprintf(local_218,0xff,L"%s%s %d",&PTR_DAT_0066af68,uVar4);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
  }
  else if ((iVar5 == 300) || (iVar5 == 400)) goto LAB_004b9aca;
  if (local_24c < 0x12d) {
    if ((local_24c != 300) && (local_24c != 0)) {
      bVar9 = local_24c == 0x96;
      goto LAB_004b9b2f;
    }
LAB_004b9b3b:
    if (local_250 < 0x12d) {
      if (((local_250 == 300) || (local_250 == 0)) || (local_250 == 0x96)) {
LAB_004b9b5f:
        uVar4 = unicode_string_list_get_string();
        FID_conflict_swprintf
                  (local_218,0xff,L"%s%s %d+%d",&PTR_DAT_0066af68,uVar4,local_24c / 0x1e,
                   local_250 / 0x1e);
        local_1a = 0;
        ticker_text_buffer_append(local_218,0);
      }
    }
    else if (local_250 == 0x1c2) goto LAB_004b9b5f;
  }
  else {
    bVar9 = local_24c == 0x1c2;
LAB_004b9b2f:
    if (bVar9) goto LAB_004b9b3b;
    local_218[0] = L'\0';
  }
  if (((local_248 == 0x96) || (local_248 == 300)) || (local_248 == 0x1c2)) {
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s %d",&PTR_DAT_0066af68,uVar4,local_248 / 0x1e);
    local_1a = 0;
    ticker_text_buffer_append(local_218,0);
  }
  if ((local_25c & 8) == 0) {
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
    ticker_text_buffer_append(local_218,0);
  }
  if (local_254 != '\0') {
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
    ticker_text_buffer_append(local_218,0);
  }
  if ((local_25c & 0x10) != 0) {
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
    ticker_text_buffer_append(local_218,0);
  }
  if ((local_25c & 4) != 0) {
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
    ticker_text_buffer_append(local_218,0);
  }
  if (bVar3) {
    if (local_228 < 4) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
      pwVar6 = (wchar_t *)unicode_string_list_get_string();
      _wcscat(local_218,pwVar6);
      if (((local_228 == 1) || (local_228 == 3)) &&
         ((local_224 == 0x96 || ((local_224 == 300 || (local_224 == 0x1c2)))))) {
        _wcscat(local_218,L" (+");
        pwVar6 = (wchar_t *)unicode_string_list_get_string();
        _wcscat(local_218,pwVar6);
        _wcscat(local_218,L")");
      }
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((local_234 & 0xf) < 9) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
      pwVar6 = (wchar_t *)unicode_string_list_get_string();
      _wcscat(local_218,pwVar6);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
    if ((local_230 & 0xf) < 9) {
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
      pwVar6 = (wchar_t *)unicode_string_list_get_string();
      _wcscat(local_218,pwVar6);
LAB_004b9f53:
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
  }
  else if ((local_234 & 0xf) < 9) {
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
    pwVar6 = (wchar_t *)unicode_string_list_get_string();
    _wcscat(local_218,pwVar6);
    goto LAB_004b9f53;
  }
  if (local_22c < 0xa8d) {
    if (((local_22c == 0xa8c) || (local_22c == 0)) || ((local_22c == 900 || (local_22c == 0x708))))
    {
LAB_004b9fc9:
      uVar4 = unicode_string_list_get_string();
      FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
      pwVar6 = (wchar_t *)unicode_string_list_get_string();
      _wcscat(local_218,pwVar6);
      local_1a = 0;
      ticker_text_buffer_append(local_218,0);
    }
  }
  else if (((local_22c == 0xe10) || (local_22c == 0x1518)) || (local_22c == 9000))
  goto LAB_004b9fc9;
  if (local_238 < 0xe) {
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
    pwVar6 = (wchar_t *)unicode_string_list_get_string();
    _wcscat(local_218,pwVar6);
    local_1a = 0;
    ticker_text_buffer_append(local_218,0);
  }
  if ((local_25c & 0x20) == 0) {
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
  }
  else {
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
  }
  ticker_text_buffer_append(local_218,0);
  if (local_258 == 0) {
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
  }
  else {
    if (local_258 != 1) goto LAB_004ba19f;
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s ",&PTR_DAT_0066af68,uVar4);
  }
  pwVar6 = (wchar_t *)unicode_string_list_get_string();
  _wcscat(local_218,pwVar6);
  ticker_text_buffer_append(local_218,0);
LAB_004ba19f:
  if ((local_25c & 1) != 0) {
    if ((local_25c & 0x40) == 0) {
      uVar4 = unicode_string_list_get_string();
    }
    else {
      uVar4 = unicode_string_list_get_string();
    }
    FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
    ticker_text_buffer_append(local_218,0);
  }
  if ((local_25c & 2) != 0) {
    uVar4 = unicode_string_list_get_string();
    FID_conflict_swprintf(local_218,0xff,L"%s%s",&PTR_DAT_0066af68,uVar4);
    ticker_text_buffer_append(local_218,0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
