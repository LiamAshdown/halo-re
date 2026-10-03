/**
 * @file src/networking/net2_vector_quantize.cpp
 * Vector quantization and digital throttle helpers.
 */
#include "message_delta_codec.h"
#include "halo/networking/net2_vector_quantize.hpp"

extern "C" {
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);
extern uint8_t message_delta_vector3d_mode;
extern uint32_t message_delta_quantize_float_to_int(uint32_t max_level, real value, real minimum, real maximum);
extern real vector3d_normalize_with_length(real_vector3d *v);
extern double acos(double x);
extern double atan(double x);
extern int _isnan(double x);
void digital_throttle_decode_vector(real *out, uint32_t code);
int32_t digital_throttle_encode_vector(real_vector3d vector);
void vector3d_from_yaw_pitch(real_vector3d *out_direction, real yaw, real pitch);
void vector3d_lerp_by_mode_denominator(vector3d_lerp_table *table, real_vector3d *out_point,
    int32_t *ratios);
void vector3d_quantize(int32_t *out_indices, int32_t *descriptor, real *point);
void vector3d_to_angles(real *out, real_vector3d vector);
uint8_t waypoint_table_quantize_initialize(message_delta_field_type *field_type);
}

static real unsigned_int_to_float(int32_t value)
{
    real result = (real)value;
    if (value < 0) {
        result = result + 4.2949673e+09f;
    }
    return result;
}

namespace halo::networking {

void VectorQuantizer::digital_throttle_decode_vector(real *out, uint32_t code)
{
    int32_t i;
    real length;

    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    if (code == 0) {
        return;
    }
    for (i = 1; i >= 0; i--) {
        if (code & 1) {
            out[i] = -1.0f;
        }
        code >>= 1;
        if (code & 1) {
            out[i] = 1.0f;
        }
        code >>= 1;
    }
    length = (real)sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
    if ((length < 0.0f ? -length : length) >= 0.0001) {
        real inverse = 1.0f / length;

        out[0] = inverse * out[0];
        out[1] = inverse * out[1];
        out[2] = inverse * out[2];
    }
}

int32_t VectorQuantizer::digital_throttle_encode_vector(real_vector3d vector)
{
    uint32_t code = vector.i > 0.0001f ? 1 : 0;

    code = code << 1 | (vector.i < -0.0001f ? 1 : 0);
    code = code << 1 | (vector.j > 0.0001f ? 1 : 0);
    code = code << 1 | (vector.j < -0.0001f ? 1 : 0);
    return (int32_t)((code << 1) >> 1);
}

void VectorQuantizer::vector3d_from_yaw_pitch(real_vector3d *out_direction, real yaw, real pitch)
{
    real sin_yaw, sin_pitch, cos_yaw, cos_pitch;

    sin_yaw = (real)sin((double)yaw);
    sin_pitch = (real)sin((double)pitch);
    cos_yaw = (real)cos((double)yaw);
    cos_pitch = (real)cos((double)pitch);

    if (sin_yaw < 0.0001f && -0.0001f < sin_yaw) {
        sin_yaw = 0.0f;
    }
    if (sin_pitch < 0.0001f && -0.0001f < sin_pitch) {
        sin_pitch = 0.0f;
    }
    if (cos_yaw < 0.0001f && -0.0001f < cos_yaw) {
        cos_yaw = 0.0f;
    }
    if (cos_pitch < 0.0001f && -0.0001f < cos_pitch) {
        cos_pitch = 0.0f;
    }

    out_direction->i = cos_pitch * sin_yaw;
    out_direction->j = sin_pitch * sin_yaw;
    out_direction->k = cos_yaw;
}

void VectorQuantizer::vector3d_lerp_by_mode_denominator(vector3d_lerp_table *table, real_vector3d *out_point,
    int32_t *ratios)
{
    int32_t denom_field;
    real denom;
    real numerator;

    denom_field = message_delta_vector3d_mode == 0 ? table->denominator_mode0 : table->denominator_mode1;

    numerator = unsigned_int_to_float(ratios[0]);
    denom = unsigned_int_to_float(denom_field);
    out_point->i = (table->maximum - table->minimum) * (numerator / denom) + table->minimum;

    numerator = unsigned_int_to_float(ratios[1]);
    denom = unsigned_int_to_float(denom_field);
    out_point->j = (table->maximum - table->minimum) * (numerator / denom) + table->minimum;

    numerator = unsigned_int_to_float(ratios[2]);
    denom = unsigned_int_to_float(denom_field);
    out_point->k = (table->maximum - table->minimum) * (numerator / denom) + table->minimum;
}

void VectorQuantizer::run_vector3d_quantize(int32_t *out_indices, int32_t *descriptor, real *point)
{
    uint32_t &message_delta_vector3d_mode = reinterpret_cast<uint32_t &>(::message_delta_vector3d_mode);
    uint32_t levels = message_delta_vector3d_mode != 0 ? (uint32_t)descriptor[3] : (uint32_t)descriptor[5];
    real minimum = *(real *)&descriptor[0];
    real maximum = *(real *)&descriptor[1];

    out_indices[0] = (int32_t)message_delta_quantize_float_to_int(levels, point[0], minimum, maximum);
    out_indices[1] = (int32_t)message_delta_quantize_float_to_int(levels, point[1], minimum, maximum);
    out_indices[2] = (int32_t)message_delta_quantize_float_to_int(levels, point[2], minimum, maximum);
}

void VectorQuantizer::vector3d_to_angles(real *out, real_vector3d vector)
{
    vector3d_normalize_with_length(&vector);
    if (!(vector.i > 0.0001f) && !(vector.i != vector.i) && !(vector.i < -0.0001f)) {
        out[1] = vector.j > 0.0f ? 1.5707964f : -1.5707964f;
        out[0] = (real)acos(vector.k);
        return;
    }
    out[1] = (real)atan(vector.j / vector.i);
    if (vector.i < 0.0f) {
        out[1] = out[1] + 3.1415927f;
    }
    out[0] = (real)acos(vector.k);
}

uint8_t VectorQuantizer::waypoint_table_quantize_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t i;

