// game_engine_ctf_profile_post_update  (not a Ghidra function; the ctf game engine definition's +0x94 slot (profile_post_update); no C existed, so that
//   stored pointer trapped as unlisted_469d10)
// address 0x469d10, size 503 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469d10..0x469f06: the profile_post_update decoder (called as (context, ECX) by
//   0x466e60): a baseline message decodes the replicated copy with message_delta_decode_compound_field; an
//   incremental one reads the changed subfields into a stack copy of the three replicated dwords at 0x87a9e0 (team
//   touch counts, active team) and stores it back. On a change: with variant +0x80 positive and a new active team the
//   first 0x80 bytes of custom waypoints are cleared; the touch counts and active team become the replicated ones and
//   the flag auto-return ticks come from the dword that context slot 0x11 points to.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "networking.h"
#include <string.h>

extern game_variant game_engine_variant; // 0x006f1c88
extern int32_t ctf_team_flag_touch_count[2]; // 0x006b0e98
extern uint8_t message_delta_decode_compound_field(void **context, void *destination); // 0x4ec590, blam-cc: EAX context, ECX destination
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
    int32_t changed_offset, int32_t destination_offset); // 0x4ed1d0, blam-cc: EDI state
extern int32_t ctf_touch_counts_network[3]; // 0x0087a9e0 (team 0 / team 1 touch counts, active team)
extern uint8_t ctf_active_team; // 0x006b0eb8
extern int32_t ctf_flag_auto_return_ticks; // 0x006b0eb0
extern uint8_t custom_waypoints[]; // 0x006f1888

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

void game_engine_ctf_profile_post_update(void **context)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    uint8_t changed;
    uint8_t team;

    if (state->incremental == 0) {
        ctf_touch_counts_network[0] = 0;
        ctf_touch_counts_network[1] = 0;
        ctf_touch_counts_network[2] = 0;
        changed = message_delta_decode_compound_field(context, ctf_touch_counts_network);
    } else {
        int32_t local[3];

        local[0] = ctf_touch_counts_network[0];
        local[1] = ctf_touch_counts_network[1];
        local[2] = ctf_touch_counts_network[2];
        changed = read_changed(context, ctf_touch_counts_network, local);
        ctf_touch_counts_network[0] = local[0];
        ctf_touch_counts_network[1] = local[1];
        ctf_touch_counts_network[2] = local[2];
    }
    team = (uint8_t)ctf_touch_counts_network[2];
    if (changed != 1) {
        return;
    }
    if (game_engine_variant.engine.ctf.single_flag_time > 0 && ctf_active_team != team) {
        memset(custom_waypoints, 0, 0x80);
    }
    ctf_team_flag_touch_count[0] = ctf_touch_counts_network[0];
    ctf_team_flag_touch_count[1] = ctf_touch_counts_network[1];
    ctf_active_team = team;
    ctf_flag_auto_return_ticks = *(int32_t *)context[0x11];
}
