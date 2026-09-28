// message_delta_locality_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4eab60, size 26 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eab60..0x4eab79: a flag plus three absolute components at the larger of the
//   two absolute widths.
// blam-cc: cdecl

#include "message_delta_codec.h"

extern uint32_t message_delta_vector3d_absolute_bits_mode1; // 0x0069a2cc
extern uint32_t message_delta_vector3d_delta_bits;          // 0x0069a2d0
extern real message_delta_vector3d_delta_range;             // 0x0069a2d4
extern real message_delta_vector3d_delta_epsilon;           // 0x0069a2d8
extern uint32_t message_delta_vector3d_absolute_bits_mode0; // 0x0069a2dc

int32_t message_delta_locality_compute_size(message_delta_field_type *field_type)
{
    int32_t mode1 = (int32_t)message_delta_vector3d_absolute_bits_mode1 * 3 + 1;
    int32_t mode0 = (int32_t)message_delta_vector3d_absolute_bits_mode0 * 3 + 1;

    (void)field_type;
    return mode0 > mode1 ? mode0 : mode1;
}
