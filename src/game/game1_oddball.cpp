/**
 * Oddball game engine text builders.
 */

#include "tags.h"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/game/constants.hpp"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

#include "halo/game/game1_oddball.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern data_array *player_data;
extern int32_t king_alt_team_score[16];
extern wchar_t empty_string;
extern game_variant game_engine_variant;
extern int32_t king_alt_player_score[];
}

namespace halo::game::engine1 {

/**
 * File-local helper of Oddball: game text.
 */
const uint16_t *Oddball::game_text(int16_t index)
{
    datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);

    return tag_id == halo::k_dword_none ? (const uint16_t *)&empty_string : halo::text::text_string_list_get_string(tag_id, index);
}

/**
 * File-local helper of Oddball: place text.
 */
const uint16_t *Oddball::place_text(datum_index recipient)
{
    return (const uint16_t *)halo::game::game_engine_get_multiplayer_text_list(halo::game::game_engine_compare_score_to_others(recipient, 1));
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
        player = (uint8_t *)halo::memory::datum_get(subject, player_data);
        if (player == 0) {
            return 0;
        }
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x22 ? 0xa4 : 0xa1), player + 4);
        return 1;
    case 0x27:
    case 0x28:
        player = (uint8_t *)halo::memory::datum_get(subject, player_data);
        if (player == 0) {
            return 0;
        }
        halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x27 ? 0xa6 : 0xa5), player + 4,
            king_alt_team_score[((struct player *)player)->team] / 30);
        return 1;
    case 0x29:
        player = (uint8_t *)halo::memory::datum_get(subject, player_data);
        if (halo::memory::datum_get(recipient, player_data) == 0 || player == 0) {
            return 0;
        }
        {
            const uint16_t *place = place_text(recipient);

            halo::text::string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0x9b), place,
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
    int32_t score = king_alt_player_score[player & halo::k_datum_slot_mask];

    if (game_engine_variant.engine.oddball.ball_type == 2) {
        halo::text::string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", score);
    } else {
        halo::game::game_time_format_minutes_seconds((uint32_t)score, 0x100, buffer);
    }
    return buffer;
}

/**
 * File-local helper of Oddball: multiplayer text.
 */
uint16_t *Oddball::multiplayer_text(int16_t index)
{
    datum_index list = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);

    return list == 0xffffffff ? (uint16_t *)L"" : halo::text::text_string_list_get_string(list, index);
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
