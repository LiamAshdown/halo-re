exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gen_engine12.py').read().split("SIG = ")[0].split("exec(open(r'C:\\Users\\Liam-\\halo-re\\scratchpad\\engine_lib.py').read())")[1])
SIG = 'uint8_t %s(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)'
EXT['ctf_text'] = ('extern int32_t ctf_flag_auto_return_ticks; // 0x006b0eb0\n'
                   'extern void game_time_format_minutes_seconds(uint32_t ticks, uint32_t count, wchar_t *dest); // 0x466530, blam-cc: ECX ticks')

emit(0x469300, 1079, 'game_engine_ctf_build_message_text',
     'the kill-feed text override (called by chimera__kill_feed as (recipient, type, subject, text, count)); returns whether it built a text. Table 0x469738 over types 0x20..0x31: 0x20 formats 0x8c with both teams\' captures; 0x21..0x23 (recipient exists) format 0x8d..0x8f with the recipient team\'s and the other team\'s captures; 0x24 empties the text; 0x25..0x2f copy strings 0x90, 0x91 (0x26 and 0x27), 0x92..0x99; 0x30 / 0x31 format L"%s (%s)" with string 0x98 / 0x99 and the flag auto-return time (m:ss).',
     ['player_data', 'ctf_team_flag_touch_count', 'message_text', 'ctf_text'], SIG,
     '    static const int16_t copy_string[11] = { 0x90, 0x91, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99 };\n\n'
     '    switch (message_type) {\n'
     '    case 0x20:\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0x8c), ctf_team_flag_touch_count[0], ctf_team_flag_touch_count[1]);\n'
     '        return 1;\n'
     '    case 0x21:\n    case 0x22:\n    case 0x23: {\n'
     '        uint8_t *player = (uint8_t *)datum_get(recipient, player_data);\n        int32_t team;\n\n'
     '        if (player == 0) {\n            return 0;\n        }\n'
     '        team = *(int32_t *)(player + 0x20);\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, game_text((int16_t)(0x8d + message_type - 0x21)),\n'
     '            ctf_team_flag_touch_count[team], ctf_team_flag_touch_count[(team + 1) % 2]);\n        return 1;\n    }\n'
     '    case 0x24:\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, (const uint16_t *)&empty_string);\n        return 1;\n'
     '    case 0x30:\n    case 0x31: {\n        wchar_t time[0x20];\n\n'
     '        game_time_format_minutes_seconds((uint32_t)ctf_flag_auto_return_ticks, 0x20, time);\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, (const uint16_t *)L"%s (%s)", game_text(message_type == 0x30 ? 0x98 : 0x99),\n'
     '            time); // 0x006607d0\n        return 1;\n    }\n'
     '    default:\n'
     '        if (message_type >= 0x25 && message_type <= 0x2f) {\n'
     '            wcsncpy(text, (const wchar_t *)game_text(copy_string[message_type - 0x25]), count);\n            return 1;\n        }\n'
     '        return 0;\n    }\n',
     helper=MT_HELP)
print('ok')
