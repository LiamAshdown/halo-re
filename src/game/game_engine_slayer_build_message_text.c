// game_engine_slayer_build_message_text  (not a Ghidra function; the slayer game engine definition's +0x6c slot (build_message_text); no C existed, so that
//   stored pointer trapped as unlisted_46f610)
// address 0x46f610, size 469 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46f610..0x46f7e4: the kill-feed text override (called by chimera__kill_feed as
//   (recipient, type, subject, text, count)). Type 0x16 for a live recipient: with teams string 0xb5 with the
//   recipient's place text (0x4633f0 of game_engine_compare_score_to_others(recipient, 1)), its own score, its team's
//   score and the score limit; without, string 0xb6 with the place, its team slot's score and the limit. Type 0x20
//   for an existing subject: string 0xb4 with the subject's name (+0x04). Returns whether it built a text.
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
extern game_variant game_engine_variant; // 0x006f1c88
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc
extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE
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

// A live player of the given handle (index in range, salt 0 or matching), else 0.
static uint8_t *player_if_valid(datum_index handle)
{
    int16_t index = (int16_t)handle;
    int16_t salt = (int16_t)(handle >> 16);
    uint8_t *player;

    if (handle == 0xffffffff || index < 0 || index >= *(int16_t *)((uint8_t *)player_data + 0x20)) {
        return 0;
    }
    player = (uint8_t *)player_data->data + index * *(int16_t *)((uint8_t *)player_data + 0x22);
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt)) {
        return 0;
    }
    return player;
}

uint8_t game_engine_slayer_build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)
{
    if (message_type == 0x16) {
        uint8_t *player = player_if_valid(recipient);
        const uint16_t *place;
        int32_t team;

        if (player == 0) {
            return 0;
        }
        place = (const uint16_t *)game_engine_get_multiplayer_text_list(game_engine_compare_score_to_others(recipient, 1));
        team = *(int32_t *)(((uint8_t *)player_data->data + ((recipient) & 0xffff) * 0x200) + 0x20);
        if (game_engine_teams_enabled_flag != 0) {
            string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb5), place, slayer_player_score[recipient & 0xffff],
                slayer_team_score[team], game_engine_variant.score_limit);
        } else {
            string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb6), place, slayer_team_score[team],
                game_engine_variant.score_limit);
        }
        return 1;
    }
    if (message_type == 0x20) {
        uint8_t *player = (uint8_t *)datum_get(subject, player_data);

        if (player == 0) {
            return 0;
        }
        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb4), player + 4);
        return 1;
    }
    return 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
