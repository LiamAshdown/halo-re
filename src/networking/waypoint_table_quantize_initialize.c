// waypoint_table_quantize_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4eb560, size 288 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb560..0x4eb67f: REWRITTEN (the previous C treated the level counts as floats
//   and passed vector3d_quantize a level): the velocity kind  initializer (0x69a56c). The validity of the range and
//   bit widths is computed but not returned (dead in the binary); missing level counts become 2^bits - 1; each of the
//   descriptor  waypoints (+0x1c, 12 bytes) must be NaN-free (else 0) and is quantized into the table at +0x19c. 1.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
