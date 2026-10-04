/**
 * @file src/networking/net2_message_delta_index.cpp
 * Index, pointer, placement and range message-delta field codecs.
 */
#include "message_delta_codec.h"
#include "halo/core/cstring.hpp"
#include "halo/core/datum.hpp"
#include "win32.h"
#include "halo/networking/net2_message_delta_index.hpp"
#include "halo/networking/field_codec.hpp"
#include "halo/memory/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/platform/memory.hpp"

static auto &item_placement_bits_x = halo::link::ref<uint32_t>(halo::networking::vars().item_placement_bits_x);
static auto &item_placement_bits_y = halo::link::ref<uint32_t>(halo::networking::vars().item_placement_bits_y);
static auto &item_placement_bits_z = halo::link::ref<uint32_t>(halo::networking::vars().item_placement_bits_z);
static auto &message_delta_parameters_enabled = halo::link::ref<uint8_t>(halo::networking::vars().message_delta_parameters_enabled);


namespace halo::networking {

uint8_t IndexFieldCodec::count_initialize(message_delta_field_type *field_type)
{
    return *(int32_t *)field_type->array_descriptor > 0;
}

int32_t IndexFieldCodec::enum_width_compute_size(message_delta_field_type *field_type)
{
    switch (*(int32_t *)field_type->array_descriptor) {
    case 0:
        return 1;
    case 1:
        return 2;
    default:
        return 4;
    }
}

int32_t IndexFieldCodec::first_dword_compute_size(message_delta_field_type *field_type)
{
    return *(int32_t *)field_type->array_descriptor;
}

int32_t IndexFieldCodec::grenade_counts_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t packed = 0;
    int32_t bits = halo::memory::bit_stream_read_bits_chunked(6, &packed, stream);

    (void)field_type;
    (void)previous;
    ((uint8_t *)current)[0] = (uint8_t)(packed >> 3);
    ((uint8_t *)current)[1] = (uint8_t)(packed & 7);
    return bits;
}

int32_t IndexFieldCodec::grenade_counts_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int8_t *counts = (int8_t *)current;
    int32_t packed = ((int32_t)counts[0] << 3) | (int32_t)counts[1];

    (void)field_type;
    if (previous != 0 && packed == ((((int32_t)((int8_t *)previous)[0]) << 3) | (int32_t)((int8_t *)previous)[1])) {
        return 0;
    }
    return halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)&packed, 6);
}

int32_t IndexFieldCodec::grenade_index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t code = 0;
    int32_t bits = halo::memory::bit_stream_read_bits_chunked(2, &code, stream);

    (void)field_type;
    (void)previous;
    if (code & 2) {
        *(int16_t *)current = -1;
    } else {
        *(int16_t *)current = (int16_t)(code & 1);
    }
    return bits;
}

int32_t IndexFieldCodec::grenade_index_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint16_t index = *(uint16_t *)current;
    int32_t code = ((index == halo::k_word_none) << 1) | (index & 1);

    (void)field_type;
    if (previous != 0) {
        int16_t previous_index = *(int16_t *)previous;

        if (code == ((((uint16_t)previous_index == halo::k_word_none) << 1) | (previous_index & 1))) {
            return 0;
        }
    }
    return halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)&code, 2);
}

int32_t IndexFieldCodec::index_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    descriptor[2] = message_delta_item_count_bits[descriptor[0] + 1];
    return descriptor[2];
}

int32_t IndexFieldCodec::index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint32_t value = 0;
    int32_t bits = halo::memory::bit_stream_read_bits_chunked(descriptor[2], &value, stream);

    (void)previous;
    *(uint32_t *)current = value;
    return bits;
}

int32_t IndexFieldCodec::index_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    if (previous != 0 && *(uint32_t *)current == *(uint32_t *)previous) {
        return 0;
    }
    return halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)current, descriptor[2]);
}

uint8_t IndexFieldCodec::index_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    if (descriptor[0] < 1 || descriptor[1] < 1) {
        return 0;
    }
    if (field_type->initialized == 0) {
        int32_t *table;

        halo::objects::hash_table_initialize((hash_table *)(descriptor + 3), descriptor[1]);
        table = (int32_t *)halo::platform::heap_allocate(0, descriptor[0] * 4);
        descriptor[10] = (int32_t)table;
        descriptor[9] = 0;
        memset(table, 0xff, descriptor[0] * 4);
        halo::objects::hash_table_set_or_remove((hash_table *)(descriptor + 3), -1, 0);
        *(int32_t *)descriptor[10] = 1;
    }
    return 1;
}

