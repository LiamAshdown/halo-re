/**
 * Oddball game engine text builders.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

#include "halo/game/game1_oddball.hpp"

extern "C" {
extern data_array *player_data;
extern int32_t king_alt_team_score[16];
extern wchar_t empty_string;
extern datum_index tag_lookup(tag_group group, char *path);
extern uint16_t *text_string_list_get_string(datum_index list_id, int16_t index);
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...);
extern void *datum_get(datum_index handle, data_array *array);
extern uint32_t game_engine_compare_score_to_others(uint32_t subject, int32_t team_mode);
extern wchar_t *game_engine_get_multiplayer_text_list(uint32_t rank);
extern game_variant game_engine_variant;
extern int32_t king_alt_player_score[];
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...);
extern void game_time_format_minutes_seconds(uint32_t ticks, uint32_t unused, wchar_t *dest);
}

namespace halo::game::engine1 {

/**
 * File-local helper of Oddball: game text.
 */
const uint16_t *Oddball::game_text(int16_t index)
{
    datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");

    return tag_id == 0xffffffff ? (const uint16_t *)&empty_string : text_string_list_get_string(tag_id, index);
}

/**
 * File-local helper of Oddball: place text.
 */
const uint16_t *Oddball::place_text(datum_index recipient)
{
    return (const uint16_t *)game_engine_get_multiplayer_text_list(game_engine_compare_score_to_others(recipient, 1));
}

/**
 * Builds the text of an oddball event message for a recipient.
 *
 * @address 0x46cac0
 */
uint8_t Oddball::build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    uint8_t *player;

    switch (message_type) {
    case 0x20: wcsncpy(text, (const wchar_t *)game_text(0xa2), count); return 1;
    case 0x21: wcsncpy(text, (const wchar_t *)game_text(0xa3), count); return 1;
    case 0x23: wcsncpy(text, (const wchar_t *)game_text(0x9f), count); return 1;
    case 0x24: wcsncpy(text, (const wchar_t *)game_text(0xa0), count); return 1;
    case 0x22:
    case 0x25:
        player = (uint8_t *)datum_get(subject, player_data);
        if (player == 0) {
            return 0;
        }
        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x22 ? 0xa4 : 0xa1), player + 4);
        return 1;
    case 0x27:
    case 0x28:
        player = (uint8_t *)datum_get(subject, player_data);
        if (player == 0) {
            return 0;
        }
        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x27 ? 0xa6 : 0xa5), player + 4,
            king_alt_team_score[((struct player *)player)->team] / 30);
        return 1;
    case 0x29:
        player = (uint8_t *)datum_get(subject, player_data);
        if (datum_get(recipient, player_data) == 0 || player == 0) {
            return 0;
        }
        {
            const uint16_t *place = place_text(recipient);

            string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0x9b), place,
                king_alt_team_score[((struct player *)player)->team] / 30);
        }
        return 1;
    default:
        return 0;
    }
}

/**
 * Builds the per-player score text shown for oddball.
 *
 * @address 0x46cf70
 */
wchar_t *Oddball::build_player_text(datum_index player, wchar_t *buffer)
{
    int32_t score = king_alt_player_score[player & 0xffff];

    if (game_engine_variant.engine.oddball.ball_type == 2) {
        string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", score);
    } else {
        game_time_format_minutes_seconds((uint32_t)score, 0x100, buffer);
    }
    return buffer;
}

/**
 * File-local helper of Oddball: multiplayer text.
 */
uint16_t *Oddball::multiplayer_text(int16_t index)
{
    datum_index list = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");

    return list == 0xffffffff ? (uint16_t *)L"" : text_string_list_get_string(list, index);
}

/**
 * Builds the scoreboard header text for oddball.
 *
 * @address 0x46cfc0
 */
wchar_t *Oddball::build_score_header_text(wchar_t *buffer)
{
    wcscpy(buffer, (const wchar_t *)multiplayer_text((int16_t)(game_engine_variant.engine.oddball.ball_type == 2 ? 0x9a : 0x9e)));
    return buffer;
}

}
