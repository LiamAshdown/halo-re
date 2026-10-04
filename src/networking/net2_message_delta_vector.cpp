/**
 * @file src/networking/net2_message_delta_vector.cpp
 * Vector, normal, throttle and quantized real message-delta field codecs.
 */
#include "halo/math/constants.hpp"
#include "message_delta_codec.h"
#include "halo/core/cstring.hpp"
#include <math.h>
#include "halo/networking/net2_message_delta_vector.hpp"
#include "halo/memory/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"
#include "halo/core/libm.hpp"

static auto &message_delta_vector3d_delta_epsilon = halo::link::ref<real>(halo::networking::vars().message_delta_vector3d_delta_epsilon);
static auto &message_delta_vector3d_delta_range = halo::link::ref<real>(halo::networking::vars().message_delta_vector3d_delta_range);
static auto &message_delta_vector3d_mode = halo::link::ref<uint8_t>(halo::networking::vars().message_delta_vector3d_mode);
static auto &message_delta_vector3d_delta_bits = halo::link::ref<uint32_t>(halo::networking::vars().message_delta_vector3d_delta_bits);
static auto &message_delta_vector3d_absolute_bits_mode0 = halo::link::ref<uint32_t>(halo::networking::vars().message_delta_vector3d_absolute_bits_mode0);
static auto &message_delta_vector3d_absolute_bits_mode1 = halo::link::ref<uint32_t>(halo::networking::vars().message_delta_vector3d_absolute_bits_mode1);
static auto &message_delta_parameters_enabled = halo::link::ref<uint8_t>(halo::networking::vars().message_delta_parameters_enabled);
static auto &message_delta_unary_ones = halo::link::ref<uint32_t []>(halo::networking::vars().message_delta_unary_ones);

static int32_t write_zero_bit(bit_stream *stream)
{
    uint32_t position = message_delta_stream_position(stream);

    if (position < stream->first_bit || position > stream->last_bit) {
        return 0;
    }
    stream->data[stream->byte_cursor] &= (uint8_t)~(1 << stream->bit_cursor);
    position = message_delta_stream_position(stream) + 1;
    if ((position >= stream->first_bit && position <= stream->last_bit) || position == stream->last_bit + 1) {
        stream->bit_cursor = position & 7;
        stream->byte_cursor = position >> 3;
    }
    return 1;
}

