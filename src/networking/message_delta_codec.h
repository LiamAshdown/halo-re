// message_delta_codec.h -- shared declarations for the message-delta field-type codecs (0x4e89c0..0x4ebc20).
// Every codec is reached only through .data: the per-kind table at 0x69a2f0 (compute_size, initialize,
// teardown) or a field type's own encode (+0x50) / decode (+0x54) pointers. All take
// (field_type, previous, current, stream) -- previous NULL means "send everything" -- and return the bits used.
#ifndef HALO_MESSAGE_DELTA_CODEC_H
#define HALO_MESSAGE_DELTA_CODEC_H
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include <string.h>
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"
#ifdef __cplusplus
#endif

inline auto &message_delta_field_type_table = halo::link::ref<message_delta_field_type_vtable [28]>(halo::networking::vars().message_delta_field_type_table);
inline auto &message_delta_item_count_bits = halo::link::ref<uint8_t []>(halo::networking::vars().message_delta_item_count_bits);

 // 0x4cf8f0
      // 0x4cf950
 // 0x4cf9a0
  // 0x4cfb80

typedef int32_t (*message_delta_codec_proc)(message_delta_field_type *field_type, void *previous, void *current,
    bit_stream *stream);
typedef uint8_t (*message_delta_initialize_proc)(message_delta_field_type *field_type);

// The inlined absolute seek every array codec uses: move the cursor to base + delta when that neither wraps nor
// leaves [first_bit, last_bit + 1].
static __inline void message_delta_stream_seek(bit_stream *stream, uint32_t base, int32_t delta)
{
    uint32_t target = base + (uint32_t)delta;

    if (delta < 0 ? target > base : (delta > 0 && target < base)) {
        return;
    }
    if ((target >= stream->first_bit && target <= stream->last_bit) || target == stream->last_bit + 1) {
        stream->bit_cursor = target & 7;
        stream->byte_cursor = target >> 3;
    }
}

static __inline uint32_t message_delta_stream_position(const bit_stream *stream)
{
    return stream->bit_cursor + stream->byte_cursor * 8;
}

#ifdef __cplusplus
#endif
#endif
