/**
 * @file include/halo/networking/message_delta_context.hpp
 * The 0x98 byte encode context the message-delta encoder keeps on its stack while it writes one message.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Encode state of one message. `stream` receives the message header and item count, `baseline_stream` and
 * `item_stream` describe where the static fields and the current item's fields land; the three bit counters
 * accumulate the size of the current item.
 */
struct message_delta_encode_context {
    uint8_t active;                 // 0x00 set once the header has been written
    uint8_t unknown_01[3];
    int32_t message_type;           // 0x04 index into message_delta_definitions; written to the stream as the type id
    int32_t mode;                   // 0x08 1 when encoding against a baseline
    uint8_t *buffer;                // 0x0c output buffer; the three streams start here
    int32_t buffer_bits;            // 0x10 capacity of buffer in bits
    int32_t item_bits;              // 0x14 bits used by the items written so far
    int32_t remaining_bits;         // 0x18 bits still available
    bit_stream stream;              // 0x1c
    int32_t header_bits;            // 0x34 header size in bits
    int32_t item_count;             // 0x38 items written so far
    int32_t bit_offset;             // 0x3c where the current item starts
    int32_t static_bits;            // 0x40 bits used by the current item's static fields
    int32_t field_bits;             // 0x44 bits used by the current item's fields
    bit_stream baseline_stream;     // 0x48
    int32_t baseline_bits;          // 0x60 size of the baseline stream in bits
    bit_stream item_stream;         // 0x64
    int32_t item_stream_bits;       // 0x7c bits left for the item stream
    int32_t header_written;         // 0x80 set to 1 by the message header writer
    int32_t type_bits;              // 0x84 bits of message_type written (6 plus the header counter)
    int32_t parameter_bits;         // 0x88 bits of the parameter / item count field
    int32_t unknown_8c[3];          // 0x8c never touched
};

static_assert(sizeof(message_delta_encode_context) == 0x98, "message_delta_encode_context size");
static_assert(offsetof(message_delta_encode_context, stream) == 0x1c, "message_delta_encode_context::stream");
static_assert(offsetof(message_delta_encode_context, header_bits) == 0x34, "message_delta_encode_context::header_bits");
static_assert(offsetof(message_delta_encode_context, baseline_stream) == 0x48, "message_delta_encode_context::baseline_stream");
static_assert(offsetof(message_delta_encode_context, item_stream) == 0x64, "message_delta_encode_context::item_stream");
static_assert(offsetof(message_delta_encode_context, parameter_bits) == 0x88, "message_delta_encode_context::parameter_bits");
