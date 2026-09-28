"""game_engine_queue_multiplayer_sound (0x46be40) takes ESI sound, EDI player, stack broadcast. Every C caller
modeled one argument (usually the broadcast flag, as Ghidra showed it). Retrofit the callee and all callers
from the 39 binary call sites (scratchpad/sound_sites.py)."""
import os, re
os.chdir(r'C:\Users\Liam-\halo-re')

DECL = ('extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); '
        '// 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast')
decl_re = re.compile(r'^extern void game_engine_queue_multiplayer_sound\(int32_t sound_index\);[^\n]*$', re.M)

EDITS = {
    'src/game/ctf_engine_flag_tick.c': [
        ('game_engine_queue_multiplayer_sound(1);\n                    game_engine_ctf_reset_team_return_credit',
         'game_engine_queue_multiplayer_sound(0x25 + (*(int16_t *)((uint8_t *)flag_obj + 0xb8) != 0), 0xffffffff, 1); // 0x468e5d..0x468e79\n'
         '                    game_engine_ctf_reset_team_return_credit'),
        ('game_engine_queue_multiplayer_sound(1);\n        ctf_team_return_credit_active[team] = 0;',
         'game_engine_queue_multiplayer_sound(team != 0 ? 9 : 0xc, 0xffffffff, 1); // 0x469026..0x469037\n'
         '        ctf_team_return_credit_active[team] = 0;'),
    ],
    'src/game/game_engine_begin_end_game_sequence.c': [
        ('game_engine_queue_multiplayer_sound(0);', 'game_engine_queue_multiplayer_sound(1, 0xffffffff, 0); // 0x45fdb1..0x45fdcf'),
    ],
    'src/game/game_engine_build_kill_feed_message_text.c': [
        ('            wcsncpy(out, text, buffer_size);\n            game_engine_queue_multiplayer_sound(0); // UNSURE args\n',
         '            wcsncpy(out, text, buffer_size);\n'
         '            // FIXED 2026-09-28: this group queues no sound -- the only five calls to 0x46be40 in the\n'
         '            //   function sit in the adjusted-type 0x0e..0x12 arms (0x45eced..0x45eeb7).\n'),
        ('            game_engine_queue_multiplayer_sound(0); // UNSURE args\n',
         '            // FIXED 2026-09-28 from the arms at 0x45ec87/0x45ecfa/0x45ed6c/0x45eddf/0x45ee52 (jump table\n'
         '            //   0x45f198 entries 0x0e..0x12): sounds 0x10, 0x0f, 0x0e, 0x11, 0x12, no player, no broadcast.\n'
         '            {\n                static const uint8_t arm_sound[5] = { 0x10, 0x0f, 0x0e, 0x11, 0x12 };\n\n'
         '                game_engine_queue_multiplayer_sound(arm_sound[adjusted_type - 0x0e], 0xffffffff, 0);\n            }\n'),
    ],
    'src/game/game_engine_ctf_notify_flag_carried_throttled.c': [
        ('game_engine_queue_multiplayer_sound(1);', 'game_engine_queue_multiplayer_sound(0x1c, (datum_index)target_player, 1); // 0x4689f1..0x4689f8'),
    ],
    'src/game/game_engine_ctf_on_flag_captured.c': [
        ('game_engine_queue_multiplayer_sound(1);', 'game_engine_queue_multiplayer_sound(0x2a, flag_index, 1); // 0x46de12..0x46de26, EDI still the argument'),
    ],
    'src/game/game_engine_ctf_player_flag_tick.c': [
        ('game_engine_broadcast_kill_feed_by_relationship(player_index, 0x25, 0x2a, 0x28, player_index);\n                        game_engine_queue_multiplayer_sound(1);',
         'game_engine_broadcast_kill_feed_by_relationship(player_index, 0x25, 0x2a, 0x28, player_index);\n'
         '                        game_engine_queue_multiplayer_sound(p->team != 0 ? 9 : 0xc, 0xffffffff, 1); // 0x46988d..0x46989f'),
        ('                game_engine_queue_multiplayer_sound(1);\n                ctf_team_return_credit_active[team] = 1;',
         '                game_engine_queue_multiplayer_sound(p->team != 0 ? 8 : 0xb, 0xffffffff, 1); // 0x4698fe..0x46990d\n'
         '                ctf_team_return_credit_active[team] = 1;'),
    ],
    'src/game/game_engine_ctf_player_touch_flag.c': [
        ('game_engine_queue_multiplayer_sound(1);', 'game_engine_queue_multiplayer_sound(p->team != 0 ? 0xa : 0xd, 0xffffffff, 1); // 0x46895b..0x46896d'),
    ],
    'src/game/game_engine_ctf_reset_round.c': [
        ('game_engine_queue_multiplayer_sound(0x16);', 'game_engine_queue_multiplayer_sound(0x16, 0xffffffff, 0);'),
    ],
    'src/game/game_engine_ctf_score_flag.c': [
        ('game_engine_queue_multiplayer_sound(1);', 'game_engine_queue_multiplayer_sound(0x1a, team, 1); // 0x46e0bc..0x46e0cb: EDI is the first argument (a player handle)'),
    ],
    'src/game/game_engine_end_game_sequence_stage1.c': [
        ('game_engine_queue_multiplayer_sound(0);', 'game_engine_queue_multiplayer_sound(1, 0xffffffff, 0); // 0x4670c2..0x4670dc'),
    ],
    'src/game/game_engine_king_reset_round.c': [
        ('game_engine_queue_multiplayer_sound(teams ? 0x20 : 0x24);', 'game_engine_queue_multiplayer_sound(teams ? 0x20 : 0x24, 0xffffffff, 0);'),
    ],
    'src/game/game_engine_koth_alt_scorer_tick.c': [
        ('== 900) {\n            game_engine_queue_multiplayer_sound(1);',
         '== 900) {\n            game_engine_queue_multiplayer_sound(current_game_engine != 0 && game_engine_teams_enabled_flag != 0\n'
         '                ? 5 + 2 * (p->team != 0) : 3, 0xffffffff, 1); // 0x46c27d..0x46c2a8'),
        ('== 0x708) {\n            game_engine_queue_multiplayer_sound(1);',
         '== 0x708) {\n            game_engine_queue_multiplayer_sound(current_game_engine != 0 && game_engine_teams_enabled_flag != 0\n'
         '                ? 4 + 2 * (p->team != 0) : 2, 0xffffffff, 1); // 0x46c2c8..0x46c2f5'),
    ],
    'src/game/game_engine_koth_dispatch_player_scoring.c': [
        ('game_engine_queue_multiplayer_sound(0);', 'game_engine_queue_multiplayer_sound(0x2a, 0xffffffff, 0); // 0x46c5a4..0x46c5ad (EDX, the zero remainder, is the broadcast)'),
    ],
    'src/game/game_engine_koth_player_tick.c': [
        ('? 5 + 2 * (p->team != 0) : 3);', '? 5 + 2 * (p->team != 0) : 3, 0xffffffff, 1);'),
        ('? 4 + 2 * (p->team != 0) : 2);', '? 4 + 2 * (p->team != 0) : 2, 0xffffffff, 1);'),
        ('                // 0x46ac66 also loads EDI (the recipient player) here; the established\n'
         '                // single-parameter signature cannot carry it. UNSURE.\n'
         '                game_engine_queue_multiplayer_sound(0x2a);',
         '                // 0x46ac66 loads EDI (the recipient) from the first argument.\n'
         '                game_engine_queue_multiplayer_sound(0x2a, player_index, 1);'),
    ],
    'src/game/game_engine_koth_relocate_object_hill.c': [
        ('game_engine_queue_multiplayer_sound(1);', 'game_engine_queue_multiplayer_sound(0x1e, 0xffffffff, 1); // 0x46c1fd..0x46c207'),
    ],
    'src/game/game_engine_koth_update_hill_occupancy_state.c': [
        ('                game_engine_queue_multiplayer_sound(1);\n                king_hill_state_globals.hill_ticks = 0;\n                king_hill_state_globals.occupant',
         '                game_engine_queue_multiplayer_sound(0x27, 0xffffffff, 1); // 0x46ae57..0x46ae60\n'
         '                king_hill_state_globals.hill_ticks = 0;\n                king_hill_state_globals.occupant'),
        ('                if (king_hill_state_globals.hill_ticks > 300) {\n                    game_engine_queue_multiplayer_sound(1);',
         '                if (king_hill_state_globals.hill_ticks > 300) {\n'
         '                    game_engine_queue_multiplayer_sound(0x27, 0xffffffff, 1); // 0x46ad56..0x46ad60'),
        ('    if (king_hill_state_globals.hill_ticks == 300) {\n        game_engine_queue_multiplayer_sound(1);',
         '    if (king_hill_state_globals.hill_ticks == 300) {\n        game_engine_queue_multiplayer_sound(0x28, 0xffffffff, 1); // 0x46aea3..0x46aeac'),
    ],
    'src/game/game_engine_player_ready_to_respawn.c': [
        ('game_engine_queue_multiplayer_sound(0);',
         'game_engine_queue_multiplayer_sound(p->respawn_timer == 1 ? 0x1f : 0x1d, 0xffffffff, 0); // 0x461004..0x461034'),
    ],
    'src/game/game_engine_race_unknown_48.c': [
        ('game_engine_queue_multiplayer_sound(teams ? 0x22 : 0x14);', 'game_engine_queue_multiplayer_sound(teams ? 0x22 : 0x14, 0xffffffff, 0);'),
    ],
    'src/game/game_engine_slayer_reset_round.c': [
        ('game_engine_queue_multiplayer_sound(teams ? 0x23 : 0x15);', 'game_engine_queue_multiplayer_sound(teams ? 0x23 : 0x15, 0xffffffff, 0);'),
    ],
    'src/game/game_engine_update_teleporter.c': [
        ('game_engine_queue_multiplayer_sound(0);', 'game_engine_queue_multiplayer_sound(0x1b, 0xffffffff, 0); // 0x4618ff..0x461909'),
    ],
    'src/objects/object_throttled_multiplayer_sound_event.c': [
        ('game_engine_queue_multiplayer_sound(0);', 'game_engine_queue_multiplayer_sound(0x2b, 0xffffffff, 0); // 0x4ee390..0x4ee39a'),
    ],
}