namespace halo::networking {

int32_t VectorFieldCodec::decode_vector3d_indexed(int32_t param_1, int32_t mode, real *destination,
    bit_stream *stream)
{
    uint8_t *table;
    int32_t total_bits;
    int32_t index;
    uint32_t bit_value;
    uint32_t absolute_bit;
    uint32_t bit_cursor_byte;
    int32_t got_bit;
    int32_t ratios[3];
    int32_t a, b, c;
    int32_t component_bits;
    real *entry;

    table = *(uint8_t **)(param_1 + 0x58);
    total_bits = 0;

    component_bits = message_delta_vector3d_mode == 0 ? *(int32_t *)(table + 0x10) : *(int32_t *)(table + 8);
    if (mode != 0) {
        a = b = c = 0;
        total_bits = halo::memory::bit_stream_read_bits_chunked(component_bits, (uint32_t *)&a, stream);
        total_bits += halo::memory::bit_stream_read_bits_chunked(component_bits, (uint32_t *)&b, stream);
        total_bits += halo::memory::bit_stream_read_bits_chunked(component_bits, (uint32_t *)&c, stream);
        ratios[0] = a; ratios[1] = b; ratios[2] = c;
        halo::networking::vector3d_lerp_by_mode_denominator((vector3d_lerp_table *)table, (real_vector3d *)destination, ratios);
        return total_bits;
    }

    index = -1;
    bit_value = 1;
    do {
        absolute_bit = stream->bit_cursor + stream->byte_cursor * 8;
        got_bit = 0;
        if (stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) {
            bit_cursor_byte = stream->bit_cursor;
            bit_value = (uint32_t)(uint8_t)((stream->data[stream->byte_cursor] &
                (1 << (bit_cursor_byte & 0x1f))) >> (bit_cursor_byte & 0x1f));
            absolute_bit = absolute_bit + 1;
            if ((stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) ||
                absolute_bit == stream->last_bit + 1) {
                stream->bit_cursor = absolute_bit & 7;
                stream->byte_cursor = absolute_bit >> 3;
            }
            got_bit = 1;
        }
        index = index + (int32_t)(bit_value & 0xff);
        total_bits = total_bits + got_bit;
    } while (*(int32_t *)(table + 0x18) != 1 && (uint8_t)bit_value != 0);

    if (-1 < index) {
        entry = (real *)(table + 0x1c + index * 0xc);
        destination[0] = entry[0];
        destination[1] = entry[1];
        destination[2] = entry[2];
        return total_bits;
    }

    a = b = c = 0;
    total_bits += halo::memory::bit_stream_read_bits_chunked(component_bits, (uint32_t *)&a, stream);
    total_bits += halo::memory::bit_stream_read_bits_chunked(component_bits, (uint32_t *)&b, stream);
    total_bits += halo::memory::bit_stream_read_bits_chunked(component_bits, (uint32_t *)&c, stream);
    ratios[0] = a; ratios[1] = b; ratios[2] = c;
    halo::networking::vector3d_lerp_by_mode_denominator((vector3d_lerp_table *)table, (real_vector3d *)destination, ratios);
    return total_bits;
}

int32_t VectorFieldCodec::encode_vector3d(int32_t unused, real *previous, real *values,
    bit_stream *stream)
{
    real delta[3];
    uint8_t sign[3];
    int32_t total_bits;
    int32_t i;
    uint8_t out_of_range;
    uint32_t absolute_bit;
    uint32_t level_count;
    real level_count_as_float;
    double scaled;
    uint32_t quantized;
    int32_t written_bits;
    uint32_t absolute_bits;

    total_bits = 0;
    if (previous != 0) {
        delta[0] = values[0] - previous[0];
        delta[1] = values[1] - previous[1];
        delta[2] = values[2] - previous[2];
        if (halo::libm::sqrt(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]) <=
            message_delta_vector3d_delta_epsilon) {
            return 0;
        }
        out_of_range = 0;
        for (i = 0; i < 3; i = i + 1) {
            sign[i] = 0;
            if (delta[i] < 0.0f) {
                sign[i] = 1;
                delta[i] = -delta[i];
            }
            if (message_delta_vector3d_delta_range < delta[i]) {
                out_of_range = 1;
            }
        }
        if (!out_of_range && message_delta_vector3d_mode == 1) {

            absolute_bit = stream->byte_cursor * 8 + stream->bit_cursor;
            total_bits = 0;
            if (stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) {
                stream->data[stream->byte_cursor] &= ~(1 << (stream->bit_cursor & 0x1f));
                absolute_bit = stream->bit_cursor + 1 + stream->byte_cursor * 8;
                if ((stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) ||
                    absolute_bit == stream->last_bit + 1) {
                    stream->bit_cursor = absolute_bit & 7;
                    stream->byte_cursor = absolute_bit >> 3;
                }
                total_bits = 1;
            }
            for (i = 0; i < 3; i = i + 1) {
                total_bits = total_bits + (halo::memory::bit_stream_write_bit(sign[i], stream) != 0);
                level_count = (1u << (message_delta_vector3d_delta_bits & 0x1f)) - 1;
                level_count_as_float = (real)(int32_t)level_count;
                if ((int32_t)level_count < 0) {
                    level_count_as_float = level_count_as_float + 4.2949673e+09f;
                }
                scaled = halo::libm::floor((double)(level_count_as_float *
                    (delta[i] / message_delta_vector3d_delta_range) + 0.5));
                quantized = (uint32_t)(int32_t)scaled;
                if (level_count < quantized) {
                    quantized = level_count;
                }
                written_bits = halo::memory::bit_stream_write_bits_chunked(stream, &quantized,
                    (int32_t)message_delta_vector3d_delta_bits);
                total_bits = total_bits + written_bits;
            }
            return total_bits;
        }

        absolute_bit = stream->bit_cursor + stream->byte_cursor * 8;
        total_bits = 0;
        if (stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) {
            stream->data[stream->byte_cursor] |= (uint8_t)(1 << (stream->bit_cursor & 0x1f));
            absolute_bit = stream->bit_cursor + 1 + stream->byte_cursor * 8;
            if ((stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) ||
                absolute_bit == stream->last_bit + 1) {
                stream->bit_cursor = absolute_bit & 7;
                stream->byte_cursor = absolute_bit >> 3;
            }
            total_bits = 1;
        }
    }

