/**
 * @file src/networking/net2_message_delta_aggregate.cpp
 * Array, structure and compound message-delta field codecs.
 */
#include "message_delta_codec.h"
#include "halo/networking/delta_message_types.hpp"
#include <stdint.h>
#include "halo/networking/net2_message_delta_aggregate.hpp"
#include "halo/networking/field_codec.hpp"
#include "halo/memory/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"

static auto &message_delta_definitions = halo::link::ref<message_delta_definition * [56]>(halo::networking::vars().message_delta_definitions);

typedef int32_t (*message_delta_field_decode_fn)(void *field_type, int32_t changed, int32_t offset, bit_stream *stream);

namespace halo::networking {

int32_t AggregateFieldCodec::array_field_decode(message_delta_field_type *field_type, uint8_t *previous,
    uint8_t *destination, bit_stream *stream)
{
    message_delta_array_descriptor *descriptor;
    int32_t total_bits;
    int32_t index;
    int32_t element_bits;
    uint8_t *src;
    uint8_t *dst;
    uint32_t absolute_bit;
    uint32_t first_bit;
    uint32_t target_bit;
    int32_t reserved_bits;
    int32_t flag_position;
    int32_t data_position;
    uint8_t changed_bit;
    int32_t changed_total;
    uint32_t stride_bytes;
    uint32_t stride_words;
    uint32_t stride_tail;

    descriptor = (message_delta_array_descriptor *)field_type->array_descriptor;
    total_bits = 0;
    changed_total = 0;

    if (previous == 0) {
        if (0 < descriptor->count) {
            for (index = 0; index < descriptor->count; index = index + 1) {
                element_bits = descriptor->field_type->decode(descriptor->field_type, 0,
                    destination + descriptor->element_size * index, stream);
                total_bits = total_bits + element_bits;
            }
        }
        return total_bits;
    }

    absolute_bit = stream->byte_cursor * 8 + stream->bit_cursor;
    reserved_bits = field_type->reserved_bits;
    flag_position = (int32_t)(absolute_bit - stream->first_bit);
    target_bit = absolute_bit + reserved_bits;
    if ((reserved_bits > -1 || target_bit <= absolute_bit) &&
        (reserved_bits < 1 || absolute_bit <= target_bit) &&
        ((stream->first_bit <= target_bit && target_bit <= stream->last_bit) ||
            target_bit == stream->last_bit + 1)) {
        stream->bit_cursor = target_bit & 7;
        stream->byte_cursor = target_bit >> 3;
    }

    if (descriptor->count < 1) {
        return 0;
    }
    for (index = 0; index < descriptor->count; index = index + 1) {
        src = previous + descriptor->element_size * index;
        dst = destination + descriptor->element_size * index;

        first_bit = stream->first_bit;
        data_position = (int32_t)((stream->bit_cursor + stream->byte_cursor * 8) - first_bit);
        changed_bit = 0;

        target_bit = (uint32_t)flag_position + first_bit;
        if ((flag_position > -1 || target_bit <= first_bit) &&
            (flag_position < 1 || first_bit <= target_bit) &&
            ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                target_bit == stream->last_bit + 1)) {
            stream->bit_cursor = target_bit & 7;
            stream->byte_cursor = target_bit >> 3;
        }

        halo::memory::bit_stream_read_bit(&changed_bit, stream);

        first_bit = stream->first_bit;
        target_bit = first_bit + (uint32_t)data_position;
        if ((data_position > -1 || target_bit <= first_bit) &&
            (data_position < 1 || first_bit <= target_bit) &&
            ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                target_bit == stream->last_bit + 1)) {
            stream->bit_cursor = target_bit & 7;
            stream->byte_cursor = target_bit >> 3;
        }

        if (changed_bit == 0) {
            stride_bytes = (uint32_t)descriptor->element_size;
            for (stride_words = stride_bytes >> 2; stride_words != 0; stride_words = stride_words - 1) {
                *(uint32_t *)dst = *(uint32_t *)src;
                src = src + 4;
                dst = dst + 4;
            }
            for (stride_tail = stride_bytes & 3; stride_tail != 0; stride_tail = stride_tail - 1) {
                *dst = *src;
                src = src + 1;
                dst = dst + 1;
            }
        } else {
            element_bits = descriptor->field_type->decode(descriptor->field_type, src, dst, stream);
            changed_total = changed_total + element_bits;
        }

