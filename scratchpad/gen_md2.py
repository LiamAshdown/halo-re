exec(open(r'C:\Users\Liam-\halo-re\scratchpad\md_lib.py').read())
e = emit
MODE = 'extern uint32_t message_delta_vector3d_mode; // 0x0069b350, nonzero picks the first bit widths\n'
FLOOR = 'extern double floor(double x);\n'
QF = 'extern uint32_t message_delta_quantize_float_to_int(uint32_t max_level, real value, real minimum, real maximum); // 0x4ea480, ESI max_level\n'
REG = 'extern void message_delta_parameters_protocol_register(char *scope, char *name, int32_t type, void *value); // 0x4ebe00, EAX scope\n'
PARAMS = 'extern uint8_t message_delta_parameters_enabled; // 0x0071cfa8\n'
LEVELS = '(uint32_t)((1 << (bits)) - 1)'

# ---------------------------------------------------------------- kind 21: unit normal as two angles
e(0x4ea6a0, 29, 'message_delta_normal_compute_size', 'the larger of the two bit-width pairs (a + b, c + d).', '''
int32_t message_delta_normal_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t second = descriptor[2] + descriptor[3];
    int32_t first = descriptor[0] + descriptor[1];

    return second > first ? second : first;
}
''')
e(0x4ea6c0, 92, 'message_delta_normal_initialize', 'with the parameters protocol on, registers the phi and theta bit widths under the type  name ("bits_theta_internet" -> +0, "bits_phi_internet" -> +4); all four widths positive.', REG + PARAMS + '''
uint8_t message_delta_normal_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    if (message_delta_parameters_enabled == 1) {
        message_delta_parameters_protocol_register(field_type->name, "bits_theta_internet", 1, descriptor);
        message_delta_parameters_protocol_register(field_type->name, "bits_phi_internet", 1, descriptor + 1);
    }
    return descriptor[0] > 0 && descriptor[1] > 0 && descriptor[2] > 0 && descriptor[3] > 0;
}
''')
e(0x4ea720, 167, 'vector3d_to_angles', 'ESI out, stack vector (by value): normalizes a copy; out[1] = atan(y / x) (+pi for x < 0), or +/-pi/2 by the sign of y when |x| <= 0.0001; out[0] = acos(z). The inverse of vector3d_from_yaw_pitch 0x4ea7d0.', '''
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX v
extern double acos(double x);
extern double atan(double x);

void vector3d_to_angles(real *out, real_vector3d vector)
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
''', cc='ESI -> out, stack -> vector (12 bytes by value)')
e(0x4ea8b0, 441, 'message_delta_normal_encode', 'the direction as angles: acos(z) / pi and (yaw + pi/2) / 2pi, each times its level count (2^bits - 1, bits by connection mode) + 0.5, floored and clamped; unchanged from the previous direction quantized over [0, pi] and [-pi/2, 3pi/2]: 0; else both levels.', MODE + FLOOR + QF + '''
extern void vector3d_to_angles(real *out, real_vector3d vector); // 0x4ea720

int32_t message_delta_normal_encode''' + SIG + '''
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t bits_a = message_delta_vector3d_mode != 0 ? descriptor[0] : descriptor[2];
    int32_t bits_b = message_delta_vector3d_mode != 0 ? descriptor[1] : descriptor[3];
    uint32_t levels_a = (uint32_t)((1 << bits_a) - 1);
    uint32_t levels_b = (uint32_t)((1 << bits_b) - 1);
    real angles[2];
    uint32_t level_a;
    uint32_t level_b;

    vector3d_to_angles(angles, *(real_vector3d *)current);
    level_a = (uint32_t)(int64_t)floor((double)(angles[0] * 0.31830987f * (real)levels_a + 0.5f));
    if (level_a > levels_a) {
        level_a = levels_a;
    }
    level_b = (uint32_t)(int64_t)floor((double)((angles[1] - -1.5707964f) * 0.15915494f * (real)levels_b + 0.5f));
    if (level_b > levels_b) {
        level_b = levels_b;
    }
    if (previous != 0) {
        uint32_t previous_a;

        vector3d_to_angles(angles, *(real_vector3d *)previous);
        previous_a = message_delta_quantize_float_to_int(levels_a, angles[0], 0.0f, 3.1415927f);
        if (level_a == previous_a &&
            level_b == message_delta_quantize_float_to_int(levels_b, angles[1], -1.5707964f, 4.712389f)) {
            return 0;
        }
    }
    return bit_stream_write_bits_chunked(stream, &level_a, bits_a) + bit_stream_write_bits_chunked(stream, &level_b, bits_b);
}
''')
e(0x4eaa70, 233, 'message_delta_normal_decode', 'both levels (bits by connection mode) back to angles -- a / levels * pi and b / levels * 2pi - pi/2 -- and vector3d_from_yaw_pitch into the destination.', MODE + '''
extern void vector3d_from_yaw_pitch(real_vector3d *out_direction, real yaw, real pitch); // 0x4ea7d0, ECX out

int32_t message_delta_normal_decode''' + SIG + '''
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t bits_a = message_delta_vector3d_mode != 0 ? descriptor[0] : descriptor[2];
    int32_t bits_b = message_delta_vector3d_mode != 0 ? descriptor[1] : descriptor[3];
    uint32_t value_a = 0;
    uint32_t value_b = 0;
    int32_t bits;
    real angle_a;
    real angle_b;

    (void)previous;
    bits = bit_stream_read_bits_chunked(bits_a, &value_a, stream);
    bits += bit_stream_read_bits_chunked(bits_b, &value_b, stream);
    angle_a = (real)((double)value_a / (double)(uint32_t)((1 << bits_a) - 1)) * 3.1415927f;
    angle_b = (real)((double)value_b / (double)(uint32_t)((1 << bits_b) - 1)) * 6.2831855f - 1.5707964f;
    vector3d_from_yaw_pitch((real_vector3d *)current, angle_a, angle_b);
    return bits;
}
''')

