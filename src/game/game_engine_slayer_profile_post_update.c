// game_engine_slayer_profile_post_update  (not a Ghidra function; the slayer game engine definition's +0x94 slot (profile_post_update); no C existed, so that
//   stored pointer trapped as unlisted_46fb20)
// address 0x46fb20, size 333 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46fb20..0x46fc6c: the profile_post_update decoder (called as (context, ECX) by
//   0x466e60): a baseline message decodes the replicated copy with message_delta_decode_compound_field; an
//   incremental one reads the changed subfields into the live team and player scores (0x6b13d8, 0x20 dwords) and
//   copies them to the replicated copy; on a change they come back from it. The binary then, when the dword at
//   0x6f1d40 is positive, walks the players narrowing each name to ASCII in a stack buffer that nothing reads (a
//   debug leftover with no effect), which is left out here.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "networking.h"
#include <string.h>

extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE
extern uint8_t message_delta_decode_compound_field(void **context, void *destination); // 0x4ec590, blam-cc: EAX context, ECX destination
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
    int32_t changed_offset, int32_t destination_offset); // 0x4ed1d0, blam-cc: EDI state

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

void game_engine_slayer_profile_post_update(void **context)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    uint8_t changed;

    if (state->incremental == 0) {
        changed = message_delta_decode_compound_field(context, slayer_unknown_0087a4a0);
    } else {
        changed = read_changed(context, slayer_unknown_0087a4a0, slayer_team_score);
        memcpy(slayer_unknown_0087a4a0, slayer_team_score, 0x20 * 4);
    }
    if (changed == 1) {
        memcpy(slayer_team_score, slayer_unknown_0087a4a0, 0x20 * 4);
    }
}