        flag_position = flag_position + 1;
    }
    if (0 < changed_total) {
        return changed_total + field_type->reserved_bits;
    }
    return changed_total;
}

int32_t AggregateFieldCodec::array_field_encode(message_delta_field_type *field_type, uint8_t *previous,
    uint8_t *destination, bit_stream *stream)
{
    message_delta_array_field_list *list;
    int32_t total_bits;
    int32_t index;
    int32_t element_bits;
    uint32_t absolute_bit;
    uint32_t first_bit;
    uint32_t target_bit;
    int32_t reserved_bits;
    int32_t block_position;
    int32_t flag_position;
    int32_t data_position;
    message_delta_field_binding *field;

    list = (message_delta_array_field_list *)field_type->array_descriptor;
    total_bits = 0;

    if (previous == 0) {
        if (0 < list->count) {
            for (index = 0; index < list->count; index = index + 1) {
                field = &list->fields[index];
                element_bits = field->field_type->encode(field->field_type, 0,
                    destination + field->destination_offset, stream);
                total_bits = total_bits + element_bits;
            }
        }
        return total_bits;
    }

    reserved_bits = field_type->reserved_bits;
    absolute_bit = stream->bit_cursor + stream->byte_cursor * 8;
    block_position = (int32_t)(absolute_bit - stream->first_bit);
    target_bit = absolute_bit + reserved_bits;
    if ((reserved_bits > -1 || target_bit <= absolute_bit) &&
        (reserved_bits < 1 || absolute_bit <= target_bit) &&
        ((stream->first_bit <= target_bit && target_bit <= stream->last_bit) ||
            target_bit == stream->last_bit + 1)) {
        stream->bit_cursor = target_bit & 7;
        stream->byte_cursor = target_bit >> 3;
    }

    if (0 < list->count) {
        flag_position = block_position;
        for (index = 0; index < list->count; index = index + 1) {
            field = &list->fields[index];
            element_bits = field->field_type->encode(field->field_type,
                previous + field->source_offset, destination + field->destination_offset, stream);
            total_bits = total_bits + element_bits;

            first_bit = stream->first_bit;
            data_position = (int32_t)((stream->bit_cursor + stream->byte_cursor * 8) - first_bit);
            target_bit = (uint32_t)flag_position + first_bit;
            if ((flag_position > -1 || target_bit <= first_bit) &&
                (flag_position < 1 || first_bit <= target_bit) &&
                ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                    target_bit == stream->last_bit + 1)) {
                stream->bit_cursor = target_bit & 7;
                stream->byte_cursor = target_bit >> 3;
            }

            halo::memory::bit_stream_write_bit(0 < element_bits, stream);

            first_bit = stream->first_bit;
            target_bit = first_bit + (uint32_t)data_position;
            if ((data_position > -1 || target_bit <= first_bit) &&
                (data_position < 1 || first_bit <= target_bit) &&
                ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                    target_bit == stream->last_bit + 1)) {
                stream->bit_cursor = target_bit & 7;
                stream->byte_cursor = target_bit >> 3;
            }

            flag_position = flag_position + 1;
        }
        if (0 < total_bits) {
            return total_bits + field_type->reserved_bits;
        }
    }

    first_bit = stream->first_bit;
    target_bit = first_bit + (uint32_t)block_position;
    if ((block_position > -1 || target_bit <= first_bit) &&
        (block_position < 1 || first_bit <= target_bit) &&
        ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
            target_bit == stream->last_bit + 1)) {
        stream->byte_cursor = target_bit >> 3;
        stream->bit_cursor = target_bit & 7;
        return total_bits;
    }
    return total_bits;
}

