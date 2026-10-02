// message_delta_throttle_encode  (reached only through a .data code pointer; no C existed)
// address 0x4eb160, size 110 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb160..0x4eb1cd: the throttle  4-bit code; the same as the previous vector
//   code: 0; else 4 bits.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t digital_throttle_encode_vector(real_vector3d vector); // 0x4eb050

int32_t message_delta_throttle_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t code = digital_throttle_encode_vector(*(real_vector3d *)current);

    (void)field_type;
    if (previous != 0 && code == digital_throttle_encode_vector(*(real_vector3d *)previous)) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)&code, 4);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
