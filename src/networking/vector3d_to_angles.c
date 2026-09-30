// vector3d_to_angles  (reached only through a .data code pointer; no C existed)
// address 0x4ea720, size 167 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea720..0x4ea7c6: ESI out, stack vector (by value): normalizes a copy; out[1] =
//   atan(y / x) (+pi for x < 0), or +/-pi/2 by the sign of y when |x| <= 0.0001; out[0] = acos(z). The inverse of
//   vector3d_from_yaw_pitch 0x4ea7d0.
// blam-cc: ESI -> out, stack -> vector (12 bytes by value)

#include "message_delta_codec.h"
#include "fn_math.h"
#include "fn_networking.h"


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