int32_t AggregateFieldCodec::compound_compute_size(message_delta_field_type *field_type)
{
    message_delta_array_field_list *list = (message_delta_array_field_list *)field_type->array_descriptor;
    int32_t total = 0;
    int32_t i;

    for (i = 0; i < list->count; i++) {
        int32_t bits = FieldCodecRegistry::get(list->fields[i].field_type).compute_size();

        total += bits;
        list->fields[i].field_type->size_bits = bits;
    }
    field_type->reserved_bits = list->count;
    return list->count + total;
}

int32_t AggregateFieldCodec::compound_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    message_delta_array_field_list *list = (message_delta_array_field_list *)field_type->array_descriptor;
    int32_t total = 0;
    int32_t flag;
    int32_t i;

    if (previous == 0) {
        for (i = 0; i < list->count; i++) {
            total += FieldCodecRegistry::get(list->fields[i].field_type).decode(0, (uint8_t *)current + list->fields[i].destination_offset, stream);
        }
        return total;
    }
    flag = (int32_t)(message_delta_stream_position(stream) - stream->first_bit);
    message_delta_stream_seek(stream, message_delta_stream_position(stream), field_type->reserved_bits);
    for (i = 0; i < list->count; i++) {
        message_delta_field_binding *binding = &list->fields[i];
        int32_t data = (int32_t)(message_delta_stream_position(stream) - stream->first_bit);
        uint8_t changed = 0;

        message_delta_stream_seek(stream, stream->first_bit, flag);
        halo::memory::bit_stream_read_bit(&changed, stream);
        message_delta_stream_seek(stream, stream->first_bit, data);
        if (changed) {
            total += FieldCodecRegistry::get(binding->field_type).decode((uint8_t *)previous + binding->source_offset, (uint8_t *)current + binding->destination_offset, stream);
        }
        flag++;
    }
    if (total > 0) {
        return total + field_type->reserved_bits;
    }
    return total;
}

uint8_t AggregateFieldCodec::compound_initialize(message_delta_field_type *field_type)
{
    message_delta_array_field_list *list = (message_delta_array_field_list *)field_type->array_descriptor;
    uint8_t result = FieldCodecRegistry::kind_flag(9) != 1;
    int32_t i;

    if (list->count <= 0) {
        return 0;
    }
    for (i = 0; i < list->count; i++) {
        message_delta_field_binding *binding = &list->fields[i];

        if (binding == 0 || binding->field_type == 0) {
            return 0;
        }
        result = FieldCodecRegistry::get(binding->field_type).initialize();
        if (result != 1) {
            return 0;
        }
    }
    return result;
}

int32_t AggregateFieldCodec::decode_array_field(void **context)
{
    message_delta_decode_state *state;
    int32_t array_bits;
    int32_t treat_as_success;

    state = halo::networking::delta_context(context)->state;
    array_bits = halo::networking::message_delta_decode_field_changed_flags(context);
    treat_as_success = (0 < array_bits);
    if (!treat_as_success && array_bits == 0 && state->incremental == 0) {
        message_delta_definition *definition = message_delta_definitions[state->message_type];
        if (definition->statics->count <= 0) {
            treat_as_success = 1;
        }
    }

    if (treat_as_success) {
        state->bits_read = state->bits_read + array_bits;
        state->more_items = 1;
        return 1;
    }

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
    return 0;
}

uint8_t AggregateFieldCodec::decode_compound_field(void **context, void *destination)
{
    message_delta_decode_state *state;
    int32_t bits;

    state = halo::networking::delta_context(context)->state;
    bits = halo::networking::message_delta_read_changed_subfields(state, halo::networking::delta_context(context)->changed, 0, (int32_t)(int32_t)destination);
    state->bits_read = state->bits_read + bits;
    if (bits == 0) {
        bit_stream *stream = (bit_stream *)state->stream;
        int32_t delta = state->start_bit_offset;
        uint32_t target = (uint32_t)stream->first_bit + (uint32_t)delta;
        if ((delta >= 0 || target <= stream->first_bit) &&
            (delta <= 0 || stream->first_bit <= target) &&
            ((stream->first_bit <= target && target <= stream->last_bit) || target == stream->last_bit + 1)) {
            stream->bit_cursor = target & 7;
            stream->byte_cursor = target >> 3;
        }
        return 0;
    }
    state->changed = 1;
    return 1;
}

