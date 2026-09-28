exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())

NET_INC = '#include "networking.h"\n#include <string.h>\n'
EXT.update({
    'decode': ('extern uint8_t message_delta_decode_compound_field(void **context, void *destination); // 0x4ec590, blam-cc: EAX context, ECX destination\n'
               'extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,\n'
               '    int32_t changed_offset, int32_t destination_offset); // 0x4ed1d0, blam-cc: EDI state'),
    'king_net': ('extern int32_t king_team_hill_seconds_network[16]; // 0x0087a7e0 (the replicated king globals, 0x6b dwords)\n'
                 'extern int32_t king_hill_broadcast_overrun_value; // 0x0087a984 (replicated king_starting_location_type)\n'
                 'extern int32_t king_starting_location_type; // 0x006b1064\n'
                 'extern void game_engine_koth_build_hill_boundary(void); // 0x46a240'),
    'oddball_net': ('extern int32_t king_alt_score_target; // 0x006b1148 (the oddball globals, 0x51 dwords)\n'
                    'extern int32_t king_alt_team_scores_network[16]; // 0x0087a680 (their replicated copy)\n'
                    'extern int32_t king_alt_team_scores_network2[16]; // 0x0087a684\n'
                    'extern int32_t king_alt_player_scores_network[16]; // 0x0087a6c4\n'
                    'extern int32_t king_alt_scores_network_tail[16]; // 0x0087a744'),
    'race_net': ('extern uint8_t ctf_globals_live_bytes[]; // 0x006b1290 (ctf_globals as the race engine uses it)\n'
                 'extern uint8_t ctf_globals_network_bytes[]; // 0x0087a520 (its replicated copy)'),
    'ctf_net': ('extern int32_t ctf_touch_counts_network[3]; // 0x0087a9e0 (team 0 / team 1 touch counts, active team)\n'
                'extern uint8_t ctf_active_team; // 0x006b0eb8\n'
                'extern int32_t ctf_flag_auto_return_ticks; // 0x006b0eb0\n'
                'extern uint8_t custom_waypoints[]; // 0x006f1888'),
})

SEEK = '''// The inline tail every decoder shares with message_delta_decode_compound_field: nothing changed, so the stream
//   cursor moves past this message's bits when the target is inside the stream.
static void skip_unchanged_message(message_delta_decode_state *state)
{
    bit_stream *stream = state->stream;
    int32_t delta = state->unknown_14;
    uint32_t target = (uint32_t)stream->first_bit + (uint32_t)delta;

    if ((delta >= 0 || target <= stream->first_bit) &&
        (delta <= 0 || stream->first_bit <= target) &&
        ((stream->first_bit <= target && target <= stream->last_bit) || target == stream->last_bit + 1)) {
        stream->bit_cursor = target & 7;
        stream->byte_cursor = target >> 3;
    }
}

// message_delta_read_changed_subfields plus the bookkeeping around it; returns whether anything changed.
static uint8_t read_changed(void **context, void *changed_base, void *destination)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    int32_t bits = message_delta_read_changed_subfields(state, (uint8_t *)(context + 1), (int32_t)changed_base, (int32_t)destination);

    state->bits_read += bits;
    if (bits != 0) {
        state->changed = 1;
        return 1;
    }
    skip_unchanged_message(state);
    return 0;
}
'''
BASE = 'the profile_post_update decoder (called as (context, ECX) by 0x466e60): a baseline message decodes the replicated copy with message_delta_decode_compound_field; an incremental one '