    absolute_bits = message_delta_vector3d_mode == 0 ? message_delta_vector3d_absolute_bits_mode0
                                                       : message_delta_vector3d_absolute_bits_mode1;
    level_count = (1u << (absolute_bits & 0x1f)) - 1;
    level_count_as_float = (real)(int32_t)level_count;
    if ((int32_t)level_count < 0) {
        level_count_as_float = level_count_as_float + 4.2949673e+09f;
    }
    for (i = 0; i < 3; i = i + 1) {
        scaled = halo::libm::floor((double)(level_count_as_float * (values[i] - -5000.0) * 0.0001 + 0.5));
        quantized = (uint32_t)(int32_t)scaled;
        if (level_count < quantized) {
            quantized = level_count;
        }
        written_bits = halo::memory::bit_stream_write_bits_chunked(stream, &quantized, (int32_t)absolute_bits);
        total_bits = total_bits + written_bits;
    }
    return total_bits;
}

int32_t VectorFieldCodec::locality_compute_size(message_delta_field_type *field_type)
{
    int32_t mode1 = (int32_t)message_delta_vector3d_absolute_bits_mode1 * 3 + 1;
    int32_t mode0 = (int32_t)message_delta_vector3d_absolute_bits_mode0 * 3 + 1;

    (void)field_type;
    return mode0 > mode1 ? mode0 : mode1;
}

int32_t VectorFieldCodec::locality_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t &message_delta_vector3d_mode = reinterpret_cast<uint32_t &>(::message_delta_vector3d_mode);
    real *destination = (real *)current;
    int32_t total = 0;
    uint32_t bits;
    int32_t i;

    (void)field_type;
    if (previous != 0) {
        uint8_t absolute;

        total = (int32_t)halo::memory::bit_stream_read_bit(&absolute, stream);
        if (!absolute) {
            uint8_t sign[3];
            real delta[3];

            for (i = 0; i < 3; i++) {
                uint32_t value = 0;
                int32_t sign_bits = (int32_t)halo::memory::bit_stream_read_bit(&sign[i], stream);
                int32_t value_bits = halo::memory::bit_stream_read_bits_chunked((int32_t)message_delta_vector3d_delta_bits, &value, stream);

                total += value_bits + sign_bits;
                delta[i] = (real)((double)value / (double)(uint32_t)((1 << message_delta_vector3d_delta_bits) - 1)) *
                           message_delta_vector3d_delta_range;
                if (sign[i]) {
                    delta[i] = -delta[i];
                }
            }
            destination[0] = delta[0] + ((real *)previous)[0];
            destination[1] = delta[1] + ((real *)previous)[1];
            destination[2] = delta[2] + ((real *)previous)[2];
            return total;
        }
    }
    bits = message_delta_vector3d_mode != 0 ? message_delta_vector3d_absolute_bits_mode1 : message_delta_vector3d_absolute_bits_mode0;
    for (i = 0; i < 3; i++) {
        uint32_t value = 0;

        total += halo::memory::bit_stream_read_bits_chunked((int32_t)bits, &value, stream);
        destination[i] = (real)((double)value / (double)(uint32_t)((1 << bits) - 1)) * 10000.0f - 5000.0f;
    }
    return total;
}