uint8_t AggregateFieldCodec::decode_compound_field_forced(void **context, void *destination, int32_t changed_offset,
                                                    uint8_t force)
{
    message_delta_decode_state *state;
    int32_t bits;

    state = halo::networking::delta_context(context)->state;
    bits = halo::networking::message_delta_read_changed_subfields(state, halo::networking::delta_context(context)->changed, changed_offset,
                                                 (int32_t)(int32_t)destination);
    state->bits_read = state->bits_read + bits;
    if (bits == 0 && force == 0) {
        bit_stream *stream = (bit_stream *)state->stream;
        int32_t delta = state->start_bit_offset;
        uint32_t target = (uint32_t)stream->first_bit + (uint32_t)delta;
        if ((delta >= 0 || target <= stream->first_bit) &&
            (delta <= 0 || stream->first_bit <= target) &&
            ((stream->first_bit <= target && target <= stream->last_bit) || target == stream->last_bit + 1)) {
            stream->bit_cursor = target & 7;
            stream->byte_cursor = target >> 3;
        }
        return 0;
    }
    state->changed = 1;
    return 1;
}

uint8_t AggregateFieldCodec::decode_compound_field_staged(void **context)
{
    message_delta_decode_state *state;
    uint8_t scratch[0x800];
    int32_t changed_offset;
    int32_t bits;

    state = halo::networking::delta_context(context)->state;
    changed_offset = (state->incremental == 1) ? (int32_t)(int32_t)scratch : 0;
    bits = halo::networking::message_delta_read_changed_subfields(state, halo::networking::delta_context(context)->changed, changed_offset,
                                                 (int32_t)(int32_t)scratch);
    if (bits == 0 && state->incremental == 0) {
        bit_stream *stream = (bit_stream *)state->stream;
        int32_t delta = state->start_bit_offset;
        uint32_t target = (uint32_t)stream->first_bit + (uint32_t)delta;
        if ((delta >= 0 || target <= stream->first_bit) &&
            (delta <= 0 || stream->first_bit <= target) &&
            ((stream->first_bit <= target && target <= stream->last_bit) || target == stream->last_bit + 1)) {
            stream->bit_cursor = target & 7;
            stream->byte_cursor = target >> 3;
        }
        return 0;
    }
    state->changed = 1;
    state->bits_read = state->bits_read + bits;
    return 1;
}

int32_t AggregateFieldCodec::dword_array_decode(message_delta_field_type *field_type, uint32_t *previous,
    uint32_t *destination, bit_stream *stream)
{
    message_delta_scalar_array_descriptor *descriptor;
    int32_t total_bits;
    int32_t index;
    uint32_t absolute_bit;
    uint32_t first_bit;
    uint32_t target_bit;
    int32_t reserved_bits;
    int32_t flag_position;
    int32_t data_position;
    uint8_t changed_bit;
    uintptr_t previous_offset;
    uint32_t *cursor;
    int32_t decoded_bits;

    descriptor = (message_delta_scalar_array_descriptor *)field_type->array_descriptor;
    total_bits = 0;
    cursor = destination;

    if (previous == 0) {
        if (0 < descriptor->count) {
            for (index = 0; index < descriptor->count; index = index + 1) {
                decoded_bits = halo::memory::bit_stream_read_bits_chunked(0x20, &destination[index], stream);
                total_bits = total_bits + decoded_bits;
            }
        }
        return total_bits;
    }

    reserved_bits = field_type->reserved_bits;
    absolute_bit = stream->bit_cursor + stream->byte_cursor * 8;
    flag_position = (int32_t)(absolute_bit - stream->first_bit);
    target_bit = absolute_bit + reserved_bits;
    if ((reserved_bits > -1 || target_bit <= absolute_bit) &&
        (reserved_bits < 1 || absolute_bit <= target_bit) &&
        ((stream->first_bit <= target_bit && target_bit <= stream->last_bit) ||
            target_bit == stream->last_bit + 1)) {
        stream->bit_cursor = target_bit & 7;
        stream->byte_cursor = target_bit >> 3;
    }

    if (descriptor->count < 1) {
        return total_bits;
    }
    previous_offset = (uint8_t *)previous - (uint8_t *)destination;
    for (index = 0; index < descriptor->count; index = index + 1) {
        first_bit = stream->first_bit;
        data_position = (int32_t)((stream->bit_cursor + stream->byte_cursor * 8) - first_bit);
        changed_bit = 0;

        target_bit = (uint32_t)flag_position + first_bit;
        if ((flag_position > -1 || target_bit <= first_bit) &&
            (flag_position < 1 || first_bit <= target_bit) &&
            ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                target_bit == stream->last_bit + 1)) {
            stream->bit_cursor = target_bit & 7;
            stream->byte_cursor = target_bit >> 3;
        }

        halo::memory::bit_stream_read_bit(&changed_bit, stream);

        first_bit = stream->first_bit;
        target_bit = first_bit + (uint32_t)data_position;
        if ((data_position > -1 || target_bit <= first_bit) &&
            (data_position < 1 || first_bit <= target_bit) &&
            ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                target_bit == stream->last_bit + 1)) {
            stream->bit_cursor = target_bit & 7;
            stream->byte_cursor = target_bit >> 3;
        }

        if (changed_bit == 0) {
            *cursor = *(uint32_t *)((uint8_t *)cursor + previous_offset);
        } else {
            decoded_bits = halo::memory::bit_stream_read_bits_chunked(0x20, cursor, stream);
            total_bits = total_bits + decoded_bits;
        }

        cursor = cursor + 1;
        flag_position = flag_position + 1;
    }
    if (0 < total_bits) {
        return total_bits + field_type->reserved_bits;
    }
    return total_bits;
}