# ---------------------------------------------------------------- kind 22: locality reference position
LOC = '''extern uint32_t message_delta_vector3d_absolute_bits_mode1; // 0x0069a2cc
extern uint32_t message_delta_vector3d_delta_bits;          // 0x0069a2d0
extern real message_delta_vector3d_delta_range;             // 0x0069a2d4
extern real message_delta_vector3d_delta_epsilon;           // 0x0069a2d8
extern uint32_t message_delta_vector3d_absolute_bits_mode0; // 0x0069a2dc
'''
e(0x4eab60, 26, 'message_delta_locality_compute_size', 'a flag plus three absolute components at the larger of the two absolute widths.', LOC + '''
int32_t message_delta_locality_compute_size(message_delta_field_type *field_type)
{
    int32_t mode1 = (int32_t)message_delta_vector3d_absolute_bits_mode1 * 3 + 1;
    int32_t mode0 = (int32_t)message_delta_vector3d_absolute_bits_mode0 * 3 + 1;

    (void)field_type;
    return mode0 > mode1 ? mode0 : mode1;
}
''')
e(0x4eab80, 91, 'message_delta_locality_initialize', 'with the parameters protocol on registers LOCALITY_BITS_PER_COMPONENT_FULL / _DELTA (ints), LOCALITY_DELTA_CUTOFF_DISTANCE and LOCALITY_MINIMUM_MOVE_DISTANCE (reals); valid.', LOC + REG + PARAMS + '''
uint8_t message_delta_locality_initialize(message_delta_field_type *field_type)
{
    (void)field_type;
    if (message_delta_parameters_enabled == 1) {
        message_delta_parameters_protocol_register(0, "LOCALITY_BITS_PER_COMPONENT_FULL", 1,
            &message_delta_vector3d_absolute_bits_mode1);
        message_delta_parameters_protocol_register(0, "LOCALITY_BITS_PER_COMPONENT_DELTA", 1, &message_delta_vector3d_delta_bits);
        message_delta_parameters_protocol_register(0, "LOCALITY_DELTA_CUTOFF_DISTANCE", 0, &message_delta_vector3d_delta_range);
        message_delta_parameters_protocol_register(0, "LOCALITY_MINIMUM_MOVE_DISTANCE", 0,
            &message_delta_vector3d_delta_epsilon);
    }
    return 1;
}
''')
e(0x4eaed0, 374, 'message_delta_locality_decode', 'with a previous position a flag bit picks the form: 0 -- per axis a sign bit and a delta-bits magnitude, v / levels * range (negated for the sign) added to the previous position; 1 (or no previous) -- three absolute components at the mode  width, v / levels * 10000 - 5000. The bits read.', LOC + MODE + '''
int32_t message_delta_locality_decode''' + SIG + '''
{
    real *destination = (real *)current;
    int32_t total = 0;
    uint32_t bits;
    int32_t i;

    (void)field_type;
    if (previous != 0) {
        uint8_t absolute;

        total = (int32_t)bit_stream_read_bit(&absolute, stream);
        if (!absolute) {
            uint8_t sign[3];
            real delta[3];

            for (i = 0; i < 3; i++) {
                uint32_t value = 0;
                int32_t sign_bits = (int32_t)bit_stream_read_bit(&sign[i], stream);
                int32_t value_bits = bit_stream_read_bits_chunked((int32_t)message_delta_vector3d_delta_bits, &value, stream);

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

        total += bit_stream_read_bits_chunked((int32_t)bits, &value, stream);
        destination[i] = (real)((double)value / (double)(uint32_t)((1 << bits) - 1)) * 10000.0f - 5000.0f;
    }
    return total;
}
''')

