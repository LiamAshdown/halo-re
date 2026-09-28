// vector3d_quantize  (reached only through a .data code pointer; no C existed)
// address 0x4eb4a0, size 123 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb4a0..0x4eb51a: REWRITTEN (the previous C took the level count as an
//   argument; the binary picks it): EBX out, EDI descriptor {min, max, bits1, levels1, bits0, levels0, ...}, stack
//   point: each axis through message_delta_quantize_float_to_int over [min, max] with levels1 when the connection
//   mode (0x69b350) is set, else levels0.
// blam-cc: EBX -> out_indices, EDI -> descriptor, stack -> point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint32_t message_delta_vector3d_mode; // 0x0069b350
extern uint32_t message_delta_quantize_float_to_int(uint32_t max_level, real value, real minimum, real maximum); // 0x4ea480, ESI max_level

void vector3d_quantize(int32_t *out_indices, int32_t *descriptor, real *point)
{
    uint32_t levels = message_delta_vector3d_mode != 0 ? (uint32_t)descriptor[3] : (uint32_t)descriptor[5];
    real minimum = *(real *)&descriptor[0];
    real maximum = *(real *)&descriptor[1];

    out_indices[0] = (int32_t)message_delta_quantize_float_to_int(levels, point[0], minimum, maximum);
    out_indices[1] = (int32_t)message_delta_quantize_float_to_int(levels, point[1], minimum, maximum);
    out_indices[2] = (int32_t)message_delta_quantize_float_to_int(levels, point[2], minimum, maximum);
}
