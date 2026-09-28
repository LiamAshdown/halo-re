"""0x460c10 forwards BL (a broadcast byte every caller sets: 1 at the ctf sites, 0 at 0x46ca77 / 0x46ce4e) and the
recipient (EDI, pushed again as the first stack argument) to chimera__kill_feed 0x460a30 (0x460ce1..0x460ce5);
the C passed -1 and 0. 0x468910 loads EBX = 1 itself (0x46894e) before game_engine_player_profile_cache_sync_all."""
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

OLD = '    int32_t no_source_message, int32_t message_a, int32_t message_b, uint32_t subject); // 0x460c10'
NEW = '    int32_t no_source_message, int32_t message_a, int32_t message_b, uint32_t subject, uint8_t broadcast); // 0x460c10, blam-cc: BL broadcast'

edit('src/game/game_engine_broadcast_kill_feed_by_relationship.c', [
    ('void game_engine_broadcast_kill_feed_by_relationship(uint32_t source_player, int32_t no_source_message,\n'
     '    int32_t message_a, int32_t message_b, uint32_t subject)\n',
     '// FIXED 2026-09-28: BL is a fifth input (a broadcast byte); 0x460ce1..0x460ce5 pushes it, the subject, the message\n'
     '//   and the recipient (EDI, also the register argument) for chimera__kill_feed.\n'
     'void game_engine_broadcast_kill_feed_by_relationship(uint32_t source_player, int32_t no_source_message,\n'
     '    int32_t message_a, int32_t message_b, uint32_t subject, uint8_t broadcast)\n'),
    ("chimera__kill_feed(iter.index, 0xffffffff, (uint32_t)message, subject, '\\0'); // UNSURE: last 2 chimera__kill_feed args",
     'chimera__kill_feed(iter.index, (int32_t)iter.index, (uint32_t)message, subject, (char)broadcast);'),
])

edit('src/game/game_engine_ctf_on_flag_captured.c', [
    (OLD, NEW),
    ('game_engine_broadcast_kill_feed_by_relationship(flag_index, 0x23, 0x24, 0x22, flag_index);',
     'game_engine_broadcast_kill_feed_by_relationship(flag_index, 0x23, 0x24, 0x22, flag_index, 1); // BL = 1 at 0x46de85'),
    ('game_engine_broadcast_kill_feed_by_relationship(flag_index, 0x20, 0x21, 0x22, flag_index);',
     'game_engine_broadcast_kill_feed_by_relationship(flag_index, 0x20, 0x21, 0x22, flag_index, 1);'),
])
edit('src/game/game_engine_ctf_player_flag_tick.c', [
    (OLD, NEW),
    ('game_engine_broadcast_kill_feed_by_relationship(player_index, 0x25, 0x2a, 0x28, player_index);',
     'game_engine_broadcast_kill_feed_by_relationship(player_index, 0x25, 0x2a, 0x28, player_index, 1); // BL = 1 at 0x469886'),
    ('game_engine_broadcast_kill_feed_by_relationship(player_index, 0xffffffff, 0x29, 0x26, player_index);',
     'game_engine_broadcast_kill_feed_by_relationship(player_index, 0xffffffff, 0x29, 0x26, player_index, 1); // BL = 1 at 0x469928'),
])
edit('src/game/game_engine_ctf_player_touch_flag.c', [
    (OLD, NEW),
    ('game_engine_broadcast_kill_feed_by_relationship(player_index, 0x21, 0x23, 0x22, player_index);',
     'game_engine_broadcast_kill_feed_by_relationship(player_index, 0x21, 0x23, 0x22, player_index, 1); // BL = 1 at 0x46897a'),
    ('\n// blam-cc: stack -> player_index, EAX -> team, EBX -> forwarded_commit\n',
     '\n// FIXED 2026-09-28: 0x46894e loads EBX = 1 itself for the profile cache sync; EBX is not an input.\n'
     '// blam-cc: stack -> player_index, EAX -> team\n'),
    ('void game_engine_ctf_player_touch_flag(uint32_t player_index, int32_t team, int32_t forwarded_commit)',
     'void game_engine_ctf_player_touch_flag(uint32_t player_index, int32_t team)'),
    ('game_engine_player_profile_cache_sync_all(forwarded_commit, (void *)0xffffffff);',
     'game_engine_player_profile_cache_sync_all(1, (void *)0xffffffff);'),
])
edit('src/game/game_engine_koth_player_eligible_to_score.c', [
    (OLD, NEW),
    ('game_engine_broadcast_kill_feed_by_relationship(player_index, 0x20, 0x21, 0x22, player_index);',
     'game_engine_broadcast_kill_feed_by_relationship(player_index, 0x20, 0x21, 0x22, player_index, 0); // BL = 0 at 0x46ce4e'),
])
print('ok')
