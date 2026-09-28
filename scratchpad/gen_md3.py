exec(open(r'C:\Users\Liam-\halo-re\scratchpad\md_lib.py').read())
QF = 'extern uint32_t message_delta_quantize_float_to_int(uint32_t max_level, real value, real minimum, real maximum); // 0x4ea480, ESI max_level\n'
HDR = '#include "tags.h"\n#include "memory.h"\n#include "math.h"\n#include "game.h"\n#include "networking.h"\n'
emit(0x4eb4a0, 123, 'vector3d_quantize', 'REWRITTEN (the previous C took the level count as an argument; the binary picks it): EBX out, EDI descriptor {min, max, bits1, levels1, bits0, levels0, ...}, stack point: each axis through message_delta_quantize_float_to_int over [min, max] with levels1 when the connection mode (0x69b350) is set, else levels0.', '''
extern uint32_t message_delta_vector3d_mode; // 0x0069b350
''' + QF + '''
void vector3d_quantize(int32_t *out_indices, int32_t *descriptor, real *point)
{
    uint32_t levels = message_delta_vector3d_mode != 0 ? (uint32_t)descriptor[3] : (uint32_t)descriptor[5];
    real minimum = *(real *)&descriptor[0];
    real maximum = *(real *)&descriptor[1];

    out_indices[0] = (int32_t)message_delta_quantize_float_to_int(levels, point[0], minimum, maximum);
    out_indices[1] = (int32_t)message_delta_quantize_float_to_int(levels, point[1], minimum, maximum);
    out_indices[2] = (int32_t)message_delta_quantize_float_to_int(levels, point[2], minimum, maximum);
}
''', cc='EBX -> out_indices, EDI -> descriptor, stack -> point', header=HDR)
emit(0x4eb560, 288, 'waypoint_table_quantize_initialize', 'REWRITTEN (the previous C treated the level counts as floats and passed vector3d_quantize a level): the velocity kind  initializer (0x69a56c). The validity of the range and bit widths is computed but not returned (dead in the binary); missing level counts become 2^bits - 1; each of the descriptor  waypoints (+0x1c, 12 bytes) must be NaN-free (else 0) and is quantized into the table at +0x19c. 1.', '''
extern void vector3d_quantize(int32_t *out_indices, int32_t *descriptor, real *point); // 0x4eb4a0
extern int _isnan(double x);

uint8_t waypoint_table_quantize_initialize(message_delta_field_type *field_type)
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
''')
print('ok')
