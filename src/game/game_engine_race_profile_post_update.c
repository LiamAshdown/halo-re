// game_engine_race_profile_post_update  (not a Ghidra function; the race game engine definition's +0x94 slot (profile_post_update); no C existed, so that
//   stored pointer trapped as unlisted_46ed30)
// address 0x46ed30, size 297 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46ed30..0x46ee58: the profile_post_update decoder (called as (context, ECX) by
//   0x466e60): a baseline message decodes the replicated copy with message_delta_decode_compound_field; an
//   incremental one reads the changed subfields straight into the live ctf globals (0x6b1290) and copies the bucket
//   scores (+0x88), the 16 dwords at +0x04, the 16 at +0x44, the mask (+0x00) and the neutral flag id (+0x84) to the
//   replicated copy; on a change the same fields come back from it.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "networking.h"
#include <string.h>

extern uint8_t message_delta_decode_compound_field(void **context, void *destination); // 0x4ec590, blam-cc: EAX context, ECX destination
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
    int32_t changed_offset, int32_t destination_offset); // 0x4ed1d0, blam-cc: EDI state
extern uint8_t ctf_globals_live[]; // 0x006b1290 (ctf_globals as the race engine uses it)
extern uint8_t ctf_globals_network[]; // 0x0087a520 (its replicated copy)

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

void game_engine_race_profile_post_update(void **context)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];
    uint8_t changed;

    if (state->incremental == 0) {
        changed = message_delta_decode_compound_field(context, ctf_globals_network);
    } else {
        changed = read_changed(context, ctf_globals_network, ctf_globals_live);
        memcpy(ctf_globals_network + 0x88, ctf_globals_live + 0x88, 16 * 4);
        memcpy(ctf_globals_network + 0x04, ctf_globals_live + 0x04, 16 * 4);
        memcpy(ctf_globals_network + 0x44, ctf_globals_live + 0x44, 16 * 4);
        *(uint32_t *)ctf_globals_network = *(uint32_t *)ctf_globals_live;
        *(int32_t *)(ctf_globals_network + 0x84) = *(int32_t *)(ctf_globals_live + 0x84);
    }
    if (changed != 1) {
        return;
    }
    memcpy(ctf_globals_live + 0x88, ctf_globals_network + 0x88, 16 * 4);
    memcpy(ctf_globals_live + 0x04, ctf_globals_network + 0x04, 16 * 4);
    memcpy(ctf_globals_live + 0x44, ctf_globals_network + 0x44, 16 * 4);
    *(uint32_t *)ctf_globals_live = *(uint32_t *)ctf_globals_network;
    *(int32_t *)(ctf_globals_live + 0x84) = *(int32_t *)(ctf_globals_network + 0x84);
}
