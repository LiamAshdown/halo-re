// digital_throttle_decode_vector  (reached only through a .data code pointer; no C existed)
// address 0x4eb0c0, size 129 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb0c0..0x4eb140: ECX out, EDX code: zero; for a code, y then x from the bit
//   pairs (low bit -1, high bit +1), then normalized when longer than 0.0001.
// blam-cc: ECX -> out, EDX -> code

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