    (void)_isnan(*(real *)&descriptor[0]);
    (void)_isnan(*(real *)&descriptor[1]);
    if (descriptor[3] == 0) {
        descriptor[3] = (1 << descriptor[2]) - 1;
    }
    if (descriptor[5] == 0) {
        descriptor[5] = (1 << descriptor[4]) - 1;
    }
    for (i = 0; i < descriptor[6]; i++) {
        real *point = (real *)(descriptor + 7 + i * 3);

        if (_isnan(point[0]) || _isnan(point[1]) || _isnan(point[2])) {
            return 0;
        }
        vector3d_quantize(descriptor + 0x67 + i * 3, descriptor, point);
    }
    return 1;
}

}  // namespace halo::networking

extern "C" {
void digital_throttle_decode_vector(real *out, uint32_t code)
{
    halo::networking::VectorQuantizer::digital_throttle_decode_vector(out, code);
}

int32_t digital_throttle_encode_vector(real_vector3d vector)
{
    return halo::networking::VectorQuantizer::digital_throttle_encode_vector(vector);
}

void vector3d_from_yaw_pitch(real_vector3d *out_direction, real yaw, real pitch)
{
    halo::networking::VectorQuantizer::vector3d_from_yaw_pitch(out_direction, yaw, pitch);
}

void vector3d_lerp_by_mode_denominator(vector3d_lerp_table *table, real_vector3d *out_point,
    int32_t *ratios)
{
    halo::networking::VectorQuantizer::vector3d_lerp_by_mode_denominator(table, out_point, ratios);
}

void vector3d_quantize(int32_t *out_indices, int32_t *descriptor, real *point)
{
    halo::networking::VectorQuantizer::run_vector3d_quantize(out_indices, descriptor, point);
}

void vector3d_to_angles(real *out, real_vector3d vector)
{
    halo::networking::VectorQuantizer::vector3d_to_angles(out, vector);
}

uint8_t waypoint_table_quantize_initialize(message_delta_field_type *field_type)
{
    return halo::networking::VectorQuantizer::waypoint_table_quantize_initialize(field_type);
}

}