uint8_t VectorFieldCodec::locality_initialize(message_delta_field_type *field_type)
{
    (void)field_type;
    if (message_delta_parameters_enabled == 1) {
        halo::networking::message_delta_parameters_protocol_register(0, halo::mutable_literal("LOCALITY_BITS_PER_COMPONENT_FULL"), 1,
            &message_delta_vector3d_absolute_bits_mode1);
        halo::networking::message_delta_parameters_protocol_register(0, halo::mutable_literal("LOCALITY_BITS_PER_COMPONENT_DELTA"), 1, &message_delta_vector3d_delta_bits);
        halo::networking::message_delta_parameters_protocol_register(0, halo::mutable_literal("LOCALITY_DELTA_CUTOFF_DISTANCE"), 0, &message_delta_vector3d_delta_range);
        halo::networking::message_delta_parameters_protocol_register(0, halo::mutable_literal("LOCALITY_MINIMUM_MOVE_DISTANCE"), 0,
            &message_delta_vector3d_delta_epsilon);
    }
    return 1;
}

int32_t VectorFieldCodec::normal_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t second = descriptor[2] + descriptor[3];
    int32_t first = descriptor[0] + descriptor[1];

    return second > first ? second : first;
}

int32_t VectorFieldCodec::normal_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t &message_delta_vector3d_mode = reinterpret_cast<uint32_t &>(::message_delta_vector3d_mode);
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t bits_a = message_delta_vector3d_mode != 0 ? descriptor[0] : descriptor[2];
    int32_t bits_b = message_delta_vector3d_mode != 0 ? descriptor[1] : descriptor[3];
    uint32_t value_a = 0;
    uint32_t value_b = 0;
    int32_t bits;
    real angle_a;
    real angle_b;

    (void)previous;
    bits = halo::memory::bit_stream_read_bits_chunked(bits_a, &value_a, stream);
    bits += halo::memory::bit_stream_read_bits_chunked(bits_b, &value_b, stream);
    angle_a = (real)((double)value_a / (double)(uint32_t)((1 << bits_a) - 1)) * halo::math::k_pi;
    angle_b = (real)((double)value_b / (double)(uint32_t)((1 << bits_b) - 1)) * halo::math::k_two_pi - halo::math::k_half_pi;
    halo::networking::vector3d_from_yaw_pitch((real_vector3d *)current, angle_a, angle_b);
    return bits;
}

int32_t VectorFieldCodec::normal_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t &message_delta_vector3d_mode = reinterpret_cast<uint32_t &>(::message_delta_vector3d_mode);
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t bits_a = message_delta_vector3d_mode != 0 ? descriptor[0] : descriptor[2];
    int32_t bits_b = message_delta_vector3d_mode != 0 ? descriptor[1] : descriptor[3];
    uint32_t levels_a = (uint32_t)((1 << bits_a) - 1);
    uint32_t levels_b = (uint32_t)((1 << bits_b) - 1);
    real angles[2];
    uint32_t level_a;
    uint32_t level_b;

    halo::networking::vector3d_to_angles(angles, *(real_vector3d *)current);
    level_a = (uint32_t)(int64_t)halo::libm::floor((double)(angles[0] * 0.31830987f * (real)levels_a + 0.5f));
    if (level_a > levels_a) {
        level_a = levels_a;
    }
    level_b = (uint32_t)(int64_t)halo::libm::floor((double)((angles[1] - -halo::math::k_half_pi) * 0.15915494f * (real)levels_b + 0.5f));
    if (level_b > levels_b) {
        level_b = levels_b;
    }
    if (previous != 0) {
        uint32_t previous_a;

        halo::networking::vector3d_to_angles(angles, *(real_vector3d *)previous);
        previous_a = halo::networking::message_delta_quantize_float_to_int(levels_a, angles[0], 0.0f, halo::math::k_pi);
        if (level_a == previous_a &&
            level_b == halo::networking::message_delta_quantize_float_to_int(levels_b, angles[1], -halo::math::k_half_pi, 4.712389f)) {
            return 0;
        }
    }
    return halo::memory::bit_stream_write_bits_chunked(stream, &level_a, bits_a) + halo::memory::bit_stream_write_bits_chunked(stream, &level_b, bits_b);
}