for path, edits in EDITS.items():
    s = open(path, encoding='utf-8').read()
    head, sep, tail = s.partition('#if 0')
    n = len(decl_re.findall(head))
    assert n == 1, (path, n)
    head = decl_re.sub(DECL, head)
    for old, new in edits:
        assert head.count(old) == 1, (path, old[:60], head.count(old))
        head = head.replace(old, new)
    if 'FIXED 2026-09-28 (mp sound)' not in head:
        head = head.replace('\n\n#include', '\n// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the\n'
                            '//   call(s) here now pass all three as the binary loads them (they passed one value before).\n\n#include', 1)
    open(path, 'w', encoding='utf-8').write(head + sep + tail)

# the callee
p = 'src/game/game_engine_queue_multiplayer_sound.c'
s = open(p, encoding='utf-8').read()
head, sep, tail = s.partition('#if 0')
start = head.index('// Queues announcer sound')
head = head[:start] + '''// FIXED 2026-09-28 from objdump 0x46be40..0x46bea4: the real inputs are ESI sound, EDI player and the stack
//   broadcast byte (forced to 0 unless hosting); every caller was retrofitted to pass all three from the binary.
// Queues the sound for the player, refusing once 5 are queued, and starts playback when it is the only entry
// (or at once when the sound is disabled).
void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast)
{
    int32_t count;

    if (network_game_mode != 2) {
        broadcast = 0;
    }
    if (multiplayer_sound_enabled[sound_index] != 0) {
        int32_t duration = game_engine_get_multiplayer_sound_duration_ticks(sound_index) + 5;

        count = multiplayer_sound_queue_count;
        if (count < k_maximum_queued_multiplayer_sounds) {
            multiplayer_sound_request *slot = &multiplayer_sound_queue[count];

            slot->player = player;
            slot->sound_index = sound_index;
            slot->remaining_ticks = duration;
            slot->broadcast = broadcast;
            count++;
            multiplayer_sound_queue_count = count;
        }
        if (count != 1) {
            return;
        }
    }
    game_engine_play_multiplayer_sound(sound_index, player, broadcast);
}

'''
head = head.replace('rewrite confidence: 0.25', 'rewrite confidence: 0.85', 1)
open(p, 'w', encoding='utf-8').write(head + sep + tail)

# the generator library, for later callbacks
p = 'scratchpad/engine_lib.py'
s = open(p, encoding='utf-8').read()
s = s.replace("'extern void game_engine_queue_multiplayer_sound(int32_t sound_index); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast (the C models only the sound)'",
              "'" + DECL + "'")
open(p, 'w', encoding='utf-8').write(s)
print('ok')