# ---------------------------------------------------------------- kind 23: digital throttle
e(0x4eb050, 97, 'digital_throttle_encode_vector', 'stack vector by value: 4 bits -- x > 0.0001, x < -0.0001, y > 0.0001, y < -0.0001 (high to low).', '''
int32_t digital_throttle_encode_vector(real_vector3d vector)
{
    uint32_t code = vector.i > 0.0001f ? 1 : 0;

    code = code << 1 | (vector.i < -0.0001f ? 1 : 0);
    code = code << 1 | (vector.j > 0.0001f ? 1 : 0);
    code = code << 1 | (vector.j < -0.0001f ? 1 : 0);
    return (int32_t)((code << 1) >> 1);
}
''', cc='stack -> vector (12 bytes by value)')
e(0x4eb0c0, 129, 'digital_throttle_decode_vector', 'ECX out, EDX code: zero; for a code, y then x from the bit pairs (low bit -1, high bit +1), then normalized when longer than 0.0001.', '''
extern double sqrt(double x);

void digital_throttle_decode_vector(real *out, uint32_t code)
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
''', cc='ECX -> out, EDX -> code')
e(0x4eb150, 6, 'message_delta_compute_size_4', '4 bits (digital throttle).', '''
int32_t message_delta_compute_size_4(message_delta_field_type *field_type)
{
    (void)field_type;
    return 4;
}
''')
e(0x4eb160, 110, 'message_delta_throttle_encode', 'the throttle  4-bit code; the same as the previous vector  code: 0; else 4 bits.', '''
extern int32_t digital_throttle_encode_vector(real_vector3d vector); // 0x4eb050

int32_t message_delta_throttle_encode''' + SIG + '''
{
    int32_t code = digital_throttle_encode_vector(*(real_vector3d *)current);

    (void)field_type;
    if (previous != 0 && code == digital_throttle_encode_vector(*(real_vector3d *)previous)) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)&code, 4);
}
''')
e(0x4eb1d0, 52, 'message_delta_throttle_decode', '4 bits back into a unit throttle vector.', '''
extern void digital_throttle_decode_vector(real *out, uint32_t code); // 0x4eb0c0

int32_t message_delta_throttle_decode''' + SIG + '''
{
    uint32_t code = 0;
    int32_t bits = bit_stream_read_bits_chunked(4, &code, stream);

    (void)field_type;
    (void)previous;
    digital_throttle_decode_vector((real *)current, code);
    return bits;
}
''')