emit(0x46b920, 283, 'game_engine_king_profile_post_update',
     BASE + 'restores the live bucket ticks and hill location from the replicated copy, reads the changed subfields into them and copies the ticks back. On a change the live ticks become the replicated seconds * 30 and the location the replicated one; a moved hill (incremental only) rebuilds the boundary.',
     ['king_bucket_credit_ticks', 'decode', 'king_net'], 'void %s(void **context)',
     '    message_delta_decode_state *state = (message_delta_decode_state *)context[0];\n    uint8_t changed;\n    uint8_t moved;\n    int32_t i;\n\n'
     '    if (state->incremental == 0) {\n        changed = message_delta_decode_compound_field(context, king_team_hill_seconds_network);\n        moved = 1;\n'
     '    } else {\n        int32_t previous = king_hill_broadcast_overrun_value;\n\n'
     '        memcpy(king_bucket_credit_ticks, king_team_hill_seconds_network, 16 * 4);\n        king_starting_location_type = previous;\n'
     '        changed = read_changed(context, king_team_hill_seconds_network, king_bucket_credit_ticks);\n'
     '        memcpy(king_team_hill_seconds_network, king_bucket_credit_ticks, 16 * 4);\n'
     '        moved = king_starting_location_type != previous;\n        king_hill_broadcast_overrun_value = king_starting_location_type;\n    }\n'
     '    if (changed != 1) {\n        return;\n    }\n'
     '    memcpy(king_bucket_credit_ticks, king_team_hill_seconds_network, 16 * 4);\n    king_starting_location_type = king_hill_broadcast_overrun_value;\n'
     '    for (i = 0; i < 16; i++) {\n        king_bucket_credit_ticks[i] = king_team_hill_seconds_network[i] * 30;\n    }\n'
     '    if (moved == 1) {\n        game_engine_koth_build_hill_boundary();\n    }\n',
     extra_inc=NET_INC, helper=SEEK)

emit(0x46d1d0, 312, 'game_engine_oddball_profile_post_update',
     BASE + 'restores the 0x51 live dwords from 0x6b1148 out of the replicated copy, reads the changed subfields into them and copies the player scores, team scores and occupant table back. On a change those three blocks come from the replicated copy and, unless variant +0x8c is 2, the team and player scores are seconds and become ticks (* 30).',
     ['variant', 'king_alt_team_score', 'king_alt_player_score', 'king_hill_occupant_table', 'decode', 'oddball_net'], 'void %s(void **context)',
     '    message_delta_decode_state *state = (message_delta_decode_state *)context[0];\n    uint8_t changed;\n    int32_t i;\n\n'
     '    if (state->incremental == 0) {\n        changed = message_delta_decode_compound_field(context, king_alt_team_scores_network);\n'
     '    } else {\n        memcpy(&king_alt_score_target, king_alt_team_scores_network, 0x51 * 4);\n'
     '        changed = read_changed(context, king_alt_team_scores_network, &king_alt_score_target);\n'
     '        memcpy(king_alt_player_scores_network, king_alt_player_score, 16 * 4);\n'
     '        memcpy(king_alt_team_scores_network2, king_alt_team_score, 16 * 4);\n'
     '        memcpy(king_alt_scores_network_tail, king_hill_occupant_table, 16 * 4);\n    }\n'
     '    if (changed != 1) {\n        return;\n    }\n'
     '    memcpy(king_alt_player_score, king_alt_player_scores_network, 16 * 4);\n'
     '    memcpy(king_alt_team_score, king_alt_team_scores_network2, 16 * 4);\n'
     '    memcpy(king_hill_occupant_table, king_alt_scores_network_tail, 16 * 4);\n'
     '    if (game_engine_variant.unknown_8c == 2) {\n        return;\n    }\n'
     '    for (i = 0; i < 16; i++) {\n        king_alt_team_score[i] *= 30;\n        king_alt_player_score[i] *= 30;\n    }\n',
     extra_inc=NET_INC, helper=SEEK)

emit(0x46ed30, 297, 'game_engine_race_profile_post_update',
     BASE + 'reads the changed subfields straight into the live ctf globals (0x6b1290) and copies the bucket scores (+0x88), the 16 dwords at +0x04, the 16 at +0x44, the mask (+0x00) and the neutral flag id (+0x84) to the replicated copy; on a change the same fields come back from it.',
     ['decode', 'race_net'], 'void %s(void **context)',
     '    message_delta_decode_state *state = (message_delta_decode_state *)context[0];\n    uint8_t changed;\n\n'
     '    if (state->incremental == 0) {\n        changed = message_delta_decode_compound_field(context, ctf_globals_network_bytes);\n'
     '    } else {\n        changed = read_changed(context, ctf_globals_network_bytes, ctf_globals_live_bytes);\n'
     '        memcpy(ctf_globals_network_bytes + 0x88, ctf_globals_live_bytes + 0x88, 16 * 4);\n'
     '        memcpy(ctf_globals_network_bytes + 0x04, ctf_globals_live_bytes + 0x04, 16 * 4);\n'
     '        memcpy(ctf_globals_network_bytes + 0x44, ctf_globals_live_bytes + 0x44, 16 * 4);\n'
     '        *(uint32_t *)ctf_globals_network_bytes = *(uint32_t *)ctf_globals_live_bytes;\n'
     '        *(int32_t *)(ctf_globals_network_bytes + 0x84) = *(int32_t *)(ctf_globals_live_bytes + 0x84);\n    }\n'
     '    if (changed != 1) {\n        return;\n    }\n'
     '    memcpy(ctf_globals_live_bytes + 0x88, ctf_globals_network_bytes + 0x88, 16 * 4);\n'
     '    memcpy(ctf_globals_live_bytes + 0x04, ctf_globals_network_bytes + 0x04, 16 * 4);\n'
     '    memcpy(ctf_globals_live_bytes + 0x44, ctf_globals_network_bytes + 0x44, 16 * 4);\n'
     '    *(uint32_t *)ctf_globals_live_bytes = *(uint32_t *)ctf_globals_network_bytes;\n'
     '    *(int32_t *)(ctf_globals_live_bytes + 0x84) = *(int32_t *)(ctf_globals_network_bytes + 0x84);\n',
     extra_inc=NET_INC, helper=SEEK)

