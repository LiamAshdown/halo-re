// game_engine_ctf_build_message_text  (not a Ghidra function; the ctf game engine definition's +0x6c slot (build_message_text); no C existed, so that
//   stored pointer trapped as unlisted_469300)
// address 0x469300, size 1079 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469300..0x469736: the kill-feed text override (called by chimera__kill_feed as
//   (recipient, type, subject, text, count)); returns whether it built a text. Table 0x469738 over types 0x20..0x31:
//   0x20 formats 0x8c with both teams' captures; 0x21..0x23 (recipient exists) format 0x8d..0x8f with the recipient
//   team's and the other team's captures; 0x24 empties the text; 0x25..0x2f copy strings 0x90, 0x91 (0x26 and 0x27),
//   0x92..0x99; 0x30 / 0x31 format L"%s (%s)" with string 0x98 / 0x99 and the flag auto-return time (m:ss).
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "fn_game.h"
#include <wchar.h>

extern data_array *player_data; // 0x0087a480
extern int32_t ctf_team_flag_touch_count[2]; // 0x006b0e98
extern wchar_t empty_string; // 0x00660c34
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern uint16_t *text_string_list_get_string(datum_index list_id, int16_t index); // 0x5578c0, blam-cc: ECX list, EDX index
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, blam-cc: EDX count
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX handle, ESI array


extern int32_t ctf_flag_auto_return_ticks; // 0x006b0eb0
extern void game_time_format_minutes_seconds(uint32_t ticks, uint32_t count, wchar_t *dest); // 0x466530, blam-cc: ECX ticks

// A ui\multiplayer_game_text string, or the empty string without the tag.
static const uint16_t *game_text(int16_t index)
{
    datum_index tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text");

    return tag_id == 0xffffffff ? (const uint16_t *)&empty_string : text_string_list_get_string(tag_id, index);
}

// The recipient's place text: 0x4633f0 of game_engine_compare_score_to_others(recipient, 1).
static const uint16_t *place_text(datum_index recipient)
{
    return (const uint16_t *)game_engine_get_multiplayer_text_list(game_engine_compare_score_to_others(recipient, 1));
}

uint8_t game_engine_ctf_build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    static const int16_t copy_string[11] = { 0x90, 0x91, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99 };

    switch (message_type) {
    case 0x20:
        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0x8c), ctf_team_flag_touch_count[0], ctf_team_flag_touch_count[1]);
        return 1;
    case 0x21:
    case 0x22:
    case 0x23: {
        uint8_t *player = (uint8_t *)datum_get(recipient, player_data);
        int32_t team;

        if (player == 0) {
            return 0;
        }
        team = ((struct player *)player)->team;
        string_format_wide_va_bounded(count, (uint16_t *)text, game_text((int16_t)(0x8d + message_type - 0x21)),
            ctf_team_flag_touch_count[team], ctf_team_flag_touch_count[(team + 1) % 2]);
        return 1;
    }
    case 0x24:
        string_format_wide_va_bounded(count, (uint16_t *)text, (const uint16_t *)&empty_string);
        return 1;
    case 0x30:
    case 0x31: {
        wchar_t time[0x20];

        game_time_format_minutes_seconds((uint32_t)ctf_flag_auto_return_ticks, 0x20, time);
        string_format_wide_va_bounded(count, (uint16_t *)text, (const uint16_t *)L"%s (%s)", game_text(message_type == 0x30 ? 0x98 : 0x99),
            time); // 0x006607d0
        return 1;
    }
    default:
        if (message_type >= 0x25 && message_type <= 0x2f) {
            wcsncpy(text, (const wchar_t *)game_text(copy_string[message_type - 0x25]), count);
            return 1;
        }
        return 0;
    }
}