# ---------------------------------------------------------------- kinds 24, 25: weapon / grenade index (-1 or small)
for (addr_c, addr_e, addr_d, kind, bits, mask, hi) in ((0x4eb210, 0x4eb220, 0x4eb280, 'weapon', 3, 3, 4),
                                                         (0x4eb2c0, 0x4eb2d0, 0x4eb330, 'grenade', 2, 1, 2)):
    e(addr_c, 6, 'message_delta_compute_size_%d' % bits, '%d bits (%s index).' % (bits, kind), '''
int32_t message_delta_compute_size_%d(message_delta_field_type *field_type)
{
    (void)field_type;
    return %d;
}
''' % (bits, bits))
    e(addr_e, 89, 'message_delta_%s_index_encode' % kind, 'the short as ((index == -1) << %d) | (index & %d); unchanged: 0; else %d bits.' % (bits - 1, mask, bits), '''
int32_t message_delta_%s_index_encode''' % kind + SIG + '''
{
    uint16_t index = *(uint16_t *)current;
    int32_t code = ((index == 0xffff) << %d) | (index & %d);

    (void)field_type;
    if (previous != 0) {
        int16_t previous_index = *(int16_t *)previous;

        if (code == ((((uint16_t)previous_index == 0xffff) << %d) | (previous_index & %d))) {
            return 0;
        }
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)&code, %d);
}
''' % (bits - 1, mask, bits - 1, mask, bits))
    e(addr_d, 60, 'message_delta_%s_index_decode' % kind, '%d bits: the high bit means -1, else the low bits.' % bits, '''
int32_t message_delta_%s_index_decode''' % kind + SIG + '''
{
    uint32_t code = 0;
    int32_t bits = bit_stream_read_bits_chunked(%d, &code, stream);

    (void)field_type;
    (void)previous;
    if (code & %d) {
        *(int16_t *)current = -1;
    } else {
        *(int16_t *)current = (int16_t)(code & %d);
    }
    return bits;
}
''' % (bits, hi, mask))

# ---------------------------------------------------------------- kind 26: velocity through a waypoint table
e(0x4eb520, 56, 'message_delta_velocity_compute_size', 'the index prefix (count + 1) plus three components at the wider of the two widths.', '''
int32_t message_delta_velocity_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t mode0 = descriptor[4] * 3;
    int32_t mode1 = descriptor[2] * 3;

    if (descriptor[6] + mode0 + 1 > mode1 + descriptor[6] + 1) {
        return descriptor[6] + mode0 + 1;
    }
    return descriptor[6] + mode1 + 1;
}
''')
e(0x4eb680, 519, 'message_delta_velocity_encode', 'quantizes the vector (vector3d_quantize, bits by connection mode). Without a previous value: a vector in the waypoint table (+0x19c) is sent as i + 1 one-bits from 0x65d438 and, with more than one waypoint, a terminating 0 written in place; any other vector is a 0 bit and three components. With a previous value: unchanged quantized: 0; else just the three components.', MODE + '''
extern void vector3d_quantize(int32_t *out_indices, int32_t *descriptor, real *point); // 0x4eb4a0
extern uint32_t message_delta_unary_ones[]; // 0x0065d438

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

int32_t message_delta_velocity_encode''' + SIG + '''
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t bits = message_delta_vector3d_mode != 0 ? descriptor[2] : descriptor[4];
    int32_t quantized[3];
    int32_t total;

    vector3d_quantize(quantized, descriptor, (real *)current);
    if (previous == 0) {
        int32_t *table = descriptor + 0x67;
        int32_t i;

        for (i = 0; i < descriptor[6]; i++) {
            if (quantized[0] == table[i * 3] && quantized[1] == table[i * 3 + 1] && quantized[2] == table[i * 3 + 2]) {
                break;
            }
        }
        if (i < descriptor[6]) {
            total = bit_stream_write_bits_chunked(stream, message_delta_unary_ones, i + 1);
            if (descriptor[6] > 1) {
                total += write_zero_bit(stream);
            }
            return total;
        }
        total = write_zero_bit(stream);
    } else {
        int32_t old[3];

        vector3d_quantize(old, descriptor, (real *)previous);
        if (old[0] == quantized[0] && old[1] == quantized[1] && old[2] == quantized[2]) {
            return 0;
        }
        total = 0;
    }
    total += bit_stream_write_bits_chunked(stream, (const uint32_t *)&quantized[0], bits);
    total += bit_stream_write_bits_chunked(stream, (const uint32_t *)&quantized[1], bits);
    total += bit_stream_write_bits_chunked(stream, (const uint32_t *)&quantized[2], bits);
    return total;
}
''')

