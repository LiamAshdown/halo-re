exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
EXT.update({
    'message_text': ('extern wchar_t empty_string; // 0x00660c34\n'
                     'extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group\n'
                     'extern uint16_t *text_string_list_get_string(datum_index list_id, int16_t index); // 0x5578c0, blam-cc: ECX list, EDX index\n'
                     'extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, blam-cc: EDX count\n'
                     'extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX handle, ESI array\n'
                     'extern uint32_t game_engine_compare_score_to_others(uint32_t subject, int32_t team_mode); // 0x463480\n'
                     'extern wchar_t *game_engine_get_multiplayer_text_list(uint32_t rank); // 0x4633f0'),
})
MT_HELP = '''// A ui\\multiplayer_game_text string, or the empty string without the tag.
static const uint16_t *game_text(int16_t index)
{
    datum_index tag_id = tag_lookup(0x75737472, "ui\\\\multiplayer_game_text");

    return tag_id == 0xffffffff ? (const uint16_t *)&empty_string : text_string_list_get_string(tag_id, index);
}

// The recipient's place text: 0x4633f0 of game_engine_compare_score_to_others(recipient, 1).
static const uint16_t *place_text(datum_index recipient)
{
    return (const uint16_t *)game_engine_get_multiplayer_text_list(game_engine_compare_score_to_others(recipient, 1));
}
'''
SIG = 'uint8_t %s(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)'
NOTE = 'the kill-feed text override (called by chimera__kill_feed as (recipient, type, subject, text, count)); returns whether it built a text. '

emit(0x46b000, 499, 'game_engine_king_build_message_text',
     NOTE + 'Type 0x22 (subject and recipient exist): string 0x9b with the recipient\'s place and the subject team\'s hill seconds (bucket ticks / 30). 0x21 / 0x20: string 0x9c / 0x9d with the subject\'s name and its team\'s hill seconds.',
     ['player_data', 'king_bucket_credit_ticks', 'message_text'], SIG,
     '    uint8_t *player = (uint8_t *)datum_get(subject, player_data);\n    int32_t seconds;\n\n'
     '    switch (message_type) {\n    case 0x22:\n'
     '        if (datum_get(recipient, player_data) == 0 || player == 0) {\n            return 0;\n        }\n'
     '        {\n            const uint16_t *place = place_text(recipient);\n\n'
     '            seconds = king_bucket_credit_ticks[*(int32_t *)(player + 0x20)] / 30;\n'
     '            string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0x9b), place, seconds);\n        }\n        return 1;\n'
     '    case 0x21:\n    case 0x20:\n'
     '        if (player == 0) {\n            return 0;\n        }\n'
     '        seconds = king_bucket_credit_ticks[*(int32_t *)(player + 0x20)] / 30;\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x21 ? 0x9c : 0x9d), player + 4, seconds);\n'
     '        return 1;\n    default:\n        return 0;\n    }\n',
     extra_inc='#include <string.h>\n', helper=MT_HELP)

emit(0x46cac0, 38, 'game_engine_oddball_build_message_text',
     NOTE + '(size is the dispatch head; the arms run to 0x46cde1, table 0x46cde4.) Types 0x20 / 0x21 / 0x23 / 0x24 copy string 0xa2 / 0xa3 / 0x9f / 0xa0; 0x22 / 0x25 format 0xa4 / 0xa1 with the subject\'s name; 0x27 / 0x28 format 0xa6 / 0xa5 with the name and the subject team\'s ball seconds (score / 30); 0x29 (subject and recipient exist) formats 0x9b with the recipient\'s place and those seconds; 0x26 and the rest build nothing.',
     ['player_data', 'king_alt_team_score', 'message_text'], SIG,
     '    uint8_t *player;\n\n'
     '    switch (message_type) {\n'
     '    case 0x20: wcsncpy(text, (const wchar_t *)game_text(0xa2), count); return 1;\n'
     '    case 0x21: wcsncpy(text, (const wchar_t *)game_text(0xa3), count); return 1;\n'
     '    case 0x23: wcsncpy(text, (const wchar_t *)game_text(0x9f), count); return 1;\n'
     '    case 0x24: wcsncpy(text, (const wchar_t *)game_text(0xa0), count); return 1;\n'
     '    case 0x22:\n    case 0x25:\n'
     '        player = (uint8_t *)datum_get(subject, player_data);\n        if (player == 0) {\n            return 0;\n        }\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x22 ? 0xa4 : 0xa1), player + 4);\n        return 1;\n'
     '    case 0x27:\n    case 0x28:\n'
     '        player = (uint8_t *)datum_get(subject, player_data);\n        if (player == 0) {\n            return 0;\n        }\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x27 ? 0xa6 : 0xa5), player + 4,\n'
     '            king_alt_team_score[*(int32_t *)(player + 0x20)] / 30);\n        return 1;\n'
     '    case 0x29:\n'
     '        player = (uint8_t *)datum_get(subject, player_data);\n'
     '        if (datum_get(recipient, player_data) == 0 || player == 0) {\n            return 0;\n        }\n'
     '        {\n            const uint16_t *place = place_text(recipient);\n\n'
     '            string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0x9b), place,\n'
     '                king_alt_team_score[*(int32_t *)(player + 0x20)] / 30);\n        }\n        return 1;\n'
     '    default:\n        return 0;\n    }\n',
     helper=MT_HELP)