int32_t AggregateFieldCodec::float_array_encode(message_delta_field_type *field_type, float *previous,
    float *values, bit_stream *stream)
{
    message_delta_scalar_array_descriptor *descriptor;
    int32_t total_bits;
    float *cursor;
    int32_t index;
    int32_t remaining_width;
    uint32_t absolute_bit;
    uint32_t first_bit;
    uint32_t target_bit;
    int32_t reserved_bits;
    int32_t block_position;
    int32_t flag_position;
    int32_t data_position;
    float delta;
    uint32_t changed_bit;
    int32_t written_bits;
    uintptr_t previous_offset;

    descriptor = (message_delta_scalar_array_descriptor *)field_type->array_descriptor;
    total_bits = 0;
    cursor = values;

    if (previous == 0) {
        if (0 < descriptor->count) {
            for (index = 0; index < descriptor->count; index = index + 1) {
                remaining_width = 0x20;
                if (halo::memory::bit_stream_write_bits(0x20, *(uint32_t *)&cursor[index], stream) != 0) {
                    remaining_width = 0;
                }
                total_bits = total_bits + (0x20 - remaining_width);
            }
        }
        return total_bits;
    }

    previous_offset = (uint8_t *)previous - (uint8_t *)values;
    reserved_bits = field_type->reserved_bits;
    absolute_bit = stream->bit_cursor + stream->byte_cursor * 8;
    block_position = (int32_t)(absolute_bit - stream->first_bit);
    target_bit = absolute_bit + reserved_bits;
    if ((reserved_bits > -1 || target_bit <= absolute_bit) &&
        (reserved_bits < 1 || absolute_bit <= target_bit) &&
        ((stream->first_bit <= target_bit && target_bit <= stream->last_bit) ||
            target_bit == stream->last_bit + 1)) {
        stream->bit_cursor = target_bit & 7;
        stream->byte_cursor = target_bit >> 3;
    }

    if (0 < descriptor->count) {
        flag_position = block_position;
        for (index = 0; index < descriptor->count; index = index + 1) {
            delta = *(float *)((uint8_t *)cursor + previous_offset) - *cursor;
            if (delta < -0.0001f || 0.0001f < delta) {
                changed_bit = 1;
                written_bits = halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)cursor, 0x20);
                total_bits = total_bits + written_bits;
            } else {
                changed_bit = 0;
            }

            first_bit = stream->first_bit;
            data_position = (int32_t)((stream->bit_cursor + stream->byte_cursor * 8) - first_bit);
            target_bit = first_bit + (uint32_t)flag_position;
            if ((flag_position > -1 || target_bit <= first_bit) &&
                (flag_position < 1 || first_bit <= target_bit) &&
                ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                    target_bit == stream->last_bit + 1)) {
                stream->bit_cursor = target_bit & 7;
                stream->byte_cursor = target_bit >> 3;
            }

            halo::memory::bit_stream_write_bit((int32_t)changed_bit, stream);

            first_bit = stream->first_bit;
            target_bit = first_bit + (uint32_t)data_position;
            if ((data_position > -1 || target_bit <= first_bit) &&
                (data_position < 1 || first_bit <= target_bit) &&
                ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                    target_bit == stream->last_bit + 1)) {
                stream->bit_cursor = target_bit & 7;
                stream->byte_cursor = target_bit >> 3;
            }

            cursor = cursor + 1;
            flag_position = flag_position + 1;
        }
        if (0 < total_bits) {
            return total_bits + field_type->reserved_bits;
        }
    }

    first_bit = stream->first_bit;
    target_bit = first_bit + (uint32_t)block_position;
    if ((block_position > -1 || target_bit <= first_bit) &&
        (block_position < 1 || first_bit <= target_bit) &&
        ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
            target_bit == stream->last_bit + 1)) {
        stream->byte_cursor = target_bit >> 3;
        stream->bit_cursor = target_bit & 7;
        return total_bits;
    }
    return total_bits;
}

