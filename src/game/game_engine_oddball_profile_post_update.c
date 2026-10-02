// game_engine_oddball_profile_post_update  (not a Ghidra function; the oddball game engine definition's +0x94 slot (profile_post_update); no C existed, so that
//   stored pointer trapped as unlisted_46d1d0)
// address 0x46d1d0, size 312 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46d1d0..0x46d307: the profile_post_update decoder (called as (context, ECX) by
//   0x466e60): a baseline message decodes the replicated copy with message_delta_decode_compound_field; an
//   incremental one restores the 0x51 live dwords from 0x6b1148 out of the replicated copy, reads the changed
//   subfields into them and copies the player scores, team scores and occupant table back. On a change those three
//   blocks come from the replicated copy and, unless variant +0x8c is 2, the team and player scores are seconds and
//   become ticks (* 30).
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "networking.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_variant game_engine_variant; // 0x006f1c88
extern int32_t king_alt_team_score[16]; // 0x006b114c (oddball team score)
extern int32_t king_alt_player_score[]; // 0x006b118c (oddball player score)
extern uint32_t king_hill_occupant_table[16]; // 0x006b120c
extern uint8_t message_delta_decode_compound_field(void **context, void *destination); // 0x4ec590, blam-cc: EAX context, ECX destination
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
    int32_t changed_offset, int32_t destination_offset); // 0x4ed1d0, blam-cc: EDI state
extern int32_t king_alt_score_target; // 0x006b1148 (the oddball globals, 0x51 dwords)
extern int32_t king_alt_team_scores_network[16]; // 0x0087a680 (their replicated copy)
extern int32_t king_alt_team_scores_network2[16]; // 0x0087a684
extern int32_t king_alt_player_scores_network[16]; // 0x0087a6c4
extern int32_t king_alt_scores_network_tail[16]; // 0x0087a744

// The inline tail every decoder shares with message_delta_decode_compound_field: nothing changed, so the stream
//   cursor moves past this message's bits when the target is inside the stream.
static void skip_unchanged_message(message_delta_decode_state *state)
{
    bit_stream *stream = (bit_stream *)state->stream;
    int32_t delta = state->start_bit_offset;
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

void game_engine_oddball_profile_post_update(void **context)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    uint8_t changed;
    int32_t i;

    if (state->incremental == 0) {
        changed = message_delta_decode_compound_field(context, king_alt_team_scores_network);
    } else {
        memcpy(&king_alt_score_target, king_alt_team_scores_network, 0x51 * 4);
        changed = read_changed(context, king_alt_team_scores_network, &king_alt_score_target);
        memcpy(king_alt_player_scores_network, king_alt_player_score, 16 * 4);
        memcpy(king_alt_team_scores_network2, king_alt_team_score, 16 * 4);
        memcpy(king_alt_scores_network_tail, king_hill_occupant_table, 16 * 4);
    }
    if (changed != 1) {
        return;
    }
    memcpy(king_alt_player_score, king_alt_player_scores_network, 16 * 4);
    memcpy(king_alt_team_score, king_alt_team_scores_network2, 16 * 4);
    memcpy(king_hill_occupant_table, king_alt_scores_network_tail, 16 * 4);
    if (game_engine_variant.engine.oddball.ball_type == 2) {
        return;
    }
    for (i = 0; i < 16; i++) {
        king_alt_team_score[i] *= 30;
        king_alt_player_score[i] *= 30;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