emit(0x46e480, 55, 'game_engine_race_build_message_text',
     NOTE + '(size is the dispatch head; the arms run to 0x46e939, tables 0x46e93c / 0x46e960.) 0x23 copies string 0xa7; 0x24 / 0x25 format 0xa8 / 0xa9 with the subject\'s name; 0x20 formats 0xaa with its laps (+0xc6) and lap time (+0xc4 / 30 s); 0x21 / 0x22 format 0xab / 0xac with the name and laps; 0x26 formats 0xad with +0xc8 / 30 s. 0x16 (subject and recipient exist): with the race type (variant dword +0x7c) 2, 0xae with the place when on lap 1 else 0xaf with the place and laps; otherwise 0xb0 with the place once laps + 1 pass the score limit, else 0xb1 with the place, laps + 1 and the limit.',
     ['player_data', 'variant', 'message_text'], SIG,
     '    uint8_t *player;\n\n'
     '    if (message_type == 0x23) {\n        wcsncpy(text, (const wchar_t *)game_text(0xa7), count);\n        return 1;\n    }\n'
     '    if (message_type < 0x16 || message_type > 0x26 || (message_type > 0x16 && message_type < 0x20)) {\n        return 0;\n    }\n'
     '    player = (uint8_t *)datum_get(subject, player_data);\n    if (player == 0) {\n        return 0;\n    }\n'
     '    switch (message_type) {\n'
     '    case 0x24:\n    case 0x25:\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x24 ? 0xa8 : 0xa9), player + 4);\n        return 1;\n'
     '    case 0x20:\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xaa), (int32_t)*(int16_t *)(player + 0xc6),\n'
     '            (double)((float)*(int16_t *)(player + 0xc4) * 0.033333335f));\n        return 1;\n'
     '    case 0x21:\n    case 0x22:\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(message_type == 0x21 ? 0xab : 0xac), player + 4,\n'
     '            (int32_t)*(int16_t *)(player + 0xc6));\n        return 1;\n'
     '    case 0x26:\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xad),\n'
     '            (double)((float)*(int16_t *)(player + 0xc8) * 0.033333335f));\n        return 1;\n'
     '    default: // 0x16\n'
     '        if (datum_get(recipient, player_data) == 0) {\n            return 0;\n        }\n'
     '        if (*(int32_t *)&game_engine_variant.ctf_option_7c == 2) {\n'
     '            if (*(int16_t *)(player + 0xc6) == 1) {\n'
     '                const uint16_t *format = game_text(0xae);\n\n'
     '                string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient));\n            } else {\n'
     '                const uint16_t *format = game_text(0xaf);\n\n'
     '                string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient), (int32_t)*(int16_t *)(player + 0xc6));\n'
     '            }\n            return 1;\n        }\n'
     '        if (*(int16_t *)(player + 0xc6) + 1 > game_engine_variant.score_limit) {\n'
     '            const uint16_t *format = game_text(0xb0);\n\n'
     '            string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient));\n        } else {\n'
     '            const uint16_t *format = game_text(0xb1);\n\n'
     '            string_format_wide_va_bounded(count, (uint16_t *)text, format, place_text(recipient), *(int16_t *)(player + 0xc6) + 1,\n'
     '                game_engine_variant.score_limit);\n        }\n        return 1;\n    }\n',
     helper=MT_HELP)
print('ok')
