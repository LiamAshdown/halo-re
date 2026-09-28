"""0x46bfe0 reads ESI (the ball index): it becomes the placement's owner team (0x46c01a) and the marker type filter
(ECX for 0x46beb0 at 0x46c018); every caller loads ESI with its loop index (0x46c13b, 0x46c7c8, 0x46d4d7).
0x46c1a0 takes only EAX: the type filter is the object's +0xb8 (0x46c1cb), and ctf_flag_object_clear_carrier gets
the object (EBX = EAX at 0x46c1b1) and the found position (EDI = &local at 0x46c20f)."""
import os
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

DECL_OLD = 'extern void game_engine_koth_relocate_hill_marker(void); // 0x46bfe0'
DECL_NEW = 'extern void game_engine_koth_relocate_hill_marker(int32_t ball_index); // 0x46bfe0, blam-cc: ESI ball_index'

edit('src/game/game_engine_koth_relocate_hill_marker.c', [
    ('void game_engine_koth_relocate_hill_marker(void)\n',
     '// FIXED 2026-09-28: ESI is the ball index -- 0x46c01a stores it as the placement owner team and 0x46c018 passes\n'
     '//   it as 0x46beb0\'s type filter (ECX); every caller loads ESI with its loop index.\n'
     '// blam-cc: ESI -> ball_index\n'
     'void game_engine_koth_relocate_hill_marker(int32_t ball_index)\n'),
    ('            object_placement_data_initialize(&placement, ball_tag, (datum_index)0xffffffff);\n',
     '            object_placement_data_initialize(&placement, ball_tag, (datum_index)0xffffffff);\n'
     '            placement.owner_team = (int16_t)ball_index;\n'),
    ('game_engine_koth_find_marker_position(&placement.position, 1); // UNSURE: type_filter',
     'game_engine_koth_find_marker_position(&placement.position, (int16_t)ball_index); // was 1 (UNSURE); fixed'),
])
edit('src/game/game_engine_oddball_initialize_for_new_game.c', [
    (DECL_OLD, DECL_NEW),
    ('                game_engine_koth_relocate_hill_marker();', '                game_engine_koth_relocate_hill_marker(i);'),
])
edit('src/game/game_engine_oddball_reset_objects.c', [
    (DECL_OLD, DECL_NEW),
    ('                game_engine_koth_relocate_hill_marker();', '                game_engine_koth_relocate_hill_marker(i);'),
])
edit('src/game/game_engine_koth_relocate_object_hill.c', [
    ('// blam-cc: EAX -> object_index, EBX -> forwarded_flag_object_index, EDI -> forwarded_position\n',
     '// FIXED 2026-09-28: 0x46c1b1 copies EAX into EBX and 0x46c20f points EDI at the position 0x46beb0 found (type\n'
     '//   filter: the object\'s +0xb8, 0x46c1cb) before ctf_flag_object_clear_carrier; nothing is forwarded.\n'
     '// blam-cc: EAX -> object_index\n'),
    ('void game_engine_koth_relocate_object_hill(uint32_t object_index,\n    datum_index forwarded_flag_object_index, real_point3d *forwarded_position)\n',
     'void game_engine_koth_relocate_object_hill(uint32_t object_index)\n'),
    ('game_engine_koth_find_marker_position(&discarded_position, 1); // UNSURE: type_filter guess',
     'game_engine_koth_find_marker_position(&discarded_position, *(int16_t *)((uint8_t *)obj + 0xb8));'),
    ('ctf_flag_object_clear_carrier(forwarded_flag_object_index, forwarded_position);',
     'ctf_flag_object_clear_carrier(object_index, &discarded_position);'),
])
edit('src/game/game_engine_koth_ball_idle_tick.c', [
    ('extern void game_engine_koth_relocate_object_hill(uint32_t object_index,\n'
     '    datum_index forwarded_flag_object_index, real_point3d *forwarded_position); // 0x46c1a0, this batch',
     'extern void game_engine_koth_relocate_object_hill(uint32_t object_index); // 0x46c1a0, blam-cc: EAX object_index'),
    ('game_engine_koth_relocate_object_hill(object_handle, (datum_index)0xffffffff, (real_point3d *)0);',
     'game_engine_koth_relocate_object_hill(object_handle);'),
])
print('ok')