int32_t AggregateFieldCodec::read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
                                              int32_t changed_offset, int32_t destination_offset)
{
    bit_stream *stream;
    uint32_t position;
    int32_t total_bits;
    int32_t field_count;
    int32_t i;

    stream = (bit_stream *)state->stream;
    position = stream->bit_cursor + stream->byte_cursor * 8;
    total_bits = 0;
    if (stream->first_bit <= position && position <= stream->last_bit) {
        message_delta_definition *definition = message_delta_definitions[state->message_type];
        field_count = definition->field_count;
        for (i = 0; i < field_count; i++) {
            if (changed_flags[i] != 0) {
                message_delta_field_binding *binding = &definition->fields[i];
                int32_t src_arg = (changed_offset == 0) ? 0 : (binding->source_offset + changed_offset);
                message_delta_field_decode_fn decode =
                    *(message_delta_field_decode_fn *)((uint8_t *)binding->field_type + 0x54);
                int32_t field_bits = decode(binding->field_type, src_arg,
                                             binding->destination_offset + destination_offset, stream);
                if (field_bits < 1) {
                    return 0;
                }
                total_bits = total_bits + field_bits;
            }
        }
    }
    return total_bits;
}

int32_t AggregateFieldCodec::scalar_array_compute_size(message_delta_field_type *field_type)
{
    int32_t count = *(int32_t *)field_type->array_descriptor;

    field_type->reserved_bits = count;
    return (count << 5) + count;
}

int32_t AggregateFieldCodec::structure_array_compute_size(message_delta_field_type *field_type)
{
    message_delta_array_descriptor *descriptor = (message_delta_array_descriptor *)field_type->array_descriptor;
    int32_t count = descriptor->count;
    int32_t element_bits = FieldCodecRegistry::get(descriptor->field_type).compute_size();

    descriptor->field_type->size_bits = element_bits;
    field_type->reserved_bits = count;
    return descriptor->count * element_bits + count;
}

