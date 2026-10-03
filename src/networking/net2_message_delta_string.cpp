/**
 * @file src/networking/net2_message_delta_string.cpp
 * String and blob message-delta field codecs.
 */
#include "message_delta_codec.h"
#include <wchar.h>
#include "halo/networking/net2_message_delta_string.hpp"
#include "halo/memory/api.hpp"
#include "halo/networking/api.hpp"

extern "C" {
extern uint8_t message_delta_item_count_bits[];
}


namespace halo::networking {

int32_t StringFieldCodec::blob_compute_size(message_delta_field_type *field_type)
{
    return *(int32_t *)field_type->array_descriptor << 3;
}

int32_t StringFieldCodec::string_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    field_type->reserved_bits = message_delta_item_count_bits[descriptor[0] + 1];
    return field_type->reserved_bits + descriptor[0] * 8;
}

int32_t StringFieldCodec::string_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    char *string = (char *)current;
    int32_t length = 0;
    int32_t bits = halo::memory::bit_stream_read_bits_chunked(field_type->reserved_bits, (uint32_t *)&length, stream);
    int32_t i;

    (void)previous;
    if (length < 0 || length > descriptor[0]) {
        return bits;
    }
    for (i = 0; i < length; i++) {
        bits += halo::memory::bit_stream_read_bits_chunked(8, (uint32_t *)(string + i), stream);
    }
    string[length] = 0;
    return bits;
}

int32_t StringFieldCodec::string_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    const char *string = (const char *)current;
    int32_t length = (int32_t)strlen(string);
    int32_t bits;
    int32_t i;

    if (previous != 0 && strcmp((const char *)previous, string) == 0) {
        return 0;
    }
    bits = halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)&length, field_type->reserved_bits);
    for (i = 0; i < length; i++) {
        bits += halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)(string + i), 8);
    }
    return bits;
}

int32_t StringFieldCodec::wide_string_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    field_type->reserved_bits = message_delta_item_count_bits[descriptor[0] + 1];
    return descriptor[0] * 0x10 + field_type->reserved_bits;
}

int32_t StringFieldCodec::wide_string_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint16_t *string = (uint16_t *)current;
    int32_t length = 0;
    int32_t bits = halo::memory::bit_stream_read_bits_chunked(field_type->reserved_bits, (uint32_t *)&length, stream);
    int32_t i;

    (void)previous;
    if (length < 0 || length > descriptor[0]) {
        return bits;
    }
    for (i = 0; i < length; i++) {
        bits += halo::memory::bit_stream_read_bits_chunked(0x10, (uint32_t *)(string + i), stream);
    }
    string[length] = 0;
    return bits;
}

int32_t StringFieldCodec::wide_string_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    const wchar_t *string = (const wchar_t *)current;
    int32_t length = (int32_t)wcslen(string);
    int32_t bits;
    int32_t i;

    if (previous != 0 && wcscmp((const wchar_t *)previous, string) == 0) {
        return 0;
    }
    bits = halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)&length, field_type->reserved_bits);
    for (i = 0; i < length; i++) {
        bits += halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)(string + i), 0x10);
    }
    return bits;
}

}  // namespace halo::networking

namespace halo::networking {
int32_t message_delta_blob_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::StringFieldCodec::blob_compute_size(field_type);
}

int32_t message_delta_string_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::StringFieldCodec::string_compute_size(field_type);
}

int32_t message_delta_string_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::StringFieldCodec::string_decode(field_type, previous, current, stream);
}

int32_t message_delta_string_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::StringFieldCodec::string_encode(field_type, previous, current, stream);
}

int32_t message_delta_wide_string_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::StringFieldCodec::wide_string_compute_size(field_type);
}

int32_t message_delta_wide_string_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::StringFieldCodec::wide_string_decode(field_type, previous, current, stream);
}

int32_t message_delta_wide_string_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::StringFieldCodec::wide_string_encode(field_type, previous, current, stream);
}

}
