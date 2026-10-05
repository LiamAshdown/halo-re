/**
 * @file src/networking/net2_message_delta_scalar.cpp
 * Scalar and flag message-delta field codecs.
 */
#include "message_delta_codec.h"
#include "halo/networking/net2_message_delta_scalar.hpp"
#include "halo/memory/api.hpp"
#include "halo/networking/api.hpp"



namespace halo::networking {

int32_t ScalarFieldCodec::boolean_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)field_type;
    (void)previous;
    *(uint8_t *)current = 0;
    return (int32_t)halo::memory::bit_stream_read_bit((uint8_t *)current, stream);
}

int32_t ScalarFieldCodec::boolean_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint8_t value = *(uint8_t *)current;

    (void)field_type;
    if (previous != 0 && *(uint8_t *)previous == value) {
        return 0;
    }
    return halo::memory::bit_stream_write_bit(value, stream) ? 1 : 0;
}

int32_t ScalarFieldCodec::byte_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)field_type;
    (void)previous;
    return halo::memory::bit_stream_read_bits_chunked(8, (uint32_t *)current, stream);
}

int32_t ScalarFieldCodec::byte_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)field_type;
    if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
        return 0;
    }
    return halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 8);
}

int32_t ScalarFieldCodec::compute_size_1(message_delta_field_type *field_type)
{
    (void)field_type;
    return 1;
}

int32_t ScalarFieldCodec::compute_size_16(message_delta_field_type *field_type)
{
    (void)field_type;
    return 16;
}

int32_t ScalarFieldCodec::compute_size_2(message_delta_field_type *field_type)
{
    (void)field_type;
    return 2;
}

int32_t ScalarFieldCodec::compute_size_3(message_delta_field_type *field_type)
{
    (void)field_type;
    return 3;
}

int32_t ScalarFieldCodec::compute_size_32(message_delta_field_type *field_type)
{
    (void)field_type;
    return 32;
}

int32_t ScalarFieldCodec::compute_size_4(message_delta_field_type *field_type)
{
    (void)field_type;
    return 4;
}

int32_t ScalarFieldCodec::compute_size_6(message_delta_field_type *field_type)
{
    (void)field_type;
    return 6;
}

int32_t ScalarFieldCodec::compute_size_8(message_delta_field_type *field_type)
{
    (void)field_type;
    return 8;
}

int32_t ScalarFieldCodec::flags_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint8_t *mask = (uint8_t *)(descriptor + 1);
    uint32_t value = *(uint32_t *)current;
    int32_t total = 0;
    int32_t i;

    (void)previous;
    for (i = 0; i < descriptor[0]; i++) {
        if (mask[i] == 1) {
            uint8_t bit = 0;

            total += (int32_t)halo::memory::bit_stream_read_bit(&bit, stream);
            if (bit) {
                value |= 1u << i;
            } else {
                value &= ~(1u << i);
            }
        }
    }
    *(uint32_t *)current = value;
    return total;
}

int32_t ScalarFieldCodec::flags_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint8_t *mask = (uint8_t *)(descriptor + 1);
    uint32_t value = *(uint32_t *)current;
    int32_t start = (int32_t)(message_delta_stream_position(stream) - stream->first_bit);
    int32_t total = 0;
    uint8_t changed = 0;
    int32_t i;

    for (i = 0; i < descriptor[0]; i++) {
        if (mask[i] != 1) {
            continue;
        }
        if (previous == 0 || changed ||
            ((*(uint32_t *)previous & (1u << i)) != 0) != ((value & (1u << i)) != 0)) {
            changed = 1;
        }
        total += halo::memory::bit_stream_write_bit((value & (1u << i)) != 0, stream) ? 1 : 0;
    }
    if (changed) {
        return total;
    }
    message_delta_stream_seek(stream, stream->first_bit, start);
    return 0;
}

uint8_t ScalarFieldCodec::flags_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint8_t *mask = (uint8_t *)(descriptor + 1);
    int32_t i;

    if (descriptor[0] <= 0 || descriptor[0] > 0x20) {
        return 0;
    }
    for (i = 0; i < descriptor[0]; i++) {
        if (mask[i] != 1 && mask[i] != 0) {
            return 0;
        }
    }
    return 1;
}

int32_t ScalarFieldCodec::integer_compute_size(message_delta_field_type *field_type)
{
    switch (*(int32_t *)field_type->array_descriptor) {
    case 0:
        return 8;
    case 1:
        return 0x10;
    case 2:
        return 0x20;
    case 3:
        return 1;
    case 4:
        return 3;
    case 5:
        return 5;
    default:
        return 6;
    }
}

int32_t ScalarFieldCodec::integer_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)previous;
    switch (*(int32_t *)field_type->array_descriptor) {
    case 0:
        return halo::memory::bit_stream_read_bits_chunked(8, (uint32_t *)current, stream);
    case 1:
        return halo::memory::bit_stream_read_bits_chunked(16, (uint32_t *)current, stream);
    case 2:
        return halo::memory::bit_stream_read_bits_chunked(32, (uint32_t *)current, stream);
    case 3:
        *(uint8_t *)current = 0;
        return halo::memory::bit_stream_read_bits_chunked(1, (uint32_t *)current, stream);
    case 4:
        *(uint8_t *)current = 0;
        return halo::memory::bit_stream_read_bits_chunked(3, (uint32_t *)current, stream);
    case 5:
        *(uint8_t *)current = 0;
        return halo::memory::bit_stream_read_bits_chunked(5, (uint32_t *)current, stream);
    case 6:
        *(uint8_t *)current = 0;
        return halo::memory::bit_stream_read_bits_chunked(6, (uint32_t *)current, stream);
    }
    return 0;
}