void IndexFieldCodec::index_teardown(message_delta_field_type *field_type)
{
    int32_t *descriptor = FieldCodecRegistry::kind_flag(13) == 1 ? (int32_t *)field_type->array_descriptor : 0;

    halo::platform::heap_free((void *)descriptor[10]);
    halo::objects::hash_table_dispose((hash_table *)(descriptor + 3));
}

int32_t IndexFieldCodec::item_placement_compute_size(message_delta_field_type *field_type)
{
    (void)field_type;
    return (int32_t)(item_placement_bits_y + item_placement_bits_z + item_placement_bits_x);
}

int32_t IndexFieldCodec::item_placement_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    real *position = (real *)current;
    uint32_t value;
    int32_t total = 0;

    (void)field_type;
    (void)previous;
    value = 0;
    total += halo::memory::bit_stream_read_bits_chunked((int32_t)item_placement_bits_x, &value, stream);
    position[0] = (real)((double)value / (double)(uint32_t)((1 << item_placement_bits_x) - 1)) * 10000.0f - 5000.0f;
    value = 0;
    total += halo::memory::bit_stream_read_bits_chunked((int32_t)item_placement_bits_y, &value, stream);
    position[1] = (real)((double)value / (double)(uint32_t)((1 << item_placement_bits_y) - 1)) * 10000.0f - 5000.0f;
    value = 0;
    total += halo::memory::bit_stream_read_bits_chunked((int32_t)item_placement_bits_z, &value, stream);
    position[2] = (real)((double)value / (double)(uint32_t)((1 << item_placement_bits_z) - 1)) * 10000.0f - 5000.0f;
    return total;
}

int32_t IndexFieldCodec::item_placement_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    real *position = (real *)current;
    int32_t total = 0;

    (void)field_type;
    (void)previous;
    {
        uint32_t levels = (uint32_t)((1 << item_placement_bits_x) - 1);
        uint32_t level = (uint32_t)(int64_t)halo::libm::floor((double)((real)levels * ((position[0] - -5000.0f) * 0.0001f) + 0.5f));

        if (level > levels) {
            level = levels;
        }
        total += halo::memory::bit_stream_write_bits_chunked(stream, &level, (int32_t)item_placement_bits_x);
    }
    {
        uint32_t levels = (uint32_t)((1 << item_placement_bits_y) - 1);
        uint32_t level = (uint32_t)(int64_t)halo::libm::floor((double)((real)levels * ((position[1] - -5000.0f) * 0.0001f) + 0.5f));

        if (level > levels) {
            level = levels;
        }
        total += halo::memory::bit_stream_write_bits_chunked(stream, &level, (int32_t)item_placement_bits_y);
    }
    {
        uint32_t levels = (uint32_t)((1 << item_placement_bits_z) - 1);
        uint32_t level = (uint32_t)(int64_t)halo::libm::floor((double)((real)levels * ((position[2] - -5000.0f) * 0.0001f) + 0.5f));

        if (level > levels) {
            level = levels;
        }
        total += halo::memory::bit_stream_write_bits_chunked(stream, &level, (int32_t)item_placement_bits_z);
    }
    return total;
}

uint8_t IndexFieldCodec::item_placement_initialize(message_delta_field_type *field_type)
{
    (void)field_type;
    if (message_delta_parameters_enabled == 1) {
        halo::networking::message_delta_parameters_protocol_register(0, halo::mutable_literal("gITEM_PLACEMENT_BITS_X"), 1, &item_placement_bits_x);
        halo::networking::message_delta_parameters_protocol_register(0, halo::mutable_literal("gITEM_PLACEMENT_BITS_Y"), 1, &item_placement_bits_y);
        halo::networking::message_delta_parameters_protocol_register(0, halo::mutable_literal("gITEM_PLACEMENT_BITS_Z"), 1, &item_placement_bits_z);
    }
    return 1;
}

int32_t IndexFieldCodec::pointer_compute_size(message_delta_field_type *field_type)
{
    message_delta_field_type **descriptor = (message_delta_field_type **)field_type->array_descriptor;
    int32_t bits = FieldCodecRegistry::get(descriptor[0]).compute_size();

    descriptor[0]->size_bits = bits;
    return bits;
}

int32_t IndexFieldCodec::pointer_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    message_delta_field_type *pointed = *(message_delta_field_type **)field_type->array_descriptor;
    void *previous_value = previous != 0 ? *(void **)previous : 0;

    return FieldCodecRegistry::get(pointed).decode(previous_value, *(void **)current, stream);
}

