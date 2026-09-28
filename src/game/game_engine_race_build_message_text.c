// game_engine_race_build_message_text  (not a Ghidra function; the race game engine definition's +0x6c slot (build_message_text); no C existed, so that
//   stored pointer trapped as unlisted_46e480)
// address 0x46e480, size 55 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46e480..0x46e4b6: the kill-feed text override (called by chimera__kill_feed as
//   (recipient, type, subject, text, count)); returns whether it built a text. (size is the dispatch head; the arms
//   run to 0x46e939, tables 0x46e93c / 0x46e960.) 0x23 copies string 0xa7; 0x24 / 0x25 format 0xa8 / 0xa9 with the
//   subject's name; 0x20 formats 0xaa with its laps (+0xc6) and lap time (+0xc4 / 30 s); 0x21 / 0x22 format 0xab /
//   0xac with the name and laps; 0x26 formats 0xad with +0xc8 / 30 s. 0x16 (subject and recipient exist): with the
//   race type (variant dword +0x7c) 2, 0xae with the place when on lap 1 else 0xaf with the place and laps; otherwise
//   0xb0 with the place once laps + 1 pass the score limit, else 0xb1 with the place, laps + 1 and the limit.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data; // 0x0087a480
extern game_variant game_engine_variant; // 0x006f1c88
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
    datum_index tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text");

    return tag_id == 0xffffffff ? (const uint16_t *)&empty_string : text_string_list_get_string(tag_id, index);
}

// The recipient's place text: 0x4633f0 of game_engine_compare_score_to_others(recipient, 1).
static const uint16_t *place_text(datum_index recipient)
{
    return (const uint16_t *)game_engine_get_multiplayer_text_list(game_engine_compare_score_to_others(recipient, 1));
}

uint8_t game_engine_race_build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    uint8_t *player;

    if (message_type == 0x23) {
        wcsncpy(text, (const wchar_t *)game_text(0xa7), count);
        return 1;
    }
    if (message_type < 0x16 || message_type > 0x26 || (message_type > 0x16 && message_type < 0x20)) {
        return 0;
    }
    player = (uint8_t *)datum_get(subject, player_data);
    if (player == 0) {
        return 0;
    }
    switch (message_type) {
    case 0x24:
    case 0x25:
        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x24 ? 0xa8 : 0xa9), player + 4);
        return 1;
    case 0x20:
        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xaa), (int32_t)*(int16_t *)(player + 0xc6),
            (double)((float)*(int16_t *)(player + 0xc4) * 0.033333335f));
        return 1;
    case 0x21:
    case 0x22:
        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x21 ? 0xab : 0xac), player + 4,
            (int32_t)*(int16_t *)(player + 0xc6));
        return 1;
    case 0x26:
        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xad),
            (double)((float)*(int16_t *)(player + 0xc8) * 0.033333335f));
        return 1;
    default: // 0x16
        if (datum_get(recipient, player_data) == 0) {
            return 0;
        }
        if (*(int32_t *)&game_engine_variant.ctf_option_7c == 2) {
            if (*(int16_t *)(player + 0xc6) == 1) {
                const uint16_t *format = game_text(0xae);

                string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient));
            } else {
                const uint16_t *format = game_text(0xaf);

                string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient), (int32_t)*(int16_t *)(player + 0xc6));
            }
            return 1;
        }
        if (*(int16_t *)(player + 0xc6) + 1 > game_engine_variant.score_limit) {
            const uint16_t *format = game_text(0xb0);

            string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient));
        } else {
            const uint16_t *format = game_text(0xb1);

            string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient), *(int16_t *)(player + 0xc6) + 1,
                game_engine_variant.score_limit);
        }
        return 1;
    }
}
