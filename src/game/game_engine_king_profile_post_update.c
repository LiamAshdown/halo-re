// game_engine_king_profile_post_update  (not a Ghidra function; the king game engine definition's +0x94 slot (profile_post_update); no C existed, so that
//   stored pointer trapped as unlisted_46b920)
// address 0x46b920, size 283 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46b920..0x46ba3a: the profile_post_update decoder (called as (context, ECX) by
//   0x466e60): a baseline message decodes the replicated copy with message_delta_decode_compound_field; an
//   incremental one restores the live bucket ticks and hill location from the replicated copy, reads the changed
//   subfields into them and copies the ticks back. On a change the live ticks become the replicated seconds * 30 and
//   the location the replicated one; a moved hill (incremental only) rebuilds the boundary.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "networking.h"
#include <string.h>

extern int32_t king_bucket_credit_ticks[16]; // 0x006b0ec0
extern uint8_t message_delta_decode_compound_field(void **context, void *destination); // 0x4ec590, blam-cc: EAX context, ECX destination
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
    int32_t changed_offset, int32_t destination_offset); // 0x4ed1d0, blam-cc: EDI state
extern int32_t king_team_hill_seconds_network[16]; // 0x0087a7e0 (the replicated king globals, 0x6b dwords)
extern int32_t king_hill_broadcast_overrun_value; // 0x0087a984 (replicated king_starting_location_type)
extern int32_t king_starting_location_type; // 0x006b1064
extern void game_engine_koth_build_hill_boundary(void); // 0x46a240

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

void game_engine_king_profile_post_update(void **context)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    uint8_t changed;
    uint8_t moved;
    int32_t i;

    if (state->incremental == 0) {
        changed = message_delta_decode_compound_field(context, king_team_hill_seconds_network);
        moved = 1;
    } else {
        int32_t previous = king_hill_broadcast_overrun_value;

        memcpy(king_bucket_credit_ticks, king_team_hill_seconds_network, 16 * 4);
        king_starting_location_type = previous;
        changed = read_changed(context, king_team_hill_seconds_network, king_bucket_credit_ticks);
        memcpy(king_team_hill_seconds_network, king_bucket_credit_ticks, 16 * 4);
        moved = king_starting_location_type != previous;
        king_hill_broadcast_overrun_value = king_starting_location_type;
    }
    if (changed != 1) {
        return;
    }
    memcpy(king_bucket_credit_ticks, king_team_hill_seconds_network, 16 * 4);
    king_starting_location_type = king_hill_broadcast_overrun_value;
    for (i = 0; i < 16; i++) {
        king_bucket_credit_ticks[i] = king_team_hill_seconds_network[i] * 30;
    }
    if (moved == 1) {
        game_engine_koth_build_hill_boundary();
    }
}