emit(0x46fb20, 333, 'game_engine_slayer_profile_post_update',
     BASE + 'reads the changed subfields into the live team and player scores (0x6b13d8, 0x20 dwords) and copies them to the replicated copy; on a change they come back from it. The binary then, when the dword at 0x6f1d40 is positive, walks the players narrowing each name to ASCII in a stack buffer that nothing reads (a debug leftover with no effect), which is left out here.',
     ['slayer_scores', 'decode'], 'void %s(void **context)',
     '    message_delta_decode_state *state = (message_delta_decode_state *)context[0];\n    uint8_t changed;\n\n'
     '    if (state->incremental == 0) {\n        changed = message_delta_decode_compound_field(context, slayer_unknown_0087a4a0);\n'
     '    } else {\n        changed = read_changed(context, slayer_unknown_0087a4a0, slayer_team_score);\n'
     '        memcpy(slayer_unknown_0087a4a0, slayer_team_score, 0x20 * 4);\n    }\n'
     '    if (changed == 1) {\n        memcpy(slayer_team_score, slayer_unknown_0087a4a0, 0x20 * 4);\n    }\n',
     extra_inc=NET_INC, helper=SEEK)

emit(0x469d10, 503, 'game_engine_ctf_profile_post_update',
     BASE + 'reads the changed subfields into a stack copy of the three replicated dwords at 0x87a9e0 (team touch counts, active team) and stores it back. On a change: with variant +0x80 positive and a new active team the first 0x80 bytes of custom waypoints are cleared; the touch counts and active team become the replicated ones and the flag auto-return ticks come from the dword that context slot 0x11 points to.',
     ['variant', 'ctf_team_flag_touch_count', 'decode', 'ctf_net'], 'void %s(void **context)',
     '    message_delta_decode_state *state = (message_delta_decode_state *)context[0];\n    uint8_t changed;\n    uint8_t team;\n\n'
     '    if (state->incremental == 0) {\n        ctf_touch_counts_network[0] = 0;\n        ctf_touch_counts_network[1] = 0;\n        ctf_touch_counts_network[2] = 0;\n'
     '        changed = message_delta_decode_compound_field(context, ctf_touch_counts_network);\n'
     '    } else {\n        int32_t local[3];\n\n        local[0] = ctf_touch_counts_network[0];\n        local[1] = ctf_touch_counts_network[1];\n        local[2] = ctf_touch_counts_network[2];\n'
     '        changed = read_changed(context, ctf_touch_counts_network, local);\n'
     '        ctf_touch_counts_network[0] = local[0];\n        ctf_touch_counts_network[1] = local[1];\n        ctf_touch_counts_network[2] = local[2];\n    }\n'
     '    team = (uint8_t)ctf_touch_counts_network[2];\n'
     '    if (changed != 1) {\n        return;\n    }\n'
     '    if (game_engine_variant.ctf_value_80 > 0 && ctf_active_team != team) {\n        memset(custom_waypoints, 0, 0x80);\n    }\n'
     '    ctf_team_flag_touch_count[0] = ctf_touch_counts_network[0];\n    ctf_team_flag_touch_count[1] = ctf_touch_counts_network[1];\n'
     '    ctf_active_team = team;\n    ctf_flag_auto_return_ticks = *(int32_t *)context[0x11];\n',
     extra_inc=NET_INC, helper=SEEK)
print('ok')