int32_t ScalarFieldCodec::integer_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    // Narrow fields are widened into a local first: the original passed the field address as a dword, which reads
    // past a 16-bit field that ends its message (weapon_magazine_ammo_message is 10 bytes).
    switch (*(int32_t *)field_type->array_descriptor) {
    case 0:
        if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
            return 0;
        }
        { uint32_t value = *(uint8_t *)current; return halo::memory::bit_stream_write_bits_chunked(stream, &value, 8); }
    case 1:
        if (previous != 0 && *(uint16_t *)previous == *(uint16_t *)current) {
            return 0;
        }
        { uint32_t value = *(uint16_t *)current; return halo::memory::bit_stream_write_bits_chunked(stream, &value, 16); }
    case 2:
        if (previous != 0 && *(uint32_t *)previous == *(uint32_t *)current) {
            return 0;
        }
        return halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 32);
    case 3:
        if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
            return 0;
        }
        { uint32_t value = *(uint8_t *)current; return halo::memory::bit_stream_write_bits_chunked(stream, &value, 1); }
    case 4:
        if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
            return 0;
        }
        { uint32_t value = *(uint8_t *)current; return halo::memory::bit_stream_write_bits_chunked(stream, &value, 3); }
    case 5:
        if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
            return 0;
        }
        { uint32_t value = *(uint8_t *)current; return halo::memory::bit_stream_write_bits_chunked(stream, &value, 5); }
    case 6:
        if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
            return 0;
        }
        { uint32_t value = *(uint8_t *)current; return halo::memory::bit_stream_write_bits_chunked(stream, &value, 6); }
    }
    return 0;
}

uint8_t ScalarFieldCodec::integer_initialize(message_delta_field_type *field_type)
{
    if (field_type->array_descriptor == 0) {
        return 0;
    }
    int32_t subtype = *(int32_t *)field_type->array_descriptor;

    return subtype >= 0 && subtype < 0x1c;
}

int32_t ScalarFieldCodec::long_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)field_type;
    (void)previous;
    return halo::memory::bit_stream_read_bits_chunked(0x20, (uint32_t *)current, stream);
}

int32_t ScalarFieldCodec::long_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)field_type;
    if (previous != 0 && *(uint32_t *)previous == *(uint32_t *)current) {
        return 0;
    }
    return halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 0x20);
}

}  // namespace halo::networking

namespace halo::networking {
int32_t message_delta_boolean_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::ScalarFieldCodec::boolean_decode(field_type, previous, current, stream);
}

int32_t message_delta_boolean_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::ScalarFieldCodec::boolean_encode(field_type, previous, current, stream);
}

int32_t message_delta_byte_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::ScalarFieldCodec::byte_decode(field_type, previous, current, stream);
}

int32_t message_delta_byte_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::ScalarFieldCodec::byte_encode(field_type, previous, current, stream);
}

int32_t message_delta_compute_size_1(message_delta_field_type *field_type)
{
    return halo::networking::ScalarFieldCodec::compute_size_1(field_type);
}

int32_t message_delta_compute_size_16(message_delta_field_type *field_type)
{
    return halo::networking::ScalarFieldCodec::compute_size_16(field_type);
}

int32_t message_delta_compute_size_2(message_delta_field_type *field_type)
{
    return halo::networking::ScalarFieldCodec::compute_size_2(field_type);
}

int32_t message_delta_compute_size_3(message_delta_field_type *field_type)
{
    return halo::networking::ScalarFieldCodec::compute_size_3(field_type);
}

int32_t message_delta_compute_size_32(message_delta_field_type *field_type)
{
    return halo::networking::ScalarFieldCodec::compute_size_32(field_type);
}

int32_t message_delta_compute_size_4(message_delta_field_type *field_type)
{
    return halo::networking::ScalarFieldCodec::compute_size_4(field_type);
}

int32_t message_delta_compute_size_6(message_delta_field_type *field_type)
{
    return halo::networking::ScalarFieldCodec::compute_size_6(field_type);
}

int32_t message_delta_compute_size_8(message_delta_field_type *field_type)
{
    return halo::networking::ScalarFieldCodec::compute_size_8(field_type);
}

int32_t message_delta_flags_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::ScalarFieldCodec::flags_decode(field_type, previous, current, stream);
}

int32_t message_delta_flags_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::ScalarFieldCodec::flags_encode(field_type, previous, current, stream);
}

uint8_t message_delta_flags_initialize(message_delta_field_type *field_type)
{
    return halo::networking::ScalarFieldCodec::flags_initialize(field_type);
}

int32_t message_delta_integer_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::ScalarFieldCodec::integer_compute_size(field_type);
}

int32_t message_delta_integer_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::ScalarFieldCodec::integer_decode(field_type, previous, current, stream);
}

int32_t message_delta_integer_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::ScalarFieldCodec::integer_encode(field_type, previous, current, stream);
}

uint8_t message_delta_integer_initialize(message_delta_field_type *field_type)
{
    return halo::networking::ScalarFieldCodec::integer_initialize(field_type);
}

int32_t message_delta_long_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::ScalarFieldCodec::long_decode(field_type, previous, current, stream);
}

int32_t message_delta_long_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::ScalarFieldCodec::long_encode(field_type, previous, current, stream);
}

}
