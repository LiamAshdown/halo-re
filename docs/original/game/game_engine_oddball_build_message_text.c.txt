// game_engine_oddball_build_message_text  (not a Ghidra function; the oddball game engine definition's +0x6c slot (build_message_text); no C existed, so that
//   stored pointer trapped as unlisted_46cac0)
// address 0x46cac0, size 38 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46cac0..0x46cae5: the kill-feed text override (called by chimera__kill_feed as
//   (recipient, type, subject, text, count)); returns whether it built a text. (size is the dispatch head; the arms
//   run to 0x46cde1, table 0x46cde4.) Types 0x20 / 0x21 / 0x23 / 0x24 copy string 0xa2 / 0xa3 / 0x9f / 0xa0; 0x22 /
//   0x25 format 0xa4 / 0xa1 with the subject's name; 0x27 / 0x28 format 0xa6 / 0xa5 with the name and the subject
//   team's ball seconds (score / 30); 0x29 (subject and recipient exist) formats 0x9b with the recipient's place and
//   those seconds; 0x26 and the rest build nothing.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480
extern int32_t king_alt_team_score[16]; // 0x006b114c (oddball team score)
extern wchar_t empty_string; // 0x00660c34
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern uint16_t *text_string_list_get_string(datum_index list_id, int16_t index); // 0x5578c0, blam-cc: ECX list, EDX index
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, blam-cc: EDX count
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX handle, ESI array
extern uint32_t game_engine_compare_score_to_others(uint32_t subject, int32_t team_mode); // 0x463480
extern wchar_t *game_engine_get_multiplayer_text_list(uint32_t rank); // 0x4633f0

// A ui\multiplayer_game_text string, or the empty string without the tag.
static const uint16_t *game_text(int16_t index)
{
    datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");

    return tag_id == 0xffffffff ? (const uint16_t *)&empty_string : text_string_list_get_string(tag_id, index);
}

// The recipient's place text: 0x4633f0 of game_engine_compare_score_to_others(recipient, 1).
static const uint16_t *place_text(datum_index recipient)
{
    return (const uint16_t *)game_engine_get_multiplayer_text_list(game_engine_compare_score_to_others(recipient, 1));
}

uint8_t game_engine_oddball_build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