uint8_t VectorFieldCodec::normal_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    if (message_delta_parameters_enabled == 1) {
        halo::networking::message_delta_parameters_protocol_register(field_type->name, halo::mutable_literal("bits_theta_internet"), 1, descriptor);
        halo::networking::message_delta_parameters_protocol_register(field_type->name, halo::mutable_literal("bits_phi_internet"), 1, descriptor + 1);
    }
    return descriptor[0] > 0 && descriptor[1] > 0 && descriptor[2] > 0 && descriptor[3] > 0;
}

uint32_t VectorFieldCodec::quantize_float_to_int(uint32_t max_level, real value, real minimum,
    real maximum)
{
    real level_count_as_float;
    double scaled;
    uint32_t result;

    level_count_as_float = (real)(int32_t)max_level;
    if ((int32_t)max_level < 0) {
        level_count_as_float = level_count_as_float + 4.2949673e+09f;
    }
    scaled = halo::libm::floor((double)(level_count_as_float * ((value - minimum) / (maximum - minimum)) + 0.5));
    result = (uint32_t)(int32_t)scaled;
    if (max_level < result) {
        result = max_level;
    }
    return result;
}

int32_t VectorFieldCodec::quantized_real_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t *descriptor = (uint32_t *)field_type->array_descriptor;
    uint32_t level = 0;
    int32_t bits = halo::memory::bit_stream_read_bits_chunked((int32_t)descriptor[0], &level, stream);

    (void)previous;
    *(real *)current = (real)((double)level / (double)descriptor[1]);
    return bits;
}

int32_t VectorFieldCodec::quantized_real_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t *descriptor = (uint32_t *)field_type->array_descriptor;
    uint32_t levels = descriptor[1];
    uint32_t level = (uint32_t)(int64_t)halo::libm::floor((double)((real)levels * *(real *)current + 0.5f));

    if (level > levels) {
        level = levels;
    }
    if (previous != 0 && level == halo::networking::message_delta_quantize_float_to_int(descriptor[1], *(real *)previous, 0.0f, 1.0f)) {
        return 0;
    }
    return halo::memory::bit_stream_write_bits_chunked(stream, &level, (int32_t)descriptor[0]);
}

uint8_t VectorFieldCodec::quantized_real_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    return descriptor[0] > 0 && descriptor[1] > 0;
}

int32_t VectorFieldCodec::real_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)field_type;
    if (previous != 0) {
        real delta = *(real *)previous - *(real *)current;

        if (!(delta < -0.0001f) && !(delta > 0.0001f)) {
            return 0;
        }
    }
    return halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 0x20);
}

int32_t VectorFieldCodec::throttle_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t code = 0;
    int32_t bits = halo::memory::bit_stream_read_bits_chunked(4, &code, stream);

    (void)field_type;
    (void)previous;
    halo::networking::digital_throttle_decode_vector((real *)current, code);
    return bits;
}

int32_t VectorFieldCodec::throttle_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t code = halo::networking::digital_throttle_encode_vector(*(real_vector3d *)current);

    (void)field_type;
    if (previous != 0 && code == halo::networking::digital_throttle_encode_vector(*(real_vector3d *)previous)) {
        return 0;
    }
    return halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)&code, 4);
}

int32_t VectorFieldCodec::vector_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::message_delta_dword_array_decode(field_type, (uint32_t *)previous, (uint32_t *)current, stream);
}

int32_t VectorFieldCodec::vector_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::message_delta_float_array_encode(field_type, (float *)previous, (float *)current, stream);
}

int32_t VectorFieldCodec::velocity_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t mode0 = descriptor[4] * 3;
    int32_t mode1 = descriptor[2] * 3;

    if (descriptor[6] + mode0 + 1 > mode1 + descriptor[6] + 1) {
        return descriptor[6] + mode0 + 1;
    }
    return descriptor[6] + mode1 + 1;
}

