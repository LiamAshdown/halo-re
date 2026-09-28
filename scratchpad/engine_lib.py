"""Shared generator for multiplayer game engine callbacks (cdecl, reached through game_engine_definition slots)."""
import os, textwrap
os.chdir(r'C:\Users\Liam-\halo-re')

SLOT_OF = {}
for line in os.popen('python scratchpad/engine_slots.py').read().splitlines():
    p = line.split()
    if len(p) >= 4 and p[2].startswith('+0x') and p[3] != '?':
        SLOT_OF.setdefault(int(p[0], 16), (p[1], p[2], p[3]))

EXT = {
    'player_data': 'extern data_array *player_data; // 0x0087a480',
    'object_data': 'extern data_array *object_data; // 0x008603b0',
    'variant': 'extern game_variant game_engine_variant; // 0x006f1c88',
    'current_game_engine': 'extern void *current_game_engine; // 0x006f1d20',
    'teams': 'extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc',
    'network_game_mode': 'extern int16_t network_game_mode; // 0x00719720',
    'game_time': 'extern uint8_t *game_time; // 0x006f1d6c (game_time_globals *, +0x0c the tick)',
    'sound': 'extern void game_engine_queue_multiplayer_sound(int32_t sound_index); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast (the C models only the sound)',
    'format_d': 'extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest',
    'format_time': 'extern void game_time_format_minutes_seconds(uint32_t ticks, uint32_t unused, wchar_t *dest); // 0x466530, blam-cc: ECX ticks',
    'gamespy_team_score': 'extern void FUN_00616640(void *buffer, int32_t value); // 0x616640, GameSpy query-report field writer (networking phase)',
    'ctf_team_flag_touch_count': 'extern int32_t ctf_team_flag_touch_count[2]; // 0x006b0e98',
    'king_bucket_credit_ticks': 'extern int32_t king_bucket_credit_ticks[16]; // 0x006b0ec0',
    'king_hill_player_in_hill': 'extern uint8_t king_hill_player_in_hill[16]; // 0x006b0f40',
    'king_alt_team_score': 'extern int32_t king_alt_team_score[16]; // 0x006b114c (oddball team score)',
    'king_alt_player_score': 'extern int32_t king_alt_player_score[]; // 0x006b118c (oddball player score)',
    'king_hill_occupant_table': 'extern uint32_t king_hill_occupant_table[16]; // 0x006b120c',
    'bucket_scores': 'extern int32_t game_engine_bucket_scores[16]; // 0x006b1318',
    'ctf_globals_live': 'extern uint32_t ctf_globals_live; // 0x006b1290 (ctf_globals, first dword: the team flag mask)',
    'ctf_team_captured_flags_mask': 'extern uint32_t ctf_team_captured_flags_mask[]; // 0x006b12d4',
    'slayer_scores': ('extern int32_t slayer_team_score[16]; // 0x006b13d8\n'
                      'extern int32_t slayer_player_score[16]; // 0x006b1418\n'
                      'extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE\n'
                      'extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE'),
}

def P(expr):
    return '((uint8_t *)player_data->data + ((%s) & 0xffff) * 0x200)' % expr

def emit(addr, size, name, note, ext, sig, body, extra_inc=''):
    engine, slot, field = SLOT_OF.get(addr, ('?', '?', '?'))
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    src = ('// %s  (not a Ghidra function; the %s game engine definition\'s %s slot (%s); no C existed, so that\n'
           '//   stored pointer trapped as unlisted_%x)\n// address 0x%x, size %d bytes\n'
           '// name confidence: 0.6   rewrite confidence: 0.85\n%s// blam-cc: cdecl (called through the engine definition)\n\n'
           '#include "tags.h"\n#include "memory.h"\n#include "math.h"\n#include "game.h"\n#include <wchar.h>\n%s\n%s\n\n%s\n{\n%s}\n'
           % (name, engine, slot, field, addr, addr, size, wr, extra_inc, '\n'.join(EXT[e] for e in ext), sig % name, body))
    p = 'src/game/%s.c' % name
    assert not os.path.exists(p), p
    open(p, 'w', encoding='utf-8').write(src)
