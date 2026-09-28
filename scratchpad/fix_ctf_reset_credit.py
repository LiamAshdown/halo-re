"""0x468840 sets EBX = the object handle (0x46884a) and EDI = its team's flag stand (0x468877) itself before calling
ctf_flag_object_clear_carrier; they are not forwarded inputs. 0x4688b0 tail-jumps to it with EAX = its first stack
argument (the flag object). Fix both and their callers."""
import os, re
os.chdir(r'C:\Users\Liam-\halo-re')

def edit(path, pairs):
    s = open(path, encoding='utf-8').read()
    head, sep, tail = s.partition('#if 0')
    for old, new in pairs:
        assert head.count(old) == 1, (path, old[:70], head.count(old))
        head = head.replace(old, new)
    open(path, 'w', encoding='utf-8').write(head + sep + tail)

OLD_DECL = ('extern void game_engine_ctf_reset_team_return_credit(uint32_t object_index,\n'
            '    datum_index forwarded_flag_object_index, real_point3d *forwarded_position);')
NEW_DECL = 'extern void game_engine_ctf_reset_team_return_credit(uint32_t object_index);'

edit('src/game/game_engine_ctf_reset_team_return_credit.c', [
    ('// blam-cc: EAX -> object_index, EBX -> forwarded_flag_object_index, EDI -> forwarded_position\n',
     '// FIXED 2026-09-28: 0x46884a copies the object handle into EBX and 0x468877 loads EDI with the team\'s flag stand\n'
     '//   before the call to ctf_flag_object_clear_carrier, so neither is a forwarded input; the object is the flag.\n'
     '// blam-cc: EAX -> object_index\n'),
    ('void game_engine_ctf_reset_team_return_credit(uint32_t object_index,\n    datum_index forwarded_flag_object_index, real_point3d *forwarded_position)\n',
     'void game_engine_ctf_reset_team_return_credit(uint32_t object_index)\n'),
    ('ctf_flag_object_clear_carrier(forwarded_flag_object_index, forwarded_position);',
     'ctf_flag_object_clear_carrier(object_index, ctf_team_flag_stand_position[team]);'),
    ('rewrite confidence: 0.4', 'rewrite confidence: 0.8'),
])

edit('src/game/game_engine_ctf_player_drop_flag.c', [
    (OLD_DECL + ' // 0x468840, this batch', NEW_DECL + ' // 0x468840, blam-cc: EAX object_index'),
    ('// blam-cc: EAX -> player_index, EBX -> forwarded_flag_object_index, EDI -> forwarded_position\n',
     '// FIXED 2026-09-28: 0x4688f8 hands 0x468840 its own first stack argument (the flag object; 0x468a20 pushes it),\n'
     '//   not forwarded registers.\n'
     '// blam-cc: EAX -> player_index, stack -> flag_object_index\n'),
    ('void game_engine_ctf_player_drop_flag(uint32_t player_index,\n    datum_index forwarded_flag_object_index, real_point3d *forwarded_position)\n',
     'void game_engine_ctf_player_drop_flag(uint32_t player_index, datum_index flag_object_index)\n'),
    ('game_engine_ctf_reset_team_return_credit(unit_index, forwarded_flag_object_index, forwarded_position);',
     'game_engine_ctf_reset_team_return_credit(flag_object_index);'),
])

edit('src/game/ctf_engine_flag_tick.c', [
    (OLD_DECL, NEW_DECL),
    ('game_engine_ctf_reset_team_return_credit(flag_handle, (datum_index)0xffffffff, (real_point3d *)0); // UNSURE forwarded args',
     'game_engine_ctf_reset_team_return_credit(flag_handle); // FIXED 2026-09-28: 0x468840 takes only EAX'),
    ('        game_engine_broadcast_kill_feed_to_team((team != 0) ? 9 : 0xc, other_team, 0x2c,\n            (datum_index)0xffffffff, 0);\n    }\n',
     '        game_engine_broadcast_kill_feed_to_team((team != 0) ? 9 : 0xc, other_team, 0x2c,\n            (datum_index)0xffffffff, 0);\n'
     '        // FIXED 2026-09-28: 0x46907d..0x469080 then resets the credit for the flag (the first argument).\n'
     '        game_engine_ctf_reset_team_return_credit(flag_handle);\n    }\n'),
])

edit('src/game/game_engine_ctf_player_flag_tick.c', [
    (OLD_DECL, NEW_DECL),
    ('game_engine_ctf_reset_team_return_credit(player_index, (datum_index)0xffffffff, (real_point3d *)0); // UNSURE forwarded args',
     'game_engine_ctf_reset_team_return_credit(flag_handle); // FIXED 2026-09-28: 0x4698a7 loads EAX from the first argument'),
])
print('ok')
