// digital_throttle_encode_vector  (reached only through a .data code pointer; no C existed)
// address 0x4eb050, size 97 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb050..0x4eb0b0: stack vector by value: 4 bits -- x > 0.0001, x < -0.0001, y >
//   0.0001, y < -0.0001 (high to low).
// blam-cc: stack -> vector (12 bytes by value)

#include "message_delta_codec.h"
#include "fn_networking.h"

int32_t digital_throttle_encode_vector(real_vector3d vector)
{
    uint32_t code = vector.i > 0.0001f ? 1 : 0;

    code = code << 1 | (vector.i < -0.0001f ? 1 : 0);
    code = code << 1 | (vector.j > 0.0001f ? 1 : 0);
    code = code << 1 | (vector.j < -0.0001f ? 1 : 0);
    return (int32_t)((code << 1) >> 1);
}