int32_t VectorFieldCodec::velocity_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t &message_delta_vector3d_mode = reinterpret_cast<uint32_t &>(::message_delta_vector3d_mode);
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t bits = message_delta_vector3d_mode != 0 ? descriptor[2] : descriptor[4];
    int32_t quantized[3];
    int32_t total;

    halo::networking::vector3d_quantize(quantized, descriptor, (real *)current);
    if (previous == 0) {
        int32_t *table = descriptor + 0x67;
        int32_t i;

        for (i = 0; i < descriptor[6]; i++) {
            if (quantized[0] == table[i * 3] && quantized[1] == table[i * 3 + 1] && quantized[2] == table[i * 3 + 2]) {
                break;
            }
        }
        if (i < descriptor[6]) {
            total = halo::memory::bit_stream_write_bits_chunked(stream, message_delta_unary_ones, i + 1);
            if (descriptor[6] > 1) {
                total += write_zero_bit(stream);
            }
            return total;
        }
        total = write_zero_bit(stream);
    } else {
        int32_t old[3];

        halo::networking::vector3d_quantize(old, descriptor, (real *)previous);
        if (old[0] == quantized[0] && old[1] == quantized[1] && old[2] == quantized[2]) {
            return 0;
        }
        total = 0;
    }
    total += halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)&quantized[0], bits);
    total += halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)&quantized[1], bits);
    total += halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)&quantized[2], bits);
    return total;
}

}  // namespace halo::networking

namespace halo::networking {
int32_t message_delta_decode_vector3d_indexed(int32_t param_1, int32_t mode, real *destination,
    bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::decode_vector3d_indexed(param_1, mode, destination, stream);
}

int32_t message_delta_encode_vector3d(int32_t unused, real *previous, real *values,
    bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::encode_vector3d(unused, previous, values, stream);
}

int32_t message_delta_locality_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::VectorFieldCodec::locality_compute_size(field_type);
}

int32_t message_delta_locality_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::locality_decode(field_type, previous, current, stream);
}

uint8_t message_delta_locality_initialize(message_delta_field_type *field_type)
{
    return halo::networking::VectorFieldCodec::locality_initialize(field_type);
}

int32_t message_delta_normal_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::VectorFieldCodec::normal_compute_size(field_type);
}

int32_t message_delta_normal_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::normal_decode(field_type, previous, current, stream);
}

int32_t message_delta_normal_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::normal_encode(field_type, previous, current, stream);
}

uint8_t message_delta_normal_initialize(message_delta_field_type *field_type)
{
    return halo::networking::VectorFieldCodec::normal_initialize(field_type);
}

uint32_t message_delta_quantize_float_to_int(uint32_t max_level, real value, real minimum,
    real maximum)
{
    return halo::networking::VectorFieldCodec::quantize_float_to_int(max_level, value, minimum, maximum);
}

int32_t message_delta_quantized_real_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::quantized_real_decode(field_type, previous, current, stream);
}

int32_t message_delta_quantized_real_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::quantized_real_encode(field_type, previous, current, stream);
}

uint8_t message_delta_quantized_real_initialize(message_delta_field_type *field_type)
{
    return halo::networking::VectorFieldCodec::quantized_real_initialize(field_type);
}

int32_t message_delta_real_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::real_encode(field_type, previous, current, stream);
}

int32_t message_delta_throttle_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::throttle_decode(field_type, previous, current, stream);
}

int32_t message_delta_throttle_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::throttle_encode(field_type, previous, current, stream);
}

int32_t message_delta_vector_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::vector_decode(field_type, previous, current, stream);
}

int32_t message_delta_vector_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::vector_encode(field_type, previous, current, stream);
}

int32_t message_delta_velocity_compute_size(message_delta_field_type *field_type)
{
    return halo::networking::VectorFieldCodec::velocity_compute_size(field_type);
}

int32_t message_delta_velocity_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return halo::networking::VectorFieldCodec::velocity_encode(field_type, previous, current, stream);
}

}