int32_t AggregateFieldCodec::structure_array_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    message_delta_array_descriptor *descriptor = (message_delta_array_descriptor *)field_type->array_descriptor;
    int32_t total = 0;
    int32_t block;
    int32_t flag;
    int32_t i;

    if (previous == 0) {
        for (i = 0; i < descriptor->count; i++) {
            total += FieldCodecRegistry::get(descriptor->field_type).encode(0, (uint8_t *)current + descriptor->element_size * i, stream);
        }
        return total;
    }
    block = (int32_t)(message_delta_stream_position(stream) - stream->first_bit);
    message_delta_stream_seek(stream, message_delta_stream_position(stream), field_type->reserved_bits);
    flag = block;
    for (i = 0; i < descriptor->count; i++) {
        int32_t offset = descriptor->element_size * i;
        int32_t bits = FieldCodecRegistry::get(descriptor->field_type).encode((uint8_t *)previous + offset, (uint8_t *)current + offset, stream);
        int32_t data;

        total += bits;
        data = (int32_t)(message_delta_stream_position(stream) - stream->first_bit);
        message_delta_stream_seek(stream, stream->first_bit, flag);
        halo::memory::bit_stream_write_bit(bits > 0, stream);
        message_delta_stream_seek(stream, stream->first_bit, data);
        flag++;
    }
    if (total > 0) {
        return total + field_type->reserved_bits;
    }
    message_delta_stream_seek(stream, stream->first_bit, block);
    return total;
}

uint8_t AggregateFieldCodec::structure_array_initialize(message_delta_field_type *field_type)
{
    message_delta_array_descriptor *descriptor = (message_delta_array_descriptor *)field_type->array_descriptor;

    if (descriptor->count <= 0 || descriptor->element_size <= 0 || descriptor->field_type == 0) {
        return 0;
    }
    return FieldCodecRegistry::get(descriptor->field_type).initialize() == 1;
}

}  // namespace halo::networking

namespace halo::networking {
int32_t message_delta_array_field_decode(message_delta_field_type *field_type, uint8_t *previous,
    uint8_t *destination, bit_stream *stream)
{
    return halo::networking::AggregateFieldCodec::array_field_decode(field_type, previous, destination, stream);
}

int32_t message_delta_array_field_encode(message_delta_field_type *field_type, uint8_t *previous,
    uint8_t *destination, bit_stream *stream)
{
    return halo::networking::AggregateFieldCodec::array_field_encode(field_type, previous, destination, stream);
}

int32_t message_delta_compound_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::AggregateFieldCodec::compound_compute_size(field_type);
}

int32_t message_delta_compound_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::AggregateFieldCodec::compound_decode(field_type, previous, current, stream);
}

uint8_t message_delta_compound_initialize(message_delta_field_type *field_type)
{
    return halo::networking::AggregateFieldCodec::compound_initialize(field_type);
}

int32_t message_delta_decode_array_field(void **context)
{
    return halo::networking::AggregateFieldCodec::decode_array_field(context);
}

uint8_t message_delta_decode_compound_field(void **context, void *destination)
{
    return halo::networking::AggregateFieldCodec::decode_compound_field(context, destination);
}

uint8_t message_delta_decode_compound_field_forced(void **context, void *destination, int32_t changed_offset,
                                                    uint8_t force)
{
    return halo::networking::AggregateFieldCodec::decode_compound_field_forced(context, destination, changed_offset, force);
}

uint8_t message_delta_decode_compound_field_staged(void **context)
{
    return halo::networking::AggregateFieldCodec::decode_compound_field_staged(context);
}

int32_t message_delta_dword_array_decode(message_delta_field_type *field_type, uint32_t *previous,
    uint32_t *destination, bit_stream *stream)
{
    return halo::networking::AggregateFieldCodec::dword_array_decode(field_type, previous, destination, stream);
}

int32_t message_delta_float_array_encode(message_delta_field_type *field_type, float *previous,
    float *values, bit_stream *stream)
{
    return halo::networking::AggregateFieldCodec::float_array_encode(field_type, previous, values, stream);
}

int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
                                              int32_t changed_offset, int32_t destination_offset)
{
    return halo::networking::AggregateFieldCodec::read_changed_subfields(state, changed_flags, changed_offset, destination_offset);
}

int32_t message_delta_scalar_array_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::AggregateFieldCodec::scalar_array_compute_size(field_type);
}

int32_t message_delta_structure_array_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::AggregateFieldCodec::structure_array_compute_size(field_type);
}

int32_t message_delta_structure_array_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::AggregateFieldCodec::structure_array_encode(field_type, previous, current, stream);
}

uint8_t message_delta_structure_array_initialize(message_delta_field_type *field_type)
{
    return halo::networking::AggregateFieldCodec::structure_array_initialize(field_type);
}

}
