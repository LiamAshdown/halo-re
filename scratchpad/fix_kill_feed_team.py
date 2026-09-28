"""0x460ba0 takes ESI = message type (skipped when -1), BL = broadcast and a stack team, and calls chimera__kill_feed
with (recipient, recipient, message, -1, BL) (0x460be1..0x460bea). 0x468460 takes only EAX = team and sends 0x2f
to team % 2 and 0x2e to (team + 1) % 2, BL 1. ctf_engine_flag_tick's two direct calls are 0x2b / 0x2c with BL 1."""
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

TO_TEAM = ('extern void game_engine_broadcast_kill_feed_to_team(int32_t message_type, int32_t team, uint8_t broadcast); '
           '// 0x460ba0, blam-cc: ESI message_type, BL broadcast, stack team')

p = 'src/game/game_engine_broadcast_kill_feed_to_team.c'
s = open(p, encoding='utf-8').read()
head, sep, tail = s.partition('#if 0')
start = head.index('\n// blam-cc: unaff_ESI -> broadcast_enabled, unaff_EBX -> ebx_broadcast, stack -> team\n') + 1
head = head[:start] + '''// FIXED 2026-09-28 from objdump 0x460ba0..0x460c04: ESI is the message type (players are skipped when it is -1),
//   BL the broadcast byte, and each matching player gets chimera__kill_feed(recipient, recipient, message, -1, BL)
//   (0x460be1..0x460bea); the earlier version treated ESI as an enable flag and forwarded invented arguments.
// blam-cc: ESI -> message_type, BL -> broadcast, stack -> team
void game_engine_broadcast_kill_feed_to_team(int32_t message_type, int32_t team, uint8_t broadcast)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    element = data_iterator_next(&iter);
    while (element != 0) {
        player *p = (player *)element;

        if (p->team == team && message_type != -1) {
            chimera__kill_feed(iter.index, (int32_t)iter.index, (uint32_t)message_type, 0xffffffff, (char)broadcast);
        }
        element = data_iterator_next(&iter);
    }
}

'''
head = re.sub(r'rewrite confidence: [0-9.]+', 'rewrite confidence: 0.85', head, count=1)
open(p, 'w', encoding='utf-8').write(head + sep + tail)

p = 'src/game/game_engine_ctf_notify_both_teams.c'
s = open(p, encoding='utf-8').read()
head, sep, tail = s.partition('#if 0')
start = head.index('extern void game_engine_broadcast_kill_feed_to_team(')
head = head[:start] + TO_TEAM + '''

// FIXED 2026-09-28 from objdump 0x468460..0x46849f: only EAX (the team) is an input; the calls load ESI = 0x2f / 0x2e
//   and BL = 1 themselves.
// blam-cc: EAX -> team
void game_engine_ctf_notify_both_teams(int32_t team)
{
    game_engine_broadcast_kill_feed_to_team(0x2f, team % 2, 1);
    game_engine_broadcast_kill_feed_to_team(0x2e, (team + 1) % 2, 1);
}

'''
open(p, 'w', encoding='utf-8').write(head + sep + tail)

edit('src/game/ctf_engine_flag_tick.c', [
    ('extern void game_engine_ctf_notify_both_teams(int32_t team, int32_t forwarded_broadcast_enabled,\n'
     '    uint32_t forwarded_message_type, datum_index forwarded_subject, char forwarded_broadcast); // 0x468460, this batch',
     'extern void game_engine_ctf_notify_both_teams(int32_t team); // 0x468460, blam-cc: EAX team'),
    ('extern void game_engine_broadcast_kill_feed_to_team(int32_t broadcast_enabled, int32_t team,\n'
     '    uint32_t forwarded_message_type, datum_index forwarded_subject, char forwarded_broadcast); // 0x460ba0',
     TO_TEAM),
    ('game_engine_ctf_notify_both_teams((int32_t)toggled, 0, 0, (datum_index)0, 0); // UNSURE forwarded args',
     'game_engine_ctf_notify_both_teams((int32_t)toggled);'),
    ('        game_engine_broadcast_kill_feed_to_team((team != 0) ? 9 : 0xc, team, 0x2b,\n            (datum_index)0xffffffff, 0);\n'
     '        game_engine_broadcast_kill_feed_to_team((team != 0) ? 9 : 0xc, other_team, 0x2c,\n            (datum_index)0xffffffff, 0);\n',
     '        // FIXED 2026-09-28: 0x469049..0x469075 load ESI = 0x2b / 0x2c and BL = 1.\n'
     '        game_engine_broadcast_kill_feed_to_team(0x2b, team, 1);\n'
     '        game_engine_broadcast_kill_feed_to_team(0x2c, other_team, 1);\n'),
])
print('ok')
