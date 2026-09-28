exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
import re

# 0x4633f0 reads a stack argument: the rank game_engine_compare_score_to_others returns (flag bits low, place high).
p = 'src/game/game_engine_get_multiplayer_text_list.c'
s = open(p, encoding='utf-8').read()
head, sep, tail = s.partition('#if 0')
start = head.index('wchar_t *game_engine_get_multiplayer_text_list(void)')
head = head[:start] + '''// FIXED 2026-09-28 from objdump 0x4633f0..0x463471: the function takes the rank from
//   game_engine_compare_score_to_others on the stack (flag bits in the low word, the place in the high word) and
//   returns ui\\multiplayer_game_text string 0x66 + index: 0x23 for flags 4|1, 0x21 / 0x22 for flag 4 in
//   place 0 / 1, 0x20 for flag 2, else the place (+0x10 with flag 1). Every caller is a build_message_text slot.
// blam-cc: stack -> rank
wchar_t *game_engine_get_multiplayer_text_list(uint32_t rank)
{
    int16_t place = (int16_t)(rank >> 16);
    int32_t index;
    datum_index tag_id;

    if ((rank & 4) != 0 && (rank & 1) != 0) {
        index = 0x23;
    } else if ((rank & 4) != 0 && place == 0) {
        index = 0x21;
    } else if ((rank & 4) != 0 && place == 1) {
        index = 0x22;
    } else if ((rank & 2) != 0) {
        index = 0x20;
    } else {
        index = place;
        if ((rank & 1) != 0) {
            index += 0x10;
        }
    }
    tag_id = tag_lookup(0x75737472, "ui\\\\multiplayer_game_text");
    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return text_string_list_get_string(tag_id, (int16_t)(index + 0x66));
}

'''
head = head.replace('rewrite confidence: 0.5', 'rewrite confidence: 0.85', 1)
open(p, 'w', encoding='utf-8').write(head + sep + tail)

EXT.update({
    'slayer_update': ('extern int32_t game_engine_state_value; // 0x0087aa10\n'
                      'extern uint8_t custom_waypoints[]; // 0x006f1888 (0x20 bytes each)\n'
                      'extern void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position, const char *icon_name,\n'
                      '    float height_offset, datum_index player_filter, int16_t team_filter); // 0x462260, blam-cc: EAX owner, CX slot, EBX position, EDI icon\n'
                      'extern void game_engine_player_select_random_target(datum_index player_or_all); // 0x46f1a0\n'
                      'extern uint8_t game_engine_player_respawn_priority_gate(uint32_t player_index); // 0x463100, blam-cc: EDX player_index\n'
                      'extern void game_engine_begin_end_game_sequence(void); // 0x45fd90'),
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
'''

emit(0x46f7f0, 397, 'game_engine_slayer_update',
     'with variant +0x7d the player\'s speed (+0x6c) above 1 decays by 1/9000 a tick down to 1; with +0x7c a speed below 1 grows by 1/90000 up to 1. With +0x7e (targets) the player\'s waypoint slot is cleared and, when its +0x88 target has a unit, re-registered "target_blue" over that unit for this player only; as the server a player with a unit and no target, or whose target passes 0x463100, gets a new random target. A team reaching the score limit begins the end-game sequence.',
     ['player_data', 'object_data', 'network_game_mode', 'variant', 'slayer_scores', 'slayer_update'], 'void %s(datum_index player_index)',
     '    uint8_t *player = %s;\n    datum_index target;\n\n' % P('player_index') +
     '    if (game_engine_variant.ctf_option_7d != 0 && *(float *)(player + 0x6c) > 1.0f) {\n'
     '        float speed = *(float *)(player + 0x6c) - 0.000111111112f;\n\n'
     '        *(float *)(player + 0x6c) = speed > 1.0f ? speed : 1.0f;\n    }\n'
     '    if (game_engine_variant.ctf_option_7c != 0 && *(float *)(player + 0x6c) < 1.0f) {\n'
     '        float speed = *(float *)(player + 0x6c) + 0.0000111111112f;\n\n'
     '        *(float *)(player + 0x6c) = speed <= 1.0f ? speed : 1.0f;\n    }\n'
     '    if (game_engine_variant.ctf_option_7e != 0) {\n'
     '        memset(custom_waypoints + (int16_t)player_index * 0x20, 0, 0x20);\n'
     '        target = *(datum_index *)(player + 0x88);\n'
     '        if (target != 0xffffffff) {\n            datum_index unit_index = *(datum_index *)(%s + 0x34);\n\n' % P('target') +
     '            if (unit_index != 0xffffffff) {\n'
     '                uint8_t *unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 12 + 8);\n\n'
     '                custom_waypoint_register(0xffffffff, (int16_t)player_index, (real_point3d *)(unit + 0xa0), "target_blue", 0.0f,\n'
     '                    player_index, -1);\n            }\n        }\n'
     '        if (network_game_mode == 2) {\n'
     '            if (*(datum_index *)(player + 0x34) != 0xffffffff && *(datum_index *)(player + 0x88) == 0xffffffff) {\n'
     '                game_engine_player_select_random_target(player_index);\n            }\n'
     '            target = *(datum_index *)(player + 0x88);\n'
     '            if (target != 0xffffffff && game_engine_player_respawn_priority_gate(target) != 0) {\n'
     '                game_engine_player_select_random_target(player_index);\n            }\n        }\n    }\n'
     '    if (slayer_team_score[*(int32_t *)(player + 0x20)] >= game_engine_variant.score_limit) {\n'
     '        game_engine_begin_end_game_sequence();\n    }\n',
     extra_inc='#include <string.h>\n')

emit(0x46f610, 469, 'game_engine_slayer_build_message_text',
     'the kill-feed text override (called by chimera__kill_feed as (recipient, type, subject, text, count)). Type 0x16 for a live recipient: with teams string 0xb5 with the recipient\'s place text (0x4633f0 of game_engine_compare_score_to_others(recipient, 1)), its own score, its team\'s score and the score limit; without, string 0xb6 with the place, its team slot\'s score and the limit. Type 0x20 for an existing subject: string 0xb4 with the subject\'s name (+0x04). Returns whether it built a text.',
     ['player_data', 'variant', 'teams', 'slayer_scores', 'message_text'],
     'uint8_t %s(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count)',
     '    if (message_type == 0x16) {\n        uint8_t *player = player_if_valid(recipient);\n        const uint16_t *place;\n        int32_t team;\n\n'
     '        if (player == 0) {\n            return 0;\n        }\n'
     '        place = (const uint16_t *)game_engine_get_multiplayer_text_list(game_engine_compare_score_to_others(recipient, 1));\n'
     '        team = *(int32_t *)(%s + 0x20);\n' % P('recipient') +
     '        if (game_engine_teams_enabled_flag != 0) {\n'
     '            string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb5), place, slayer_player_score[recipient & 0xffff],\n'
     '                slayer_team_score[team], game_engine_variant.score_limit);\n'
     '        } else {\n'
     '            string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb6), place, slayer_team_score[team],\n'
     '                game_engine_variant.score_limit);\n        }\n        return 1;\n    }\n'
     '    if (message_type == 0x20) {\n        uint8_t *player = (uint8_t *)datum_get(subject, player_data);\n\n'
     '        if (player == 0) {\n            return 0;\n        }\n'
     '        string_format_wide_va_bounded(count, (uint16_t *)text, game_text(0xb4), player + 4);\n        return 1;\n    }\n'
     '    return 0;\n',
     helper=MT_HELP)
print('ok')