# ---------------------------------------------------------------- kind 27: item placement position
PLACE = '''extern uint32_t item_placement_bits_x; // 0x0069a2e0
extern uint32_t item_placement_bits_y; // 0x0069a2e4
extern uint32_t item_placement_bits_z; // 0x0069a2e8
'''
e(0x4eba40, 20, 'message_delta_item_placement_compute_size', 'the three component widths summed.', PLACE + '''
int32_t message_delta_item_placement_compute_size(message_delta_field_type *field_type)
{
    (void)field_type;
    return (int32_t)(item_placement_bits_y + item_placement_bits_z + item_placement_bits_x);
}
''')
e(0x4eba60, 72, 'message_delta_item_placement_initialize', 'with the parameters protocol on registers the three widths (ints, no scope); valid.', PLACE + REG + PARAMS + '''
uint8_t message_delta_item_placement_initialize(message_delta_field_type *field_type)
{
    (void)field_type;
    if (message_delta_parameters_enabled == 1) {
        message_delta_parameters_protocol_register(0, (char *)0x66e564, 1, &item_placement_bits_x);
        message_delta_parameters_protocol_register(0, (char *)0x66e54c, 1, &item_placement_bits_y);
        message_delta_parameters_protocol_register(0, (char *)0x66e534, 1, &item_placement_bits_z);
    }
    return 1;
}
''')
AX = '''    {
        uint32_t levels = (uint32_t)((1 << item_placement_bits_%s) - 1);
        uint32_t level = (uint32_t)(int64_t)floor((double)((real)levels * ((position[%d] - -5000.0f) * 0.0001f) + 0.5f));

        if (level > levels) {
            level = levels;
        }
        total += bit_stream_write_bits_chunked(stream, &level, (int32_t)item_placement_bits_%s);
    }
'''
e(0x4ebab0, 354, 'message_delta_item_placement_encode', 'each axis of the position as floor((v + 5000) / 10000 * levels + 0.5) clamped, at its width; always sent (the previous value is not looked at).', PLACE + FLOOR + '''
int32_t message_delta_item_placement_encode''' + SIG + '''
{
    real *position = (real *)current;
    int32_t total = 0;

    (void)field_type;
    (void)previous;
''' + AX % ('x', 0, 'x') + AX % ('y', 1, 'y') + AX % ('z', 2, 'z') + '''    return total;
}
''')
DX = '''    value = 0;
    total += bit_stream_read_bits_chunked((int32_t)item_placement_bits_%s, &value, stream);
    position[%d] = (real)((double)value / (double)(uint32_t)((1 << item_placement_bits_%s) - 1)) * 10000.0f - 5000.0f;
'''
e(0x4ebc20, 297, 'message_delta_item_placement_decode', 'each axis back as v / levels * 10000 - 5000.', PLACE + '''
int32_t message_delta_item_placement_decode''' + SIG + '''
{
    real *position = (real *)current;
    uint32_t value;
    int32_t total = 0;

    (void)field_type;
    (void)previous;
''' + DX % ('x', 0, 'x') + DX % ('y', 1, 'y') + DX % ('z', 2, 'z') + '''    return total;
}
''')
print('ok')

# placement parameter names
p = 'src/networking/message_delta_item_placement_initialize.c'
s = open(p, encoding='utf-8').read()
s = s.replace('(char *)0x66e564', '"gITEM_PLACEMENT_BITS_X"').replace('(char *)0x66e54c', '"gITEM_PLACEMENT_BITS_Y"').replace('(char *)0x66e534', '"gITEM_PLACEMENT_BITS_Z"')
open(p, 'w', encoding='utf-8').write(s)