int32_t IndexFieldCodec::pointer_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    message_delta_field_type *pointed = *(message_delta_field_type **)field_type->array_descriptor;
    void *previous_value = previous != 0 ? *(void **)previous : 0;

    return FieldCodecRegistry::get(pointed).encode(previous_value, *(void **)current, stream);
}

uint8_t IndexFieldCodec::pointer_initialize(message_delta_field_type *field_type)
{
    message_delta_field_type *pointed = *(message_delta_field_type **)field_type->array_descriptor;

    if (pointed == 0) {
        return 0;
    }
    return FieldCodecRegistry::get(pointed).initialize() == 1;
}

int32_t IndexFieldCodec::range_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    return message_delta_item_count_bits[descriptor[1] - descriptor[0] + 1];
}

uint8_t IndexFieldCodec::range_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    return descriptor[1] > descriptor[0];
}

int32_t IndexFieldCodec::weapon_index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t code = 0;
    int32_t bits = halo::memory::bit_stream_read_bits_chunked(3, &code, stream);

    (void)field_type;
    (void)previous;
    if (code & 4) {
        *(int16_t *)current = -1;
    } else {
        *(int16_t *)current = (int16_t)(code & 3);
    }
    return bits;
}

int32_t IndexFieldCodec::weapon_index_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint16_t index = *(uint16_t *)current;
    int32_t code = ((index == halo::k_word_none) << 2) | (index & 3);

    (void)field_type;
    if (previous != 0) {
        int16_t previous_index = *(int16_t *)previous;

        if (code == ((((uint16_t)previous_index == halo::k_word_none) << 2) | (previous_index & 3))) {
            return 0;
        }
    }
    return halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)&code, 3);
}

}  // namespace halo::networking

namespace halo::networking {
uint8_t message_delta_count_initialize(message_delta_field_type *field_type)
{
    return halo::networking::IndexFieldCodec::count_initialize(field_type);
}

int32_t message_delta_enum_width_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::IndexFieldCodec::enum_width_compute_size(field_type);
}

int32_t message_delta_first_dword_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::IndexFieldCodec::first_dword_compute_size(field_type);
}

int32_t message_delta_grenade_counts_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::grenade_counts_decode(field_type, previous, current, stream);
}

int32_t message_delta_grenade_counts_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::grenade_counts_encode(field_type, previous, current, stream);
}

int32_t message_delta_grenade_index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::grenade_index_decode(field_type, previous, current, stream);
}

int32_t message_delta_grenade_index_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::grenade_index_encode(field_type, previous, current, stream);
}

int32_t message_delta_index_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::IndexFieldCodec::index_compute_size(field_type);
}

int32_t message_delta_index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::index_decode(field_type, previous, current, stream);
}

int32_t message_delta_index_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::index_encode(field_type, previous, current, stream);
}

uint8_t message_delta_index_initialize(message_delta_field_type *field_type)
{
    return halo::networking::IndexFieldCodec::index_initialize(field_type);
}

void message_delta_index_teardown(message_delta_field_type *field_type)
{
    halo::networking::IndexFieldCodec::index_teardown(field_type);
}

int32_t message_delta_item_placement_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::IndexFieldCodec::item_placement_compute_size(field_type);
}

int32_t message_delta_item_placement_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::item_placement_decode(field_type, previous, current, stream);
}

int32_t message_delta_item_placement_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::item_placement_encode(field_type, previous, current, stream);
}

uint8_t message_delta_item_placement_initialize(message_delta_field_type *field_type)
{
    return halo::networking::IndexFieldCodec::item_placement_initialize(field_type);
}

int32_t message_delta_pointer_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::IndexFieldCodec::pointer_compute_size(field_type);
}

int32_t message_delta_pointer_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::pointer_decode(field_type, previous, current, stream);
}

int32_t message_delta_pointer_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::pointer_encode(field_type, previous, current, stream);
}

uint8_t message_delta_pointer_initialize(message_delta_field_type *field_type)
{
    return halo::networking::IndexFieldCodec::pointer_initialize(field_type);
}

int32_t message_delta_range_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::IndexFieldCodec::range_compute_size(field_type);
}

uint8_t message_delta_range_initialize(message_delta_field_type *field_type)
{
    return halo::networking::IndexFieldCodec::range_initialize(field_type);
}

int32_t message_delta_weapon_index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::weapon_index_decode(field_type, previous, current, stream);
}

int32_t message_delta_weapon_index_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::IndexFieldCodec::weapon_index_encode(field_type, previous, current, stream);
}

}
