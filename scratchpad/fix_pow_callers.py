"""Three callers of _CIpow (0x6283c0, pow(ST1, ST0)) called it with no operands, and 0x465690 takes (player, opacity)
on the stack -- the opacity feeds both the 0.69 background alpha (0x4656da) and the result text alpha (0x465739)."""
import os, re
os.chdir(r'C:\Users\Liam-\halo-re')

def edit(path, pairs):
    s = open(path, encoding='utf-8').read()
    head, sep, tail = s.partition('#if 0')
    for old, new in pairs:
        if head.count(old) == 0 and head.count(new) >= 1:
            continue
        assert head.count(old) == 1, (path, old[:70], head.count(old))
        head = head.replace(old, new)
    open(path, 'w', encoding='utf-8').write(head + sep + tail)

POW = 'extern double pow(double base, double exponent); // C runtime (the retail copy is the CRT _CIpow at 0x6283c0)'

edit('src/game/game_engine_rate_location_ally_bonus.c', [
    ('extern float FUN_006283c0(void); // 0x6283c0, not in this batch; UNSURE purpose', POW),
    ('bonus = bonus + FUN_006283c0();',
     '// FIXED 2026-09-28: 0x461d23..0x461d3f computes pow(1 - (distance - 1) * 0.2, 0.6) (0x00673008 is the double 0.6f).\n'
     '                    bonus = bonus + (float)pow((double)(1.0f - (distance - 1.0f) * 0.2f), (double)0.6f);'),
])
edit('src/game/hud_draw_teammate_nameplate.c', [
    ('extern float FUN_006283c0(void); // 0x6283c0, not in this batch; UNSURE exact meaning', POW),
    ('hud_draw_teammate_nameplate_text(name, FUN_006283c0() * 0.5f);',
     '// FIXED 2026-09-28: 0x45e5e8..0x45e649: the scale is pow(min(+0x80, 10) * 0.1, 1.9) * 0.5\n'
     '                //   (0x00672c30 is the double 1.9f).\n'
     '                hud_draw_teammate_nameplate_text(name,\n'
     '                    (float)pow((double)((float)(p->unknown_80 < 10 ? p->unknown_80 : 10) * 0.1f), (double)1.9f) * 0.5f);'),
])
edit('src/game/hud_update_teammate_nameplate_fade.c', [
    ('extern float FUN_006283c0(void); // 0x6283c0, not in this batch; UNSURE exact meaning', POW),
    ('extern void game_engine_rasterize_in_game_score(datum_index player_handle, float opacity,\n    int32_t unknown_param_2);',
     'extern void game_engine_rasterize_in_game_score(datum_index subject_player, float opacity);'),
    ('    game_engine_rasterize_in_game_score(player_handle, FUN_006283c0(), 0); // UNSURE: not\n'
     '        // `opacity`; matches Ghidra literally. The third argument is not visible here.\n',
     '    // FIXED 2026-09-28: 0x45f2f0..0x45f304 passes (player, pow(opacity, 1.9)) -- two stack arguments.\n'
     '    game_engine_rasterize_in_game_score(player_handle, (float)pow((double)opacity, (double)1.9f));\n'),
])

p = 'src/game/game_engine_rasterize_in_game_score.c'
s = open(p, encoding='utf-8').read()
head, sep, tail = s.partition('#if 0')
head = head.replace('// blam-cc: EAX -> subject_player, stack -> (text_scale, unknown_param_2)',
                    '// FIXED 2026-09-28: both inputs are stack arguments (0x4656ad reads the player at +4, 0x4656da and 0x465739 the\n'
                    '//   opacity at +8); the earlier version took the player from EAX, the opacity one slot late and the result\n'
                    '//   text alpha from a third argument that no caller passes.\n'
                    '// blam-cc: stack -> subject_player, opacity')
head = re.sub(r'void game_engine_rasterize_in_game_score\(datum_index subject_player, float text_scale,\s*int32_t unknown_param_2\)',
              'void game_engine_rasterize_in_game_score(datum_index subject_player, float opacity)', head)
assert 'float opacity)' in head
head = head.replace('*(int32_t *)&params.alpha = unknown_param_2; // raw dword copy (the binary moves it with mov)',
                    'params.alpha = opacity; // the binary copies the dword with mov')
head = head.replace('text_scale', 'opacity')
assert 'unknown_param_2;' not in head and 'text_scale' not in head
open(p, 'w', encoding='utf-8').write(head + sep + tail)
print('ok')
